

#include <cstdio>

#include "Gen_386.h"

void Gen_386::
gen_rodata(std::ostream &os) {
   for (auto kv : rodata) {
      os << kv.second << ":" << std::endl;
      Variable *var = kv.first;
      if (var->type == Variable::DQString) {
          os << "\t.asciz \"";
          for (char c : var->dqstring) {
            if (c >= 0x20 && c < 0x7F) {
               os << c;
            } else if (c == '\n') {
               os << "\\n";
            }
          }
          os << "\"" << std::endl;
      }
   }
}

static int pb_num = 0;

void Gen_386::
gen_func_params(std::vector<Variable> &plist, std::ostream &os) {
   if (plist.size() < 1) {
      printf("Plist empty\n");
   }
   //we assume that the stack is pre-aligned to 16-byte bounds
   if (plist.size() > 3) {
      printf("large arg list not yet supported\n");
   }
   unsigned int stack_align = (16 - (plist.size() * 4 + 8));
   os << '\t' << "sub $" << stack_align << ", %esp" << std::endl;
   std::string l0pbn = std::string("L0$pb") + std::to_string(pb_num++);
   for (size_t i = 0; i < plist.size(); ++i) {
      os << '\t' << "call " << l0pbn << std::endl;
      os << l0pbn << ":" << std::endl;
      os << '\t' << "pop %eax" << std::endl;
      os << '\t' << "pop %ecx" << std::endl;
      if (plist[i].type == Variable::DQString) {
         os << '\t' << "lea " << get_rodata(&plist[i]) << "-" << l0pbn << "(%eax), %eax"  << std::endl;
      } else if (plist[i].ptype == Variable::INT_32BIT && plist[i].is_ptype_const) {
         os << '\t' << "mov $" << plist[i].pvalue << ", %eax" << std::endl;
      }
      os << '\t' << "sub $" << 12 << ", %esp" << std::endl;
      os << '\t' << "push %eax"  << std::endl;
   }
}

void Gen_386::
gen_expression(std::string scope_name, Expression &expr, std::ostream &os) {
   Instruction prevInstr;
   for (auto &instr : expr.instructions) {
      switch (instr.type) {
         case Instruction::INCREMENT: {
            printf("Increment instruction not implemented.\n");
            //os << '\t' << "inc " << get_var_loc(instr.lvalue_data.name) << std::endl;
         } break;
         case Instruction::SUBROUTINE_JUMP: {
            if (instr.func_call_name.compare("SOS_JUMP") == 0) {
               if (instr.is_conditional_jump && !instr.condition.is_always_true) {
                  os << '\t' << "lda #" << instr.condition.right.pvalue << std::endl;
                  os << '\t' << "cmp " << get_var_loc(instr.condition.left.name) << std::endl;
                  switch (instr.condition.condition) {
                     case Conditional::LESS_THAN: {
                        os << '\t' << "bcs " << scope_name << std::endl;
                     } break;
                  }
               } else {
                  os << '\t' << "jmp " << scope_name << std::endl;
               }
            }
         } break;
         case Instruction::FUNC_CALL: {
            if (instr.func_call_name.compare("__asm__") == 0) {
               if (instr.call_target_params[0].dqstring.find_first_of(':') == std::string::npos) {
                  os << '\t';
               }
               std::string final = instr.call_target_params[0].dqstring;
               while (final.find_first_of("@") != std::string::npos) {
                  if (final.find_first_of("@0") != std::string::npos) {
                     std::string load_from_stack = "0x4(%esp)";//TODO(josh) manage params on stack
                     final.replace(final.find_first_of("@0"), 2, load_from_stack);
                  }
               }
               os << final << std::endl;
            } else {
               Function *cfunc = expr.scope->getFuncByName(instr.func_call_name);
               if (!cfunc) {
                  printf("Undefined reference to %s\n", instr.func_call_name.c_str());
               } else {
                  printf("Func: %s\n", cfunc->name.c_str());
                  gen_func_params(instr.call_target_params, os);
                  os << '\t' << "call " << "_" << cfunc->name << "" << std::endl;
                  os << '\t' << "add $16, %esp" << std::endl;
               }
            }
         } break;
         case Instruction::ASSIGN: {
            if (instr.lvalue_data.type == Variable::POINTER) {
               if (instr.lvalue_data.name.compare("return") == 0) {
                  if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                  } else {
                     os << '\t' << "lda " << get_var_loc(instr.rvalue_data.name) << std::endl;
                  }
                  os << '\t' << "rts" << std::endl;
               } else {
                  if (instr.rvalue_data.name.compare("return_reg") == 0) {
                     //no op because we presume that this always follows a lda & rts
                  } else if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                  } else {
                     os << '\t' << "lda " << get_var_loc(instr.rvalue_data.name) << std::endl;
                  }
                  os << '\t' << "sty " << get_var_loc(instr.lvalue_data.name) << std::endl;
               }
            } else if (instr.lvalue_data.type == Variable::DEREFERENCED_POINTER) {
               if (!(prevInstr.type == Instruction::ASSIGN
                     && prevInstr.lvalue_data.type == Variable::DEREFERENCED_POINTER
                     && prevInstr.lvalue_data.name.compare(instr.lvalue_data.name) == 0
                     && prevInstr.rvalue_data.pvalue == instr.rvalue_data.pvalue)) {
                  if (!instr.lvalue_data.is_type_const) {
                     os << '\t' << "ldy " << get_var_loc(instr.lvalue_data.name) << std::endl;
                  }
                  if (instr.rvalue_data.name.compare("return_reg") == 0) {
                     //no op because we presume that this always follows a lda & rts
                  } else if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                  } else {
                     os << '\t' << "lda " << get_var_loc(instr.rvalue_data.name) << std::endl;
                  }
               }
               if (instr.lvalue_data.is_type_const) {
                  os << '\t' << "sta " << instr.lvalue_data.pvalue << std::endl;
               } else {
                  os << '\t' << "sta 0,y" << std::endl;
               }
            }
         } break;
      }
      
      prevInstr = instr;
   }
}

void Gen_386::
gen_scope_functions(Scope &scope, std::ostream &os) {
    for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}

void Gen_386::
gen_scope_expressions(std::string scope_name, Scope &scope, std::ostream &os) {
//   std::string scope_name = std::string("scope_") + std::to_string(get_scope_num(&scope));
   for (auto &expr : scope.expressions) {
      gen_expression(scope_name, expr, os);
      gen_scope(*expr.scope, os);
   }
}

void Gen_386::
gen_function(Function &func, std::ostream &os) {
   if (func.name.compare("__asm__") != 0) {
      
      //variable_ram_map.clear();
      if (!func.should_inline && !func.is_not_definition) {
         os << ".globl _" << func.name << std::endl;
         os << "_" << func.name << ":" << std::endl;
         if (!func.plain_instructions) {
            os << '\t' << "push %ebp" << std::endl;
            os << '\t' << "mov %esp, %ebp" << std::endl;
            os << '\t' << "sub $8, %esp" << std::endl;
         }
         gen_scope_expressions("_" + func.name, *func.scope, os);
         if (!func.plain_instructions) {
            os << '\t' << "add $8, %esp" << std::endl;
            os << '\t' << "pop %ebp" << std::endl;
         }
         if (func.return_info.ptype == Variable::VOID) {
            os << '\t' << "ret" << std::endl;
         }
      }
      gen_scope_functions(*func.scope, os);
   }
}

void Gen_386::
gen_scope(Scope &scope, std::ostream &os) {
   if (scope.empty()) {
      return;
   }
   unsigned int scope_num = get_scope_num(&scope);
   std::string scope_name = std::string("scope_") + std::to_string(scope_num);
   if (scope_num != 0) {
      os << scope_name << ":" << std::endl;
   }
   gen_scope_expressions(scope_name, scope, os);
   for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}


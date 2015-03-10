

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

//Represents the stack for the current function's frame
//every new scope starts off with the stack pointer being 16-byte aligned
//the earliest declared variable has the highest mem addr, the latest declared has the lowest addr
//padding always sits under the variables on the stack
struct StackMan {
   std::vector<Variable> params;
   Scope *scope = nullptr;
   
   std::string load_var(Variable var, Scope *ts = nullptr, int total_adjust = 0) {
      int stack_loc = 0;
      for (int i = 0; i < params.size(); ++i) {
         if (params[i].name.compare(var.name) == 0) {
            stack_loc = i * 4 + 8;// +8 accounts for push %ebp and 4 byte return address
            return std::to_string(stack_loc) + "(%ebp)";
         }
      }

      
      if (scope && !ts) {
//         printf("Scope\n");
//         printf("Scope is func? %s\n", scope->is_function ? "true" : "false");
         for (int i = 0; i < scope->variables.size(); ++i) {
//            printf("Scope name %s\n", scope->variables[i].name.c_str());
//            printf("Scope LF %s\n", var.name.c_str());
            if (scope->variables[i].name.compare(var.name) == 0) {
               int padding = 16 - ((scope->variables.size() * 4) % 16);
               if (padding == 16) padding = 0;
               stack_loc = i * 4 + padding + total_adjust;
               return std::to_string(stack_loc) + "(%esp)";
            }
         }
      } else if (ts) {
         printf("TS\n");
         for (int i = 0; i < ts->variables.size(); ++i) {
//            printf("name %s\n", ts->variables[i].name.c_str());
//            printf("LF %s\n", var.name.c_str());
            if (ts->variables[i].name.compare(var.name) == 0) {
               int padding = 16 - ((ts->variables.size() * 4) % 16);
               if (padding == 16) padding = 0;
               stack_loc = i * 4 + padding + total_adjust;
               return std::to_string(stack_loc) + "(%esp)";
            }
         }
      }
      Scope *sc = (ts ? ts : scope);
      int padding = 16 - ((sc->variables.size() * 4) % 16);
      if (padding == 16) padding = 0;
      int adj = padding + sc->variables.size() * 4;
      if (sc->variables.size() == 0) {
         adj = 0;
      }
      adj += total_adjust;
      if ((ts ? !ts->parent : !scope->parent)) {
         return "";
      }
      return (scope->is_function ? ""
         : (ts ? (ts->is_function ? "" : load_var(var, ts->parent, adj)) : load_var(var, scope->parent, adj)));
   }

};

static int pb_num = 0;
StackMan stack_man;

void Gen_386::
gen_func_params(std::vector<Variable> &plist, std::ostream &os) {
   if (plist.size() < 1) {
      printf("Plist empty\n");
      return;
   }
   //we assume that the stack is pre-aligned to 16-byte bounds
   if (plist.size() > 3) {
      printf("large arg list not yet supported\n");
   }
   unsigned int stack_align = (16 - (plist.size() * 4));
   while (((int)stack_align) < 0) stack_align += 16;
   if (plist.size() == 0) stack_align = 0;
//   printf("plist size: %ld\n", plist.size());
//   printf("stack_align: %ld\n", stack_align);
   std::string l0pbn = std::string("L0$pb") + std::to_string(pb_num++);
   os << '\t' << "call " << l0pbn << std::endl;
   os << l0pbn << ":" << std::endl;
   os << '\t' << "pop %ecx" << std::endl;
   //os << '\t' << "pop %ecx" << std::endl;
   printf("stack_align %d\n", stack_align);
   os << '\t' << "sub $" << stack_align << ", %esp" << std::endl;
   int stack_mov = 0;
   for (int i = plist.size() - 1; i >= 0; i--) {
      if (plist[i].type == Variable::DQString) {
         os << '\t' << "lea " << get_rodata(&plist[i]) << "-" << l0pbn << "(%ecx), %eax"  << std::endl;
      } else if (plist[i].type == Variable::INT_32BIT && plist[i].is_type_const) {
         os << '\t' << "mov $" << plist[i].pvalue << ", %eax" << std::endl;
      } else if (plist[i].type == Variable::INT_32BIT || plist[i].type == Variable::POINTER) {
         os << '\t' << "mov " << stack_man.load_var(plist[i], nullptr, stack_align + stack_mov) << ", %eax" << std::endl;
      } else if (plist[i].type == Variable::FLOAT_32BIT && plist[i].is_type_const) {
         printf("Float\n");
         os.precision(10);
         os.setf( std::ios::fixed, std:: ios::floatfield );
         os << '\t' << "mov $" << *reinterpret_cast<unsigned int*>(&plist[i].fvalue) << ", %eax" << std::endl;
      }
      os << '\t' << "push %eax"  << std::endl;
      stack_mov += 4;
   }
}



int unwind_stack_variables(Scope &scope) {
   int padding = 16 - ((scope.variables.size() * 4) % 16);
   if (padding == 16) padding = 0;
   int stack_adj = scope.variables.size() * 4 + padding;
   
   if (scope.variables.size() == 0) {
      stack_adj = 0;
   }
   //printf("Stack_adj: %d\n", stack_adj);
   if (scope.is_function) {
      return stack_adj;
   }
   //printf("ret: %d\n", stack_adj);
   return (scope.parent ? unwind_stack_variables(*scope.parent) + stack_adj : stack_adj);
}

static std::string gen_variable_load(Variable var) {
   if (var.name.compare("return_reg") == 0) {
      //no op because we presume that this always follows a mov x, %eax & ret
   } else if (var.is_type_const && var.type == Variable::INT_32BIT) {
      return std::string("$") + std::to_string(var.pvalue);
   } else {
       return stack_man.load_var(var);
   }
}

void Gen_386::
gen_expression(std::string scope_name, Expression &expr, std::ostream &os) {
   Instruction prevInstr;
   for (auto &instr : expr.instructions) {
      switch (instr.type) {
         case Instruction::LOG_OR: {
            os << '\t' << "or " << gen_variable_load(instr.rvalue_data) << ", %eax" << std::endl;
            os << '\t' << "mov %eax, " << gen_variable_load(instr.lvalue_data) << std::endl;
         } break;
         case Instruction::INCREMENT: {
            os << '\t' << "mov " << stack_man.load_var(instr.lvalue_data) << ", %eax" << std::endl;
            os << '\t' << "inc %eax" << std::endl;
            os << '\t' << "mov %eax, " << stack_man.load_var(instr.lvalue_data) << std::endl;
         } break;
         case Instruction::SUBROUTINE_JUMP: {
            //TODO(josh)
            if (instr.func_call_name.compare("SOS_JUMP") == 0) {
               if (instr.is_conditional_jump && !instr.condition.is_always_true) {
                  os << '\t' << "mov $" << instr.condition.right.pvalue << ", %eax" << std::endl;
                  os << '\t' << "cmp %eax, " << stack_man.load_var(instr.condition.left) << std::endl;
                  switch (instr.condition.condition) {
                     case Conditional::LESS_THAN: {
                        os << '\t' << "jl " << scope_name << std::endl;
                     } break;
                  }
               } else {
                  os << '\t' << "jmp " << scope_name << std::endl;
               }
            } else if (instr.func_call_name.compare("EOS_JUMP") == 0) {
               if (instr.is_conditional_jump && !instr.condition.is_always_true) {
                  os << '\t' << "mov $" << instr.condition.right.pvalue << ", %eax" << std::endl;
                  os << '\t' << "cmp %eax, " << stack_man.load_var(instr.condition.left) << std::endl;
                  switch (instr.condition.condition) {
                     case Conditional::LESS_THAN: {
                        os << '\t' << "jge " << scope_name << "_end" << std::endl;
                     } break;
                  }
               } else {
                  os << '\t' << "jmp " << scope_name << "_end" << std::endl;
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
                     std::string load_from_stack = stack_man.load_var(instr.call_target_params[1]);
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
                  if (instr.call_target_params.size()) {
                     gen_func_params(instr.call_target_params, os);
                  }
                  os << '\t' << "call " << "_" << cfunc->name << "" << std::endl;
                  std::vector<Variable> plist = instr.call_target_params;
                  unsigned int stack_align = (16 - (plist.size() * 4));
                  stack_align += plist.size() * 4;
                  if (plist.size() == 0) stack_align = 0;
                  if (instr.call_target_params.size()) {
                     os << '\t' << "add $" << stack_align << ", %esp" << std::endl;
                  }
               }
            }
         } break;
         case Instruction::ASSIGN: {
            if (instr.lvalue_data.type == Variable::POINTER || instr.lvalue_data.type == Variable::INT_32BIT) {
               if (instr.lvalue_data.name.compare("return") == 0) {
                  if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "mov $" << instr.rvalue_data.pvalue  << ", %eax"<< std::endl;
                  } else {
                     os << '\t' << "mov " << stack_man.load_var(instr.rvalue_data) << ", %eax" << std::endl;
                  }
                  os << '\t' << "add $" << unwind_stack_variables(*expr.scope) << ", %esp" << std::endl;
                  //os << '\t' << "add $8, %esp" << std::endl;
                  os << '\t' << "pop %ebp" << std::endl;
                  os << '\t' << "ret" << std::endl;
               } else {
                  if (instr.rvalue_data.name.compare("return_reg") == 0) {
                     //no op because we presume that this always follows a mov x, %eax & ret
                  } else if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "mov $" << instr.rvalue_data.pvalue << ", %eax" << std::endl;
                  } else if (instr.rvalue_data.type == Variable::DQString) {
                     std::string l0pbn = std::string("L0$pb") + std::to_string(pb_num++);
                     os << '\t' << "call " << l0pbn << std::endl;
                     os << l0pbn << ":" << std::endl;
                     os << '\t' << "pop %ecx" << std::endl;
                     os << '\t' << "lea " << get_rodata(&instr.rvalue_data) << "-" << l0pbn << "(%ecx), %eax"  << std::endl;
                  } else {
                     os << '\t' << "mov " << stack_man.load_var(instr.rvalue_data) << ", %eax" << std::endl;
                  }
                  os << '\t' << "mov %eax, " << stack_man.load_var(instr.lvalue_data) << std::endl;
               }
            } else if (instr.lvalue_data.type == Variable::DEREFERENCED_POINTER) {
               if (!(prevInstr.type == Instruction::ASSIGN
                     && prevInstr.lvalue_data.type == Variable::DEREFERENCED_POINTER
                     && prevInstr.lvalue_data.name.compare(instr.lvalue_data.name) == 0
                     && prevInstr.rvalue_data.pvalue == instr.rvalue_data.pvalue)) {
                  if (!instr.lvalue_data.is_type_const) {
                     os << '\t' << "mov " << stack_man.load_var(instr.lvalue_data) << ", %ecx" << std::endl;
                  }
                  if (instr.rvalue_data.name.compare("return_reg") == 0) {
                     //no op because we presume that this always follows a mov x, %eax & ret
                  } else if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "mov $" << instr.rvalue_data.pvalue << ", %eax" << std::endl;
                  } else {
                     os << '\t' << "mov $" << stack_man.load_var(instr.rvalue_data) << ", %eax" << std::endl;
                  }
               }
               if (instr.lvalue_data.is_type_const) {
                  os << '\t' << "mov %eax, $" << instr.lvalue_data.pvalue << std::endl;
               } else {
                  os << '\t' << "mov %eax, %ecx" << std::endl;//we can probably condense this operation
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
   stack_man.params = func.parameters;
   if (func.name.compare("__asm__") != 0) {
      
      //variable_ram_map.clear();
      if (!func.should_inline && !func.is_not_definition) {
         os << ".globl _" << func.name << std::endl;
         os << "_" << func.name << ":" << std::endl;
         if (!func.plain_instructions) {
            os << '\t' << "push %ebp" << std::endl;
            os << '\t' << "mov %esp, %ebp" << std::endl;
            //os << '\t' << "sub $8, %esp" << std::endl;
         }
         stack_man.scope = func.scope;
         int padding = 16 - ((func.scope->variables.size() * 4) % 16);
         if (padding == 16) padding = 0;
         int stack_adj = func.scope->variables.size() * 4 + padding;
         if (func.scope->variables.size() > 0) {
            printf("stack_align %d\n", stack_adj);
            os << '\t' << "sub $" << stack_adj << ", %esp" << std::endl;
         }
         gen_scope_expressions("_" + func.name, *func.scope, os);
         if (func.scope->variables.size() > 0) {
            os << '\t' << "add $" << stack_adj << ", %esp" << std::endl;
         }
         if (func.return_info.ptype == Variable::VOID) {
            if (!func.plain_instructions) {
               //os << '\t' << "add $8, %esp" << std::endl;
               os << '\t' << "pop %ebp" << std::endl;
            }
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
   stack_man.scope = &scope;
   unsigned int scope_num = get_scope_num(&scope);
   std::string scope_name = "L" + std::string("scope_") + std::to_string(scope_num);
   if (scope_num != 0) {
      os << scope_name << ":" << std::endl;
   }
   int padding = 16 - ((scope.variables.size() * 4) % 16);
   if (padding == 16) padding = 0;
   int stack_adj = scope.variables.size() * 4 + padding;
   if (scope.variables.size() > 0) {
      printf("stack_align %d\n", stack_adj);
      os << '\t' << "sub $" << stack_adj << ", %esp" << std::endl;
   }
   gen_scope_expressions(scope_name, scope, os);
   if (scope_num != 0) {
      os << scope_name << "_end" << ":" << std::endl;
   }
   if (scope.variables.size() > 0) {
      os << '\t' << "add $" << stack_adj << ", %esp" << std::endl;
   }
   for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}


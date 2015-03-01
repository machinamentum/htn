
#include <cstdio>

#include "Gen_6502.h"

void Gen_6502::
gen_expression(std::string scope_name, Expression &expr, std::ostream &os) {
   Instruction prevInstr;
   for (auto &instr : expr.instructions) {
      switch (instr.type) {
         case Instruction::INCREMENT: {
            os << '\t' << "inc " << get_var_loc(instr.lvalue_data.name) << std::endl;
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
                     final.replace(final.find_first_of("@0"), 2, std::to_string(get_var_loc(instr.call_target_params[1].name)));
                  }
               }
               os << final << std::endl;
            } else {
               Function *cfunc = expr.scope->getFuncByName(instr.func_call_name);
               if (!cfunc) {
                  printf("Undefined reference to %s\n", instr.func_call_name.c_str());
               } else {
                  os << '\t' << "jsr " << cfunc->name << std::endl;
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

void Gen_6502::
gen_scope_functions(Scope &scope, std::ostream &os) {
    for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}

void Gen_6502::
gen_scope_expressions(std::string scope_name, Scope &scope, std::ostream &os) {
//   std::string scope_name = std::string("scope_") + std::to_string(get_scope_num(&scope));
   for (auto &expr : scope.expressions) {
      gen_expression(scope_name, expr, os);
      gen_scope(*expr.scope, os);
   }
}

void Gen_6502::
gen_function(Function &func, std::ostream &os) {
   if (func.name.compare("__asm__") != 0) {
      
      //variable_ram_map.clear();
      if (!func.should_inline) {
         os << func.name << ":" << std::endl;
         gen_scope_expressions(func.name, *func.scope, os);
         if (func.return_info.ptype == Variable::VOID) {
            os << '\t' << "rts" << std::endl;
         }
      }
      gen_scope_functions(*func.scope, os);
   }
}

void Gen_6502::
gen_scope(Scope &scope, std::ostream &os) {
   if (scope.empty()) {
      return;
   }
   std::string scope_name = std::string("scope_") + std::to_string(get_scope_num(&scope));
   os << scope_name << ":" << std::endl;
   gen_scope_expressions(scope_name, scope, os);
   for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}

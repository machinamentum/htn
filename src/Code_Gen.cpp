
#include <cstdio>

#include "Code_Gen.h"




//names can be omitted for const data as the data is always copied about rather than looked up.
Variable create_const_int32(int value) {
   Variable var;
   var.type = Variable::INT_32BIT;
   var.pvalue = value;
   var.is_type_const = true;
   return var;
}

//RULE: if a variable is named usings special register strings found in Code_Gen.h
// all other variable data is ignored.
Variable create_register(std::string reg_name) {
   Variable var;
   var.name = reg_name;
   return var;
}


void Code_Gen::
gen_func_params(std::vector<Variable> &plist) {
   if (plist.size() < 1) {
      printf("Plist empty\n");
      return;
   }
   //we assume that the stack is pre-aligned to 16-byte bounds
   unsigned int stack_align = (16 - (plist.size() * 4));
   while (((int)stack_align) < 0) stack_align += 16;
   if (plist.size() == 0) stack_align = 0;
//   printf("plist size: %ld\n", plist.size());
//   printf("stack_align: %ld\n", stack_align);

   emit_sub(create_const_int32(stack_align), REG_STACK);
   int adj = stack_align;
   stack_man->ext_adj = adj;
   for (int i = plist.size() - 1; i >= 0; i--) {

      emit_push(plist[i]);
      adj += 4;
      stack_man->ext_adj = adj;
   }

   stack_man->ext_adj = 0;
}

void Code_Gen::
gen_expression(std::string scope_name, Expression &expr) {
   Instruction prevInstr;
   for (auto &instr : expr.instructions) {
      switch (instr.type) {
         case Instruction::BIT_OR: {
            emit_or(instr.rvalue_data, instr.lvalue_data);
         } break;
         case Instruction::INCREMENT: {
            emit_inc(instr.lvalue_data);
         } break;
         case Instruction::SUBROUTINE_JUMP: {
            std::string label = instr.func_call_name;

            if (label.compare("SOS_JUMP") == 0) {
               label = scope_name;
            } else if (label.compare("EOS_JUMP") == 0) {
               label = scope_name + "_end";
            }

            if (instr.is_conditional_jump && !instr.condition.is_always_true) {
               emit_cmp(instr.condition.right, instr.condition.left);
               emit_cond_jump(label, instr.condition.condition);
            } else {
               emit_jump(label);
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
                     std::string load_from_stack = stack_man->load_var(instr.call_target_params[1]);
                     final.replace(final.find_first_of("@0"), 2, load_from_stack);
                  }
               }
               os << final << std::endl;
            } else {
               //TODO(josh) implement name mangle + getFuncByNameAndParams
               Function *cfunc = expr.scope->getFuncByName(instr.func_call_name);
               if (!cfunc) {
                  for (auto &str : expr.scope->parent->structs) {
                     cfunc = str.scope->getFuncByName(instr.func_call_name);
                     if (cfunc) break;
                  }
               }
               if (!cfunc) {
                  //this should never happen unless the parser has a bug
                  printf("Undefined reference to %s\n", instr.func_call_name.c_str());
               } else {
                  printf("Func: %s\n", cfunc->name.c_str());
                  if (instr.call_target_params.size()) {
                     gen_func_params(instr.call_target_params);
                  }

                  emit_call("" + cfunc->name);
                  std::vector<Variable> plist = instr.call_target_params;
                  gen_stack_pop_params(plist);
               }
            }
         } break;
         case Instruction::ASSIGN: {
            if (instr.lvalue_data.type == Variable::POINTER || instr.lvalue_data.type == Variable::INT_32BIT) {
               if (instr.lvalue_data.name.compare("return") == 0) {
                  emit_mov(instr.rvalue_data, REG_RETURN);
                  if (int i = gen_stack_unwind(*expr.scope) > 0) {
                     emit_add(create_const_int32(i), REG_STACK);
                  }
                  emit_function_footer();
                  emit_return();
               } else {

                  emit_mov(instr.rvalue_data, instr.lvalue_data);
               }
            } else if (instr.lvalue_data.type == Variable::DEREFERENCED_POINTER) {
               printf("Dereferencing a pointer not supported right now\n");
               //TODO(josh)
            }
         } break;
      }

      prevInstr = instr;
   }
}

void Code_Gen::
gen_scope_functions(Scope &scope) {
   for (auto &func : scope.functions) {
      gen_function(func);
   }

   for (auto &str : scope.structs) {
      gen_scope_functions(*str.scope);
   }
}

void Code_Gen::
gen_scope_expressions(std::string scope_name, Scope &scope) {
   for (auto &expr : scope.expressions) {
      gen_expression(scope_name, expr);
      gen_scope(*expr.scope);
   }
}

void Code_Gen::
gen_function_attributes(Function &func) {
   os << ".globl " << func.name << std::endl;
   os << "" << func.name << ":" << std::endl;
}

void Code_Gen::
gen_function(Function &func) {
   stack_man->params = func.parameters;
   if (func.name.compare("__asm__") != 0) {


      if (!func.should_inline && !func.is_not_definition) {
         gen_function_attributes(func);
         if (!func.plain_instructions) {
            emit_function_header();
         }
         stack_man->scope = func.scope;
         gen_stack_alignment(*func.scope);
         gen_scope_expressions("" + func.name, *func.scope);
         gen_stack_unalignment(*func.scope);
         if (func.return_info.ptype == Variable::VOID) {
            if (!func.plain_instructions) {
               emit_function_footer();
            }
            emit_return();
         }
      }
      gen_scope_functions(*func.scope);
   }
}

void Code_Gen::
gen_scope(Scope &scope) {
   if (scope.empty()) {
      return;
   }
   stack_man->scope = &scope;
   unsigned int scope_num = get_scope_num(&scope);
   std::string scope_name = "L" + std::string("scope_") + std::to_string(scope_num);
   if (scope_num != 0) {
      os << scope_name << ":" << std::endl;
   }
   gen_stack_alignment(scope);
   gen_scope_expressions(scope_name, scope);
   if (scope_num != 0) {
      os << scope_name << "_end" << ":" << std::endl;
   }
   gen_stack_unalignment(scope);
   for (auto &func : scope.functions) {
      gen_function(func);
   }
}

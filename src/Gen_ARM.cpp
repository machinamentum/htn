#include "Gen_ARM.h"

const Variable REG_LINK = create_register("_REG_LINK");
const Variable REG_ARG0 = create_register("_ARM_ARG_R0");
const Variable REG_ARG1 = create_register("_ARM_ARG_R1");
const Variable REG_ARG2 = create_register("_ARM_ARG_R2");
const Variable REG_ARG3 = create_register("_ARM_ARG_R3");
const Variable ARM_ARGS[] = {
   REG_ARG0,
   REG_ARG1,
   REG_ARG2,
   REG_ARG3
};

void Gen_ARM::
gen_rodata() {
   for (size_t i = 0; i < rodata_data.size(); i++) {
      os << rodata_labels[i] << ":" << std::endl;
      Variable *var = &rodata_data[i];
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

int Gen_ARM::
gen_stack_unwind(Scope &scope) {
   int stack_adj = scope.variables.size() * 4;

   if (scope.variables.size() == 0) {
      stack_adj = 0;
   }
   if (scope.is_function) {
      return stack_adj;
   }
   return (scope.parent ? gen_stack_unwind(*scope.parent) + stack_adj : stack_adj);
}

void Gen_ARM::
gen_func_params(std::vector<Variable> &plist) {
   if (plist.size() < 1) {
      printf("Plist empty\n");
      return;
   }
   int size = plist.size();
   if (size > 5) {
      //TODO(josh) stack stuff
   }
   for (size_t i = 0; i < (plist.size() > 4 ? 4 : plist.size()); ++i) {
      emit_mov(plist[i], ARM_ARGS[i]);
   }
}

void Gen_ARM::
gen_function_attributes(Function &func) {
   os << "\t.align 2" << std::endl;
   os << "\t.globl " << func.name << std::endl;
   os << "\t.code 16" << std::endl;
   os << "\t.thumb_func" << std::endl;
   os << "\t.type " << func.name << ", %function" << std::endl;
}

bool is_reg(Variable var) {
   if (var.name.compare(ARM_ARGS[0].name) == 0) {
      return true;
   } else if (var.name.compare(ARM_ARGS[1].name) == 0) {
      return true;
   } else if (var.name.compare(ARM_ARGS[2].name) == 0) {
      return true;
   } else if (var.name.compare(ARM_ARGS[3].name) == 0) {
      return true;
   } else if (var.name.compare(REGISTER_RETURN) == 0) {
      return true;
   } else if (var.name.compare(REGISTER_ACCUMULATOR) == 0) {
      return true;
   } else if (var.name.compare(REGISTER_INDEX) == 0) {
      return true;
   } else if (var.name.compare(REGISTER_FRAME_POINTER) == 0) {
      return true;
   } else if (var.name.compare(REGISTER_STACK_POINTER) == 0) {
      return true;
   } else if (var.name.compare(REG_LINK.name) == 0) {
      return true;
   }
   return false;
}

std::string Gen_ARM::gen_var(Variable var) {

   if (var.name.compare(ARM_ARGS[0].name) == 0) {
      return "r0";
   } else if (var.name.compare(ARM_ARGS[1].name) == 0) {
      return "r1";
   } else if (var.name.compare(ARM_ARGS[2].name) == 0) {
      return "r2";
   } else if (var.name.compare(ARM_ARGS[3].name) == 0) {
      return "r3";
   } else if (var.name.compare(REGISTER_RETURN) == 0) {
      return "r0";
   } else if (var.name.compare(REGISTER_ACCUMULATOR) == 0) {
      return "r2";
   } else if (var.name.compare(REGISTER_INDEX) == 0) {
      return "r3";
   } else if (var.name.compare(REGISTER_FRAME_POINTER) == 0) {
      return "r7"; //NOTE THUMB = r7, ARM = r11
   } else if (var.name.compare(REGISTER_STACK_POINTER) == 0) {
      return "r13";
   } else if (var.name.compare(REG_LINK.name) == 0) {
      return "r14";
   }

   else if (var.is_type_const && var.type == Variable::INT_32BIT) {
      return std::string("#") + std::to_string(var.pvalue);
   } else if (var.is_type_const && var.type == Variable::FLOAT_32BIT) {
      float val = var.fvalue;
      return std::string("#") + std::to_string(*(int *)&val);
   } else if (var.type == Variable::DQString) {
      return "=" + get_rodata(var);
   } else {
       return stack_man->load_var(var);
   }
}

void Gen_ARM::emit_cmp(Variable src0, Variable src1) {
   std::string dst_s = gen_var(src1);
   //std::string src_s = gen_var(src0);
   // emit_mov(src0, REG_ACCUMULATOR);
   os << '\t' << "cmp " << dst_s << ", " << dst_s << ", " << gen_var(src0) << std::endl;
}

void Gen_ARM::emit_inc(Variable dst) {
   emit_add(create_const_int32(1), dst);
}

void Gen_ARM::emit_push(Variable src) {
   // std::string src_s = gen_var(src);
   //os << '\t' << "push " << src_s << std::endl;
   emit_mov(src, REG_ACCUMULATOR);
   os << '\t' << "push " << "{" << gen_var(REG_ACCUMULATOR) << "}" << std::endl;
}

void Gen_ARM::emit_pop(Variable dst) {
   std::string dst_s = gen_var(dst);
   os << '\t' << "pop " << "{ " << dst_s << " }" << std::endl;
}

void Gen_ARM::emit_mov(Variable src, Variable dst) {
   std::string instr = "ldr ";
   if ((is_reg(src) && is_reg(dst)) || (src.is_type_const)) {
      instr = "mov ";
   }
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << instr << dst_s << ", " << src_s << std::endl;
}

void Gen_ARM::emit_sub(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "sub " << dst_s << ", " << dst_s << ", " << src_s << std::endl;
}

void Gen_ARM::emit_add(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "add " << dst_s << ", " << dst_s << ", " << src_s << std::endl;
}

void Gen_ARM::emit_or(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "or " << dst_s << ", " << dst_s << ", " << src_s << std::endl;
}

void Gen_ARM::emit_call(std::string label) {
   os << '\t' << "bl " << label << std::endl;
}

void Gen_ARM::emit_jump(std::string label) {
   os << '\t' << "b " << label << std::endl;
}

void Gen_ARM::emit_cond_jump(std::string label, Conditional::CType condition) {
   os << '\t';
   //instruction should check the reverse case to work properly, i think
   switch (condition) {
      case Conditional::EQUAL: {
         os << "bne ";
      } break;

      case Conditional::GREATER_THAN: {
         os << "ble ";
      } break;

      case Conditional::LESS_THAN: {
         os << "bge ";
      } break;
      case Conditional::GREATER_EQUAL: {
         os << "blt ";
      } break;

      case Conditional::LESS_EQUAL: {
         os << "bgt ";
      } break;
   }
   os << label << std::endl;
}

void Gen_ARM::emit_return() {
   os << '\t' << "mov pc, " << gen_var(REG_LINK) << std::endl;
}

void Gen_ARM::emit_function_header() {
   os << '\t' << "push " << " { r4, r5, r6, r7, lr }" << std::endl;
}

void Gen_ARM::emit_function_footer() {
   os << '\t' << "pop " << " { r4, r5, r6, r7, pc }" << std::endl;
   os << '\t' << ".ltorg" << std::endl;
}

#include "Gen_ARM.h"

const Variable REG_LINK = create_register("_REG_LINK");

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

std::string  Gen_ARM::gen_var(Variable var) {

   if (var.name.compare(REGISTER_RETURN) == 0) {
      return "r0";
   } else if (var.name.compare(REGISTER_ACCUMULATOR) == 0) {
      return "r2";
   } else if (var.name.compare(REGISTER_INDEX) == 0) {
      return "r3";
   } else if (var.name.compare(REGISTER_STACK_POINTER) == 0) {
      return "r13";
   } else if (var.name.compare(REG_LINK.name) == 0) {
      return "r14";
   }

   else if (var.is_type_const && var.type == Variable::INT_32BIT) {
      return std::string("$") + std::to_string(var.pvalue);
   } else if (var.is_type_const && var.type == Variable::FLOAT_32BIT) {
      float val = var.fvalue;
      return std::string("$") + std::to_string(*(int *)&val);
   } else if (var.type == Variable::DQString) {
      // emit_call(get_new_label());
      // os << get_old_label() << ":" << std::endl;
      // emit_pop(REG_INDEX);
      // os << '\t' << "lea " << get_rodata(var) << " - " << get_old_label() << "(%ecx), %eax" << std::endl;
      // return gen_var(REG_ACCUMULATOR);
   } else {
       return stack_man.load_var(var);
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
   std::string src_s = gen_var(src);
   //os << '\t' << "push " << src_s << std::endl;
   os << '\t' << "push " << "{" << src_s << "}" << std::endl;
}

void Gen_ARM::emit_pop(Variable dst) {
   std::string dst_s = gen_var(dst);
   os << '\t' << "pop " << "{ " << dst_s << " }" << std::endl;
}

void Gen_ARM::emit_mov(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "mov " << dst_s << ", " << src_s << std::endl;
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
   // os << '\t' << "ret" << std::endl;
   os << '\t' << "mov pc, " << gen_var(REG_LINK) << std::endl;
}

void Gen_ARM::emit_function_header() {
   // emit_push(REG_LINK);
   os << "\t.code 16\n\t.thumb_func\n\t.align 2" << std::endl;
   os << '\t' << "push " << " { r4, r5, r6, r7, lr }" << std::endl;
}

void Gen_ARM::emit_function_footer() {
   os << '\t' << "pop " << " { r4, r5, r6, r7, pc }" << std::endl;
   // emit_pop(REG_LINK);
}

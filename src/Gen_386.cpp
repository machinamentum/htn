#include "Gen_386.h"

// const Variable REG_FRAME = create_register("_REG_FRAME");

void Gen_386::
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

std::string  Gen_386::gen_var(Variable var) {

   if (var.name.compare(REGISTER_RETURN) == 0) {
      return "%eax";
   } else if (var.name.compare(REGISTER_ACCUMULATOR) == 0) {
      return "%eax";
   } else if (var.name.compare(REGISTER_INDEX) == 0) {
      return "%ecx";
   } else if (var.name.compare(REGISTER_STACK_POINTER) == 0) {
      return "%esp";
   } else if (var.name.compare(REGISTER_FRAME_POINTER) == 0) {
      return "%ebp";
   }

   else if (var.is_type_const && var.type == Variable::INT_32BIT) {
      return std::string("$") + std::to_string(var.pvalue);
   } else if (var.is_type_const && var.type == Variable::FLOAT_32BIT) {
      float val = var.fvalue;
      return std::string("$") + std::to_string(*(int *)&val);
   } else if (var.type == Variable::DQString) {
      emit_call(get_new_label());
      os << get_old_label() << ":" << std::endl;
      emit_pop(REG_INDEX);
      os << '\t' << "lea " << get_rodata(var) << " - " << get_old_label() << "(%ecx), %eax" << std::endl;
      return gen_var(REG_ACCUMULATOR);
   } else {
       return stack_man->load_var(var);
   }
}

void Gen_386::emit_cmp(Variable src0, Variable src1) {
   std::string dst_s = gen_var(src1);
   //std::string src_s = gen_var(src0);
   emit_mov(src0, REG_ACCUMULATOR);
   os << '\t' << "cmp " << "%eax" << ", " << dst_s << std::endl;
}

void Gen_386::emit_inc(Variable dst) {
   emit_add(create_const_int32(1), dst);
}

void Gen_386::emit_push(Variable src) {
   std::string src_s = gen_var(src);
   os << '\t' << "push " << src_s << std::endl;
}

void Gen_386::emit_pop(Variable dst) {
   std::string dst_s = gen_var(dst);
   os << '\t' << "pop " << dst_s << std::endl;
}

void Gen_386::emit_mov(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "movl " << src_s << ", " << dst_s << std::endl;
}

void Gen_386::emit_sub(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "sub " << src_s << ", " << dst_s << std::endl;
}

void Gen_386::emit_add(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "add " << src_s << ", " << dst_s << std::endl;
}

void Gen_386::emit_or(Variable src, Variable dst) {
   std::string dst_s = gen_var(dst);
   std::string src_s = gen_var(src);
   os << '\t' << "orl " << src_s << ", " << dst_s << std::endl;
}

void Gen_386::emit_call(std::string label) {
   os << '\t' << "call " << label << std::endl;
}

void Gen_386::emit_jump(std::string label) {
   os << '\t' << "jmp " << label << std::endl;
}

void Gen_386::emit_cond_jump(std::string label, Conditional::CType condition) {
   os << '\t';
   //instruction should check the reverse case to work properly, i think
   switch (condition) {
      case Conditional::EQUAL: {
         os << "jne ";
      } break;

      case Conditional::GREATER_THAN: {
         os << "jle ";
      } break;

      case Conditional::LESS_THAN: {
         os << "jge ";
      } break;
      case Conditional::GREATER_EQUAL: {
         os << "jl ";
      } break;

      case Conditional::LESS_EQUAL: {
         os << "jg ";
      } break;
   }
   os << label << std::endl;
}

void Gen_386::emit_return() {
   os << '\t' << "ret" << std::endl;
}

void Gen_386::emit_function_header() {
   emit_push(REG_FRAME);
   emit_mov(REG_STACK, REG_FRAME);
}

void Gen_386::emit_function_footer() {
   emit_pop(REG_FRAME);
}

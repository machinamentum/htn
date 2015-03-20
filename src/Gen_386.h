
#ifndef GEN_386_H
#define GEN_386_H

#include "Code_Gen.h"

struct Gen_386 : public Code_Gen {

   Gen_386(std::ostream &ost) : Code_Gen(ost) {
   
   }
   
   std::string gen_var(Variable var);
   
   virtual void gen_rodata();

   virtual void emit_cmp(Variable src0, Variable src1);
   virtual void emit_inc(Variable dst);
   virtual void emit_push(Variable src);
   virtual void emit_pop(Variable dst);
   virtual void emit_mov(Variable src, Variable dst);
   virtual void emit_sub(Variable src, Variable dst);
   virtual void emit_add(Variable src, Variable dst);
   virtual void emit_or(Variable src, Variable dst);
   virtual void emit_call(std::string label);
   virtual void emit_jump(std::string label);
   virtual void emit_cond_jump(std::string label, Conditional::CType condition);
   virtual void emit_return();
   virtual void emit_function_header();
   virtual void emit_function_footer();
};



#endif

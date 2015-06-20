
#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <string>
#include <ostream>
#include <unordered_map>
#include <map>

#include "Code_Structure.h"

Variable create_register(std::string reg_name);
Variable create_const_int32(int value);

const std::string REGISTER_STACK_POINTER = "_REG_STACK";
const std::string REGISTER_ACCUMULATOR   = "_REGISTER_ACCUMULATOR";
const std::string REGISTER_INDEX = "_REGISTER_INDEX";
const std::string REGISTER_RETURN = "_REGISTER_RETURN";
const std::string REGISTER_FRAME_POINTER = "_REG_FRAME";

const Variable REG_STACK = create_register(REGISTER_STACK_POINTER);
const Variable REG_ACCUMULATOR = create_register(REGISTER_ACCUMULATOR);
const Variable REG_INDEX = create_register(REGISTER_INDEX);
const Variable REG_RETURN = create_register(REGISTER_RETURN);
const Variable REG_FRAME = create_register(REGISTER_FRAME_POINTER);

struct Code_Gen;

//Represents the stack for the current function's frame
//every new scope starts off with the stack pointer being 16-byte aligned
//the earliest declared variable has the highest mem addr, the latest declared has the lowest addr
//padding always sits under the variables on the stack
struct StackMan {

   std::vector<Variable> params;
   Scope *scope = nullptr;
   Code_Gen *code_gen;
   int ext_adj = 0;

   virtual std::string load_var(Variable var, Scope *ts = nullptr, int total_adjust = 0);

};

struct Var_Man {

   std::string get_var(Variable var) {
   	return std::string();//TODO implement
   }

};

struct  Code_Gen {
   std::unordered_map<Scope*, unsigned int> scope_names;
   std::vector<Variable> rodata_data;
   std::vector<std::string> rodata_labels;
   StackMan *stack_man;
   unsigned int ramp = 0;
   unsigned int scope_num = 0;
   int pb_num = 0;
   std::ostream &os;

   Code_Gen(std::ostream &ost) : os(ost) {

   }

   std::string get_new_label() {
      return std::string("L0$pb") + std::to_string(pb_num++);
   }

   std::string get_old_label() {
      return std::string("L0$pb") + std::to_string(pb_num - 1);
   }

   std::string get_rodata(Variable var) {
      std::string ref_str = "L";
      if (var.type == Variable::DQString) {
         ref_str += "str";
      }
      ref_str += std::to_string(ramp++);
      rodata_data.push_back(var);
      rodata_labels.push_back(ref_str);
      return ref_str;
   }

   unsigned int get_scope_num(Scope *scope) {
      return scope_num++;
   }

   void gen_scope(Scope &scope);
   void gen_expression(std::string scope_name, Expression &expr);
   void gen_scope_expressions(std::string scope_name, Scope &scope);
   void gen_scope_functions(Scope &scope);
   void gen_function(Function &func);
   virtual void gen_func_params(std::vector<Variable> &plist);

   virtual void gen_function_attributes(Function &func);

   virtual std::string gen_var(Variable var) = 0;

   virtual void gen_rodata() = 0;

   virtual void emit_cmp(Variable src0, Variable src1) = 0;
   virtual void emit_inc(Variable dst) = 0;
   virtual void emit_push(Variable src) = 0;
   virtual void emit_pop(Variable dst) = 0;
   virtual void emit_mov(Variable src, Variable dst) = 0;
   virtual void emit_sub(Variable src, Variable dst) = 0;
   virtual void emit_add(Variable src, Variable dst) = 0;
   virtual void emit_or(Variable src, Variable dst) = 0;
   virtual void emit_call(std::string label) = 0;
   virtual void emit_jump(std::string label) = 0;
   virtual void emit_cond_jump(std::string label, Conditional::CType condition) = 0;
   virtual void emit_return() = 0;
   virtual void emit_function_header() = 0;
   virtual void emit_function_footer() = 0;

};



#endif

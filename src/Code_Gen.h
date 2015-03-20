
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

const Variable REG_STACK = create_register(REGISTER_STACK_POINTER);
const Variable REG_ACCUMULATOR = create_register(REGISTER_ACCUMULATOR);
const Variable REG_INDEX = create_register(REGISTER_INDEX);
const Variable REG_RETURN = create_register(REGISTER_RETURN);

//Represents the stack for the current function's frame
//every new scope starts off with the stack pointer being 16-byte aligned
//the earliest declared variable has the highest mem addr, the latest declared has the lowest addr
//padding always sits under the variables on the stack
struct StackMan {

   std::vector<Variable> params;
   Scope *scope = nullptr;
   
   int ext_adj = 0;
   
   std::string load_var(Variable var, Scope *ts = nullptr, int total_adjust = 0) {
   
      if (ext_adj) {
         total_adjust = ext_adj;
         ext_adj = 0;
      }
   
      int stack_loc = 0;
      for (int i = 0; i < params.size(); ++i) {
         if (params[i].name.compare(var.name) == 0) {
            stack_loc = i * 4 + 8;// +8 accounts for push %ebp and 4 byte return address
            return std::to_string(stack_loc) + "(%ebp)";
         }
      }

      
      if (scope && !ts) {

         for (int i = 0; i < scope->variables.size(); ++i) {

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

struct Var_Man {
   
   std::string get_var(Variable var) {
   
   }
   
};

struct  Code_Gen {
   std::unordered_map<Scope*, unsigned int> scope_names;
   std::vector<Variable> rodata_data;
   std::vector<std::string> rodata_labels;
   StackMan stack_man;
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
   void gen_func_params(std::vector<Variable> &plist);
//   void gen_rodata(std::ostream &os);
   
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

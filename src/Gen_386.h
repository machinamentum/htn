
#ifndef GEN_386_H
#define GEN_386_H

#include <unordered_map>
#include <iostream>

#include "Code_Structure.h"

struct Gen_386 {

   std::unordered_map<std::string, unsigned int> variable_ram_map;
   std::unordered_map<Scope*, unsigned int> scope_names;
   std::unordered_map<Variable*, std::string> rodata;
   unsigned int ramp = 0;
   unsigned int scope_num = 0;
   const unsigned int RAM_START_ADDR = 0x180;
   
   unsigned int get_var_loc (std::string name) {
      if (!variable_ram_map.count(name)) {
         variable_ram_map[name] = ramp;
         ++ramp;
      }
      
      return  variable_ram_map[name];
   }
   
   unsigned int get_scope_num(Scope *scope) {
      return scope_num++;
   }
   
   std::string get_rodata(Variable *var) {
      if (!rodata.count(var)) {
         std::string ref_str = "L";
         if (var->type == Variable::DQString) {
            ref_str += "str";
         }
         ref_str += std::to_string(ramp++);
         rodata[var] = ref_str;
      }
      return rodata[var];
   }

   void gen_scope(Scope &scope, std::ostream &os);
   void gen_expression(std::string scope_name, Expression &expr, std::ostream &os);
   void gen_scope_expressions(std::string scope_name, Scope &scope, std::ostream &os);
   void gen_scope_functions(Scope &scope, std::ostream &os);
   void gen_function(Function &func, std::ostream &os);
   void gen_func_params(std::vector<Variable> &plist, std::ostream &os);
   void gen_rodata(std::ostream &os);
};



#endif

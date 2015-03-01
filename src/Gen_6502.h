
#ifndef GEN_6502_H
#define GEN_6502_H

#include <unordered_map>
#include <iostream>

#include "Code_Structure.h"

struct Gen_6502 {

   std::unordered_map<std::string, unsigned int> variable_ram_map;
   std::unordered_map<Scope*, unsigned int> scope_names;
   unsigned int ramp = 0;
   unsigned int scope_num = 0;
   const unsigned int RAM_START_ADDR = 0x180;
   
   unsigned int get_var_loc (std::string name) {
      if (!variable_ram_map.count(name)) {
         variable_ram_map[name] = ramp;
         ++ramp;
      }
      
      return (variable_ram_map[name] + RAM_START_ADDR);
   }
   
   unsigned int get_scope_num(Scope *scope) {
//      if (!scope_names.count(scope)) {
//         scope_names[scope] = scope_num;
//         ++scope_num;
//      }
      
//      return scope_names[scope];
      return scope_num++;
   }
   
   void gen_scope(Scope &scope, std::ostream &os);
   void gen_expression(std::string scope_name, Expression &expr, std::ostream &os);
   void gen_scope_expressions(std::string scope_name, Scope &scope, std::ostream &os);
   void gen_scope_functions(Scope &scope, std::ostream &os);
   void gen_function(Function &func, std::ostream &os);
};


#endif
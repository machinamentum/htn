
#include "Code_Structure.h"
#include "Target.h"

extern Target *target;

int Variable::get_sizeof() {
   switch (type) {
      case DQString://wat
      case VOID://wat
         return 0;
      case CHAR:
      case INT_8BIT:
         return 1;
      case INT_16BIT:
         return 2;
      case INT_32BIT:
      case FLOAT_32BIT:
         return 4;
      case POINTER:
         return target->get_pointer_size();
      case STRUCT: {
            int total = 0;
            for (Variable var : pstruct->scope->variables) {
               total += var.get_sizeof();
            }
            return total;
         }
      case INT_64BIT:
         return 8;
      case UNKNOWN:
         return -1;
   }

   return 0;
}

Function::
Function() {
   scope = new Scope();
   scope->is_function = true;
   scope->function = this;
}

Struct::
Struct() {
   scope = new Scope();
   scope->is_struct = true;
   scope->_struct = this;
}

Expression::
Expression() {
   scope = new Scope();
}


Scope::
Scope(Scope *p) {
   parent = p;
}

bool Scope::
contains_symbol(const std::string name) {
   for (auto& f : functions) {
      if (f.name.compare(name) == 0) {
         return true;
      }
   }

   for (auto &f : variables) {
      if (f.name.compare(name) == 0) {
         return true;
      }
   }

   return (parent ? parent->contains_symbol(name) : false);
}

Struct *Scope::
getStructByName(const std::string name) {
   for (auto &s : structs) {
      if (s.name.compare(name) == 0) {
         return &s;
      }
   }

   return (parent ? parent->getStructByName(name) : nullptr);
}

Function *Scope::
getFuncByName(const std::string name) {
   for (auto &f : functions) {
      if (f.name.compare(name) == 0) {
         return &f;
      }
   }

   return (parent ? parent->getFuncByName(name) : nullptr);
}

Variable *Scope::
getVarByName(const std::string name) {
   for (auto &f : variables) {
      if (f.name.compare(name) == 0) {
         return &f;
      }
   }
   for (auto &func : functions) {
      for (auto &f : func.parameters) {
         if (f.name.compare(name) == 0) {
            return &f;
         }
      }
   }

   if (is_function) {
      for (auto &f : function->parameters) {
         if (f.name.compare(name) == 0) {
            return &f;
         }
      }
   }

   return (parent ? parent->getVarByName(name) : nullptr);
}

bool Scope::
empty() {
   return !functions.size() && !expressions.size();
}


#ifndef CODE_STRUCTURE_H
#define CODE_STRUCTURE_H

#include <string>
#include <vector>
#include <cstdint>

struct Struct;

struct Variable {
   enum VType {
      DQString,
      VOID,
      CHAR,
      INT_8BIT,
      INT_16BIT,
      INT_32BIT,
      INT_64BIT,
      FLOAT_32BIT,
      POINTER,
      DEREFERENCED_POINTER,//this is hacky and shouldnt be a type :P
      STRUCT,
      UNKNOWN
   };

   std::string name;
   std::string dqstring;
   Struct *pstruct = nullptr;
   VType type;
   VType ptype; //used when type is a pointer
   intptr_t pvalue;
   float fvalue;
   bool is_type_const = false;
   bool is_ptype_const = false;

   int get_sizeof();
};

struct Scope;

struct Function {
   std::string name;
   std::vector<Variable> parameters;
   Scope *scope = nullptr;
   bool should_inline = false;
   bool plain_instructions = false;
   bool is_not_definition = false;
   Variable return_info;
   Function();
};

struct Conditional {
   enum CType {
      EQUAL,
      GREATER_THAN,
      LESS_THAN,
      GREATER_EQUAL,
      LESS_EQUAL
   };

   Variable left;
   CType condition;
   Variable right;
   bool is_always_true = false;
};

struct Instruction {
   enum IType {
      FUNC_CALL,
      SUBROUTINE_JUMP, //used mainly for loops
      ASSIGN,
      INCREMENT,
      BIT_OR
   };

   IType type;
   std::string func_call_name;
   std::vector<Variable> call_target_params;
   Variable lvalue_data;
   Variable rvalue_data;
   bool is_conditional_jump = false;
   Conditional condition;
};

struct Struct {
   std::string name;
   Scope *scope = nullptr;
   Struct();
   //NOTE function members can just be added to the parent scope with mangled name
};

struct Expression {
   std::vector<Instruction> instructions;

   /**
    * holds the variable data of the destination variable or
    * (in the case of a return) the data of a temporary, or const,
    * that the expression will evaluate to.
    */
   Variable return_value;
   Scope *scope = nullptr;

   Expression();
};

struct Scope {

   std::vector<Function> functions;
   std::vector<Expression> expressions;
   std::vector<Variable> variables;
   std::vector<Struct> structs;
   Scope *parent = nullptr;
   bool is_function = false;
   Function *function;
   bool is_struct = false;
   Struct *_struct;

   Scope(Scope *p = nullptr);

   bool contains_symbol(const std::string name);
   Struct *getStructByName(const std::string name);
   Function *getFuncByName(const std::string name);
   Variable *getVarByName(const std::string name);
   bool empty();
};




#endif

#define STB_C_LEXER_IMPLEMENTATION
#include "stb_c_lexer.h"


#include <cstdio>

static void print_token(stb_lexer *lexer)
{
   switch (lexer->token) {
      case CLEX_id        : printf("_%s", lexer->string); break;
      case CLEX_eq        : printf("=="); break;
      case CLEX_noteq     : printf("!="); break;
      case CLEX_lesseq    : printf("<="); break;
      case CLEX_greatereq : printf(">="); break;
      case CLEX_andand    : printf("&&"); break;
      case CLEX_oror      : printf("||"); break;
      case CLEX_shl       : printf("<<"); break;
      case CLEX_shr       : printf(">>"); break;
      case CLEX_plusplus  : printf("++"); break;
      case CLEX_minusminus: printf("--"); break;
      case CLEX_arrow     : printf("->"); break;
      case CLEX_andeq     : printf("&="); break;
      case CLEX_oreq      : printf("|="); break;
      case CLEX_xoreq     : printf("^="); break;
      case CLEX_pluseq    : printf("+="); break;
      case CLEX_minuseq   : printf("-="); break;
      case CLEX_muleq     : printf("*="); break;
      case CLEX_diveq     : printf("/="); break;
      case CLEX_modeq     : printf("%%="); break;
      case CLEX_shleq     : printf("<<="); break;
      case CLEX_shreq     : printf(">>="); break;
         //case CLEX_eqarrow   : printf("=>"); break;
      case CLEX_dqstring  : printf("\"%s\"", lexer->string); break;
         //case CLEX_sqstring  : printf("'\"%s\"'", lexer->string); break;
      case CLEX_charlit   : printf("'%s'", lexer->string); break;
#if defined(STB__clex_int_as_double) && !defined(STB__CLEX_use_stdlib)
      case CLEX_intlit    : printf("#%g", lexer->real_number); break;
#else
      case CLEX_intlit    : printf("#%ld", lexer->int_number); break;
#endif
      case CLEX_floatlit  : printf("%g", lexer->real_number); break;
      default:
         if (lexer->token >= 0 && lexer->token < 256)
            printf("%c", (int) lexer->token);
         else {
            printf("<<<UNKNOWN TOKEN %ld >>>\n", lexer->token);
         }
         break;
   }
}

#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <cstdint>

struct Variable {
   enum VType {
      DQString,
      VOID,
      POINTER,
      DEREFERENCED_POINTER
   };
   
   std::string name;
   std::string dqstring;
   VType type;
   VType ptype; //used when type is a pointer
   intptr_t pvalue;
};

struct Scope;

struct Function {
   std::string name;
   std::vector<Variable> parameters;
   Scope *scope;
   
   Function();
};

struct Instruction {
   enum IType {
      FUNC_CALL,
      ASSIGN
   };
   
   IType type;
   std::string func_call_name;
   std::vector<Variable> call_target_params;
   Variable lvalue_data;
   Variable rvalue_data;
};

struct Expression {
   std::vector<Instruction> instructions;
};

struct Scope {
   
   std::vector<Function> functions;
   std::vector<Expression> expressions;
   std::vector<Variable> variables;
   std::vector<Scope *> children;
   Scope *parent;
   
   Scope(Scope *p = nullptr) {
      parent = p;
   }

   bool contains_symbol(const std::string name) {
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
   
   Function *getFuncByName(const std::string name) {
      for (auto &f : functions) {
         if (f.name.compare(name) == 0) {
            return &f;
         }
      }
      
      return (parent ? parent->getFuncByName(name) : nullptr);
   }
   
   Variable *getVarByName(const std::string name) {
      for (auto &f : variables) {
         if (f.name.compare(name) == 0) {
            return &f;
         }
      }
      
      return (parent ? parent->getVarByName(name) : nullptr);
   }

};

Function::
Function() {
      scope = new Scope();
}

static void parse_scope(std::string name, Scope &parent, stb_lexer &lex, long delim_token);
static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex);
static void parse_function(std::string name, Scope &scope, stb_lexer &lex);
static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex);
static void parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref);

static void parse_scope(std::string name, Scope &scope, stb_lexer &lex, long delim_token = 0) {

   bool deref = false;

   while (stb_c_lexer_get_token(&lex)) {
      if (lex.token == delim_token) {
         break;
      }
      if (lex.token == '*') {
         deref = true;
      } else if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         if (!scope.contains_symbol(name)) {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               if (deref) {
                  printf("Error: attempt to dereference variable in declaration.\n");
               } else {
                  parse_declaration(name, scope, lex);
               }
            }
         } else {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               printf("Error: identifier already declared\n");
            } else {
               //printf("parsing expression\n");
               parse_expression(name, scope, lex, deref);
               deref = false;
            }
         }
      } else {
         print_token(&lex);
         printf("  ");
      }
   }
}

static Variable::VType get_vtype(std::string t) {
   if (t.compare("void") == 0) {
      return Variable::VOID;
   }
}


static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex) {
   Variable var;
   var.name = name;
   bool is_pointer = false;
   while (lex.token != ';') {
      if (lex.token == '*') {
         is_pointer = true;
         var.type = Variable::POINTER;
      } else if (lex.token == CLEX_id) {
         std::string vtype = std::string(lex.string, strlen(lex.string));
         if (is_pointer) {
            var.ptype = get_vtype(vtype);
         } else {
            var.type = get_vtype(vtype);
         }
      }
      
      stb_c_lexer_get_token(&lex);
   }
   
   return var;
}

static Variable parse_rvalue(Scope &scope, stb_lexer &lex) {
   Variable var;
   stb_c_lexer_get_token(&lex);
   while (lex.token != ';') {
      if (lex.token == CLEX_intlit) {
         var.pvalue = lex.int_number;
      }
      stb_c_lexer_get_token(&lex);
   }
   
   return var;
}

static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if(lex.token == '(') {
      parse_function(name, scope, lex);
   } else {
      printf("Parse var: %s\n", name.c_str());
      scope.variables.push_back(parse_variable(name, scope, lex));
   }
}

static std::vector<Variable> parse_parameter_list(Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   std::vector<Variable> plist;
   while (lex.token != ')') {
      Variable var;
      
      if (lex.token == CLEX_dqstring) {
         var.type = Variable::DQString;
         var.dqstring = std::string(lex.string, strlen(lex.string));
         plist.push_back(var);
      }
      
      stb_c_lexer_get_token(&lex);
   }
   
   return plist;
}


static void parse_function(std::string name, Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   Function func;
   func.name = name;
   printf("Parsing function: %s\n", name.c_str());
   func.scope->parent = &scope;
   if (lex.token != ')') {
      func.parameters = parse_parameter_list(*func.scope, lex);
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != CLEX_arrow) {
      printf("Error: expected ->\n");
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != CLEX_id) {
      printf("Error: unexpected token\n");
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != '{') {
      printf("Error: unexpected token\n");
   }
   
   parse_scope(name, *func.scope, lex, '}');
   scope.functions.push_back(func);
}




static void parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref) {
   Expression expr;
   Instruction instr;
   
   print_token(&lex);
   printf("\n");
   
   if (lex.token == '(') {
      //printf("parsing function call\n");
      instr.type = Instruction::FUNC_CALL;
      instr.func_call_name = name;
      instr.call_target_params = parse_parameter_list(scope, lex);
      
      stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         printf("Error: unexpected token");
      }
      expr.instructions.push_back(instr);
   }
   /*else if (lex.token == ':') {
      scope.variables.push_back(parse_variable(name, scope, lex));
   }*/
   else if (lex.token == '=') {
      printf("parsing assignment\n");
      instr.type = Instruction::ASSIGN;
      Variable *var = scope.getVarByName(name);
      if (var) {
         instr.lvalue_data = *var;
      } else {
         printf("Error: undefined reference to %s\n", name.c_str());
      }
      instr.lvalue_data.type = (deref ? Variable::DEREFERENCED_POINTER : Variable::POINTER);
      instr.rvalue_data = parse_rvalue(scope, lex);
      //stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         printf("Error: unexpected token");
      }
      expr.instructions.push_back(instr);
   }
   scope.expressions.push_back(expr);
}

static Function asmInlineFunc() {
   Function func;
   func.name = "__asm__";
   
   Variable dqs;
   dqs.type = Variable::DQString;
   func.parameters.push_back(dqs);
   return func;
}

static Scope parse(std::string src) {
   const char *text = src.c_str();
   int len = strlen(text);
   stb_lexer lex;
   stb_c_lexer_init(&lex, text, text+len, (char *) malloc(1<<16), 1<<16);
   Scope globalScope;
//   Function print_func;
//   print_func.name = "print";
//   Variable dqs;
//   dqs.type = Variable::DQString;
//   print_func.parameters.push_back(dqs);
//   globalScope.functions.push_back(print_func);
   globalScope.functions.push_back(asmInlineFunc());
   parse_scope(std::string(), globalScope, lex, CLEX_eof);
   return globalScope;
}


static std::string load_file(const std::string pathname) {
   std::ifstream t(pathname);
   std::string str;
   
   t.seekg(0, std::ios::end);
   str.reserve(t.tellg());
   t.seekg(0, std::ios::beg);
   
   str.assign((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
   return str;
}

#include <iostream>

static void generate_6502_scope(Scope &scope, std::ostream &os) {
   for (auto &func : scope.functions) {
      generate_6502_scope(*func.scope, os);
      if (func.name.compare("__asm__") == 0) continue;
      os << func.name << ":" << std::endl;
      for (auto &expr : func.scope->expressions) {
         for (auto &instr : expr.instructions) {
            switch (instr.type) {
               case Instruction::FUNC_CALL: {
                  if (instr.func_call_name.compare("__asm__") == 0) {
                     if (instr.call_target_params[0].dqstring.find_first_of(':') == std::string::npos) {
                        os << '\t';
                     }
                     os << instr.call_target_params[0].dqstring << std::endl;
                  } else {
                     Function *cfunc = func.scope->getFuncByName(instr.func_call_name);
                     if (!cfunc) {
                        printf("Undefined reference to %s\n", instr.func_call_name.c_str());
                     } else {
                        os << '\t' << "jsr " << cfunc->name << std::endl;
                     }
                  }
               } break;
               case Instruction::ASSIGN: {
                  printf("Assignment\n");
                  if (instr.lvalue_data.type == Variable::POINTER) {
                     os << '\t' << "ldy #" << instr.rvalue_data.pvalue << std::endl;
                  } else if (instr.lvalue_data.type == Variable::DEREFERENCED_POINTER) {
                     os << '\t' << "pha" << std::endl;
                     os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                     os << '\t' << "sta 0,y" << std::endl;
                     os << '\t' << "pla" << std::endl;
                  }
               } break;
            }
         }
      }
      os << '\t' << "rts" << std::endl;
   }
}

static void generate_6502(Scope &scope, std::ostream &os) {
   os << '\t' << "processor 6502" << std::endl;
   os << '\t' << "SEG" << std::endl;
   os << '\t' << "ORG $F000" << std::endl;
   generate_6502_scope(scope, os);
   os << '\t' << "ORG $FFFA" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << "END" << std::endl;
}

#include <fstream>

int main(int argc, char** argv) {
   std::string source = load_file("test.htn");
   Scope scope = parse(source);
   std::ofstream ofs("test.s");
   generate_6502(scope, ofs);
   //interpret(scope);
   return 0;
}



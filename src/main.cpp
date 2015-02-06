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

struct Variable {
   enum VType {
      DQString
   };
   std::string dqstring;
   VType type;
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
      FUNC_CALL
   };
   
   IType type;
   std::string func_call_name;
   std::vector<Variable> call_target_params;
};

struct Expression {
   std::vector<Instruction> instructions;
};

struct Scope {
   
   std::vector<Function> functions;
   std::vector<Expression> expressions;
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

};

Function::
Function() {
      scope = new Scope();
}

static void parse_scope(std::string name, Scope &parent, stb_lexer &lex, long delim_token);
static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex);
static void parse_function(std::string name, Scope &scope, stb_lexer &lex);
static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex);
static void parse_expression(std::string name, Scope &scope, stb_lexer &lex);

static void parse_scope(std::string name, Scope &scope, stb_lexer &lex, long delim_token = 0) {
   while (stb_c_lexer_get_token(&lex)) {
      if (lex.token == delim_token) {
         break;
      }
      if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         if (!scope.contains_symbol(name)) {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               parse_declaration(name, scope, lex);
            }
         } else {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               printf("Error: identifier already declared\n");
            } else {
               printf("parsing expression\n");
               parse_expression(name, scope, lex);
            }
         }
      } else {
         print_token(&lex);
         printf("  ");
      }
   }
}

static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if(lex.token == '(') {
      parse_function(name, scope, lex);
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

static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex) {

}


static void parse_expression(std::string name, Scope &scope, stb_lexer &lex) {
   Expression expr;
   Instruction instr;
   if (lex.token == '(') {
      printf("parsing function call\n");
      instr.type = Instruction::FUNC_CALL;
      instr.func_call_name = name;
      instr.call_target_params = parse_parameter_list(scope, lex);
      
      stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         printf("Error: unexpected token");
      }
      expr.instructions.push_back(instr);
   }
   scope.expressions.push_back(expr);
}

static Scope parse(std::string src) {
   const char *text = src.c_str();
   int len = strlen(text);
   stb_lexer lex;
   stb_c_lexer_init(&lex, text, text+len, (char *) malloc(1<<16), 1<<16);
   Scope globalScope;
   Function print_func;
   print_func.name = "print";
   Variable dqs;
   dqs.type = Variable::DQString;
   print_func.parameters.push_back(dqs);
   globalScope.functions.push_back(print_func);
   parse_scope(std::string(), globalScope, lex, CLEX_eof);
   return globalScope;
}

static void print(std::string dqstring) {
   printf("%s", dqstring.c_str());
}

static void interpret_func(Function *func, std::vector<Variable> plist) {
   for (auto &expr : func->scope->expressions) {
      for (auto &instr : expr.instructions) {
         switch (instr.type) {
            case Instruction::FUNC_CALL: {
               //printf("Func call name %s\n", instr.func_call_name.c_str());
               if (instr.func_call_name.compare("print") == 0) {
//                  printf("calling print\n");
                  print(instr.call_target_params[0].dqstring);
               } else {
                  Function *cfunc = func->scope->getFuncByName(instr.func_call_name);
                  if (!cfunc) {
                     printf("Undefined reference to %s\n", instr.func_call_name.c_str());
                  } else {
                     interpret_func(cfunc, instr.call_target_params);
                  }
               }
            } break;
         }
      }
   }
}

static void interpret(Scope &scope) {
   Function *main_f = scope.getFuncByName("main");
   if (!main_f) {
      printf("Undefined reference to main\n");
   }
   
//   printf("Expressions: %d\n", main_f->scope->expressions.size());
   interpret_func(main_f, std::vector<Variable>());
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

int main(int argc, char** argv) {
   std::string source = load_file("test.htn");
   Scope scope = parse(source);
   interpret(scope);
   return 0;
}



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
   bool is_type_const = false;
   bool is_ptype_const = false;
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
   Scope *scope;
   
   Expression();
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

Expression::
Expression() {
   scope = new Scope();
}

static void parse_scope(std::string name, Scope &parent, stb_lexer &lex, long delim_token);
static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex);
static void parse_function(std::string name, Scope &scope, stb_lexer &lex);
static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex);
static void parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref);
static Variable parse_rvalue(Scope &scope, stb_lexer &lex);
static void compiler_error(std::string msg, stb_lexer &lex);
static void compiler_warning(std::string msg, stb_lexer &lex);


std::string token_to_string(stb_lexer &lex) {
   switch (lex.token) {
      case CLEX_id        : return std::string(lex.string, strlen(lex.string));
      case CLEX_eq        : return "==";
      case CLEX_noteq     : return "!=";
      case CLEX_lesseq    : return "<=";
      case CLEX_greatereq : return ">=";
      case CLEX_andand    : return "&&";
      case CLEX_oror      : return "||";
      case CLEX_shl       : return "<<";
      case CLEX_shr       : return ">>";
      case CLEX_plusplus  : return "++";
      case CLEX_minusminus: return "--";
      case CLEX_arrow     : return "->";
      case CLEX_andeq     : return "&=";
      case CLEX_oreq      : return "|=";
      case CLEX_xoreq     : return "^=";
      case CLEX_pluseq    : return "+=";
      case CLEX_minuseq   : return "-=";
      case CLEX_muleq     : return "*=";
      case CLEX_diveq     : return "/=";
      case CLEX_modeq     : return "%%=";
      case CLEX_shleq     : return "<<=";
      case CLEX_shreq     : return ">>=";
         //case CLEX_eqarrow   : printf("=>"); break;
      case CLEX_dqstring  : return std::string(lex.string, strlen(lex.string));
         //case CLEX_sqstring  : printf("'\"%s\"'", lexer->string); break;
      case CLEX_charlit   : return "'" + std::string(lex.string, strlen(lex.string)) + "'";
#if defined(STB__clex_int_as_double) && !defined(STB__CLEX_use_stdlib)
      //case CLEX_intlit    : printf("#%g", lexer->real_number); break;
#else
     // case CLEX_intlit    : printf("#%ld", lexer->int_number); break;
#endif
     // case CLEX_floatlit  : printf("%g", lexer->real_number); break;
      default:
         if (lex.token >= 0 && lex.token < 256)
            return std::string() + (char)lex.token;
         else {
            //printf("<<<UNKNOWN TOKEN %ld >>>\n", lexer->token);
         }
         break;
   }
}

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
                  compiler_error("attempt to dereference variable in declaration.", lex);
               } else {
                  parse_declaration(name, scope, lex);
               }
            }
         } else {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               compiler_error("identifier already declared.", lex);
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
   bool push = true;
   while (lex.token != ';') {
      if (lex.token == '*') {
         is_pointer = true;
         var.type = Variable::POINTER;
      } else if (lex.token == CLEX_id) {
         std::string vtype = std::string(lex.string, strlen(lex.string));
         if (vtype.compare("const") == 0) {
            if (is_pointer) {
               var.is_ptype_const = true;
            } else {
               var.is_type_const = true;
            }
         } else if (is_pointer) {
            var.ptype = get_vtype(vtype);
         } else {
            var.type = get_vtype(vtype);
         }
      } else if (lex.token == '=') {
         if (var.is_type_const) {
            var.pvalue = parse_rvalue(scope, lex).pvalue;
            break;
         } else {
            scope.variables.push_back(var);
            push = false;
            
            parse_expression(name, scope, lex, false);
            break;
         }
      }
      
      stb_c_lexer_get_token(&lex);
   }
   
   if (push) {
      scope.variables.push_back(var);
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
      //printf("Parse var: %s\n", name.c_str());
      parse_variable(name, scope, lex);
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
   //printf("Parsing function: %s\n", name.c_str());
   func.scope->parent = &scope;
   if (lex.token != ')') {
      func.parameters = parse_parameter_list(*func.scope, lex);
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != CLEX_arrow) {
      compiler_error(std::string("expected token '->' before token '") + token_to_string(lex) + "'", lex);
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != CLEX_id) {
      compiler_error(std::string("unexpected token '") + token_to_string(lex) + "'", lex);
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != '{') {
      compiler_error(std::string("unexpected token '") + token_to_string(lex)+ "'", lex);
   }
   
   parse_scope(name, *func.scope, lex, '}');
   scope.functions.push_back(func);
}




static void parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref) {
   Expression expr;
   Instruction instr;
   
//   print_token(&lex);
//   printf("\n");
   
   if (lex.token == '(') {
      //printf("parsing function call\n");
      instr.type = Instruction::FUNC_CALL;
      instr.func_call_name = name;
      instr.call_target_params = parse_parameter_list(scope, lex);
      
      stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
      }
      expr.instructions.push_back(instr);
   }
   /*else if (lex.token == ':') {
      scope.variables.push_back(parse_variable(name, scope, lex));
   }*/
   else if (lex.token == '=') {
      //printf("parsing assignment\n");
      instr.type = Instruction::ASSIGN;
      Variable *var = scope.getVarByName(name);
      if (var) {
         if (var->is_type_const && !deref) {
            compiler_error("read-only variable is not assignable", lex);
         }
         instr.lvalue_data = *var;
      } else {
         compiler_error("Error: undefined reference to " + name, lex);
      }
      instr.lvalue_data.type = (deref ? Variable::DEREFERENCED_POINTER : Variable::POINTER);
      instr.rvalue_data = parse_rvalue(scope, lex);
      //stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
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
#include <unordered_map>

struct Gen_6502 {

   std::unordered_map<std::string, unsigned int> variable_ram_map;
   unsigned int ramp = 0;
   const unsigned int RAM_START_ADDR = 0x180;
   
   unsigned int get_var_loc (std::string name) {
      if (!variable_ram_map.count(name)) {
         variable_ram_map[name] = ramp;
         ++ramp;
      }
      
      return (variable_ram_map[name] + RAM_START_ADDR);
   }
   
   void gen_scope(Scope &scope, std::ostream &os);
};

void Gen_6502::
gen_scope(Scope &scope, std::ostream &os) {
   for (auto &func : scope.functions) {
      gen_scope(*func.scope, os);
      variable_ram_map.clear();
      if (func.name.compare("__asm__") == 0) continue;
      os << func.name << ":" << std::endl;
      
      Instruction prevInstr;
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
                  if (instr.lvalue_data.type == Variable::POINTER) {
                     os << '\t' << "ldy #" << instr.rvalue_data.pvalue << std::endl;
                     os << '\t' << "sty " << get_var_loc(instr.lvalue_data.name) << std::endl;
                  } else if (instr.lvalue_data.type == Variable::DEREFERENCED_POINTER) {
                     if (!(prevInstr.type == Instruction::ASSIGN
                           && prevInstr.lvalue_data.type == Variable::DEREFERENCED_POINTER
                           && prevInstr.lvalue_data.name.compare(instr.lvalue_data.name) == 0
                           && prevInstr.rvalue_data.pvalue == instr.rvalue_data.pvalue)) {
                        if (!instr.lvalue_data.is_type_const) {
                           os << '\t' << "ldy " << get_var_loc(instr.lvalue_data.name) << std::endl;
                        }
                        os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                     }
                     if (instr.lvalue_data.is_type_const) {
                        os << '\t' << "sta " << instr.lvalue_data.pvalue << std::endl;
                     } else {
                        os << '\t' << "sta 0,y" << std::endl;
                     }
                  }
               } break;
            }
            
            prevInstr = instr;
         }
      }
      os << '\t' << "rts" << std::endl;
   }
}


static void generate_6502(Scope &scope, std::ostream &os) {
   os << '\t' << "processor 6502" << std::endl;
   os << '\t' << "SEG" << std::endl;
   os << '\t' << "ORG $F000" << std::endl;
   Gen_6502 g6502;
   g6502.gen_scope(scope, os);
   os << '\t' << "ORG $FFFA" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << ".word main" << std::endl;
   os << '\t' << "END" << std::endl;
}

#include <fstream>

#include <string>
#include <iostream>
#include <stdio.h>
#include <sstream>

static int error_count = 0;
static std::string csource;

static void print_line_with_arrow(int line_number, int offset) {
   std::string line;
   std::stringstream ss(csource);
   for (int i = 0; i < line_number; i++) {
      getline(ss, line);
   }
   std::cout << line << std::endl;
   for (int i = 0; i < offset; i++) {
      std::cout << " ";
   }
   
   std::cout << "^" << std::endl;
}

static void compiler_warning(std::string msg, stb_lexer &lex) {
   stb_lex_location loc;
	stb_c_lexer_get_location(&lex, lex.where_firstchar, &loc);
	std::cout << "test.htn:" << loc.line_number << ":" << loc.line_offset << ": warning: " << msg << std::endl;
   print_line_with_arrow(loc.line_number, loc.line_offset);
}

static void compiler_error(std::string msg, stb_lexer &lex) {
   stb_lex_location loc;
	stb_c_lexer_get_location(&lex, lex.where_firstchar, &loc);
	std::cout << "test.htn:" << loc.line_number << ":" << loc.line_offset << ": error: " << msg << std::endl;
   print_line_with_arrow(loc.line_number, loc.line_offset);
   error_count++;
   if (error_count > 5) {
      exit(-1);
   }
}

std::string exec(const char* cmd) {
    FILE* pipe = popen(cmd, "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
    	if(fgets(buffer, 128, pipe) != NULL)
    		result += buffer;
    }
    pclose(pipe);
    return result;
}

int main(int argc, char** argv) {
   std::string source = load_file("test.htn");
   csource = source;
   Scope scope = parse(source);
   if (error_count) {
      return -1;
   }
   std::ofstream ofs("test.s");
   generate_6502(scope, ofs);
   std::cout << exec("dasm test.s -f3 -v4 -otest.bin") << std::endl;
   return 0;
}



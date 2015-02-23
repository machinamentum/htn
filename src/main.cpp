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
      INT_8BIT,
      INT_16BIT,
      INT_32BIT,
      INT_64BIT,
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
   bool should_inline = false;
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
      INCREMENT
   };
   
   IType type;
   std::string func_call_name;
   std::vector<Variable> call_target_params;
   Variable lvalue_data;
   Variable rvalue_data;
   bool is_conditional_jump = false;
   Conditional condition;
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

   bool empty() {
      return !functions.size() && !expressions.size() && !children.size();
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

static void parse_scope(std::string name, Scope &parent, stb_lexer &lex, long delim_token = 0);
static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex);
static void parse_function(std::string name, Scope &scope, stb_lexer &lex, bool should_inline = false);
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

static Conditional parse_conditional(Scope &scope, stb_lexer &lex) {
   Conditional cond;
   stb_c_lexer_get_token(&lex);
   if (lex.token == CLEX_id) {
      std::string name = std::string(lex.string, strlen(lex.string));
      Variable *lvar = scope.getVarByName(name);
      if (lvar) {
         cond.left = *lvar;
      } else if (name.compare("true") == 0) {
         cond.is_always_true = true;
         stb_c_lexer_get_token(&lex);
         if (lex.token != ')') {
            compiler_error(std::string("Conditional laziness error"), lex);
         }
         return cond;
      }
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token == '<') {
      cond.condition = Conditional::LESS_THAN;
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token == CLEX_intlit) {
      Variable var;
      var.pvalue = lex.int_number;
      var.is_type_const = true;
      cond.right = var;
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != ')') {
      compiler_error(std::string("expected token ')' before token '") + token_to_string(lex) + "'", lex);
   }
   
   return cond;
}

static void parse_while_loop(Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if (lex.token != '(') {
       compiler_error(std::string("expected token '(' before token '") + token_to_string(lex) + "'", lex);
   }
   
   Expression expr;
   Conditional cond = parse_conditional(scope, lex);
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != '{') {
      compiler_error(std::string("expected token '{' before token '") + token_to_string(lex) + "'", lex);
   }
   expr.scope->parent = &scope;
   parse_scope(std::string(), *expr.scope, lex, '}');
   Expression jump_back;
   Instruction instr;
   instr.type = Instruction::SUBROUTINE_JUMP;
   instr.is_conditional_jump = true;
   instr.condition = cond;
   instr.func_call_name = "SOS_JUMP"; //Start-Of-Scope
   jump_back.instructions.push_back(instr);
   expr.scope->expressions.push_back(jump_back);
   scope.expressions.push_back(expr);
}

static void parse_preincrement(Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if (lex.token == CLEX_id) {
      std::string name = std::string(lex.string, strlen(lex.string));
      Variable *var = scope.getVarByName(name);
      if (!var) {
         compiler_error(std::string("use of undeclared identifier '") + token_to_string(lex) + "'", lex);
      } else {
         Expression expr;
         expr.scope->parent = &scope;
         Instruction instr;
         instr.type = Instruction::INCREMENT;
         instr.lvalue_data = *var;
         expr.instructions.push_back(instr);
         scope.expressions.push_back(expr);
      }
   } else {
      compiler_error(std::string("expected an identifier before token '") + token_to_string(lex) + "'", lex);
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != ';') {
      compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
   }
   
}

static void parse_scope(std::string name_str, Scope &scope, stb_lexer &lex, long delim_token) {

   bool deref = false;

   while (stb_c_lexer_get_token(&lex)) {
      if (lex.token == delim_token) {
         break;
      }
      if (lex.token == '*') {
         deref = true;
      } else if (lex.token == CLEX_plusplus) {
         parse_preincrement(scope, lex);
      } else if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         if (name.compare("while") == 0) {
            parse_while_loop(scope, lex);
         } else if (!scope.contains_symbol(name)) {
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
   } else if (t.compare("tiny") == 0) {
      return Variable::INT_8BIT;
   } else if (t.compare("small") == 0) {
      return Variable::INT_16BIT;
   } else if (t.compare("int") == 0) {
      return Variable::INT_32BIT;
   } else if (t.compare("big") == 0) {
      return Variable::INT_64BIT;
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
         var.is_type_const = true;
      } else if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         Variable *rvar = scope.getVarByName(name);
         if (!rvar) {
            compiler_error(std::string("use of undeclared identifier '") + token_to_string(lex) + "'", lex);
         } else {
            var = *rvar;
         }
      }
      stb_c_lexer_get_token(&lex);
   }
   
   return var;
}

static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if(lex.token == '(') {
      parse_function(name, scope, lex);
   } else if(lex.token == CLEX_id) {
      std::string tname = std::string(lex.string, strlen(lex.string));
      if (tname.compare("inline") == 0) {
          stb_c_lexer_get_token(&lex);
         if(lex.token == '(') {
            parse_function(name, scope, lex, true);
         }
      } else {
         parse_variable(name, scope, lex);
      }
   } else {
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
      } else if (lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         Variable *var_ref = scope.getVarByName(name);
         
         if (var_ref) {
            plist.push_back(*var_ref);
         } else {
            compiler_error(std::string("use of undeclared identifier '") + name + "'", lex);
         }
      }
      
      stb_c_lexer_get_token(&lex);
   }
   
   return plist;
}


static void parse_function(std::string name, Scope &scope, stb_lexer &lex, bool should_inline) {
   stb_c_lexer_get_token(&lex);
   Function func;
   func.should_inline = should_inline;
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
   expr.scope->parent = &scope;
   Instruction instr;
   
//   print_token(&lex);
//   printf("\n");
   
   if (lex.token == '(') {
      Function *tfunc = scope.getFuncByName(name);
      if (tfunc) {
         if (tfunc->should_inline) {
            parse_parameter_list(scope, lex); //TODO(josh)
            for (Expression &expr : tfunc->scope->expressions) {
               scope.expressions.push_back(expr);
            }
            stb_c_lexer_get_token(&lex);
            if (lex.token != ';') {
               compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
            }
         } else {
            instr.type = Instruction::FUNC_CALL;
            instr.func_call_name = name;
            instr.call_target_params = parse_parameter_list(scope, lex);
            
            stb_c_lexer_get_token(&lex);
            if (lex.token != ';') {
               compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
            }
            expr.instructions.push_back(instr);
         }
      } else {
         compiler_error(std::string("use of undeclared identifier '") + name + "'", lex);
      }
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

static std::vector<char> load_bin_file(const std::string pathname) {
   std::ifstream t(pathname);
   std::vector<char> str;
   
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

void Gen_6502::
gen_expression(std::string scope_name, Expression &expr, std::ostream &os) {
   Instruction prevInstr;
   for (auto &instr : expr.instructions) {
      switch (instr.type) {
         case Instruction::INCREMENT: {
            printf("increment\n");
            os << '\t' << "inc " << get_var_loc(instr.lvalue_data.name) << std::endl;
         } break;
         case Instruction::SUBROUTINE_JUMP: {
            printf("sub jump\n");
            if (instr.func_call_name.compare("SOS_JUMP") == 0) {
               if (instr.is_conditional_jump && !instr.condition.is_always_true) {
                  os << '\t' << "lda #" << instr.condition.right.pvalue << std::endl;
                  os << '\t' << "cmp " << get_var_loc(instr.condition.left.name) << std::endl;
                  switch (instr.condition.condition) {
                     case Conditional::LESS_THAN: {
                        os << '\t' << "bcs " << scope_name << std::endl;
                     } break;
                  }
               } else {
                  os << '\t' << "jmp " << scope_name << std::endl;
               }
            }
         } break;
         case Instruction::FUNC_CALL: {
            if (instr.func_call_name.compare("__asm__") == 0) {
               if (instr.call_target_params[0].dqstring.find_first_of(':') == std::string::npos) {
                  os << '\t';
               }
               std::string final = instr.call_target_params[0].dqstring;
               while (final.find_first_of("@") != std::string::npos) {
                  if (final.find_first_of("@0") != std::string::npos) {
                     final.replace(final.find_first_of("@0"), 2, std::to_string(get_var_loc(instr.call_target_params[1].name)));
                  }
               }
               os << final << std::endl;
            } else {
               Function *cfunc = expr.scope->getFuncByName(instr.func_call_name);
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
                  if (instr.rvalue_data.is_type_const) {
                     os << '\t' << "lda #" << instr.rvalue_data.pvalue << std::endl;
                  } else {
                     os << '\t' << "lda " << get_var_loc(instr.rvalue_data.name) << std::endl;
                  }
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

void Gen_6502::
gen_scope_functions(Scope &scope, std::ostream &os) {
    for (auto &func : scope.functions) {
      gen_function(func, os);
   }
}

void Gen_6502::
gen_scope_expressions(std::string scope_name, Scope &scope, std::ostream &os) {
//   std::string scope_name = std::string("scope_") + std::to_string(get_scope_num(&scope));
   for (auto &expr : scope.expressions) {
      gen_expression(scope_name, expr, os);
      gen_scope(*expr.scope, os);
   }
}

void Gen_6502::
gen_function(Function &func, std::ostream &os) {
   if (func.name.compare("__asm__") != 0) {
      
      //variable_ram_map.clear();
      if (!func.should_inline) {
         os << func.name << ":" << std::endl;
         gen_scope_expressions(func.name, *func.scope, os);

         os << '\t' << "rts" << std::endl;
      }
      gen_scope_functions(*func.scope, os);
   }
}

void Gen_6502::
gen_scope(Scope &scope, std::ostream &os) {
   if (scope.empty()) {
      return;
   }
   std::string scope_name = std::string("scope_") + std::to_string(get_scope_num(&scope));
   os << scope_name << ":" << std::endl;
   gen_scope_expressions(scope_name, scope, os);
   for (auto &func : scope.functions) {
      gen_function(func, os);
   }
   
   
}


static void generate_6502(Scope &scope, std::ostream &os) {
   //os << '\t' << "processor 6502" << std::endl;
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


extern "C" {
   extern int main_asm6(int argc, char **argv);
};

void assemble_6502(const char *filename) {

   auto create_str = [](const char *str) {
      char *out = (char *)malloc(strlen(str) + 1);
      snprintf(out, strlen(str) + 1, "%s", str);
      return out;
   };

   char *args[3];
   args[1] = create_str("-q");
   args[2] = create_str(filename);
   main_asm6(3, args);
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
   ofs.close();
   //std::cout << exec("dasm test.s -f3 -v4 -otest.bin") << std::endl;
   assemble_6502("test.s");
   return 0;
}



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
#include <stack>

#include "Code_Structure.h"

Function::
Function() {
   scope = new Scope();
   scope->is_function = true;
   scope->function = this;
}

Expression::
Expression() {
   scope = new Scope();
}

static std::stack<std::string> source_code_stack;
static std::stack<std::string> source_file_name;

static Function asmInlineFunc();
static std::string load_file(const std::string pathname);
static void parse_scope(std::string name, Scope &parent, stb_lexer &lex, long delim_token = 0);
static void parse_declaration(std::string name, Scope &scope, stb_lexer &lex);
static void parse_function(std::string name, Scope &scope, stb_lexer &lex, bool should_inline = false, bool is_plain = false);
static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex, char delim_token = ';', char opt_delim_token = ';');
static Expression parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref);
static std::vector<Instruction> parse_rvalue(Variable dst, Scope &scope, stb_lexer &lex);
static std::vector<Variable> parse_parameter_list(Scope &scope, stb_lexer &lex);
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
      case CLEX_intlit    : printf("#%g", lex.real_number); break;
#else
      case CLEX_intlit    : printf("#%ld", lex.int_number); break;
#endif
      case CLEX_floatlit  : printf("%g", lex.real_number); break;
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
      } else {
         compiler_error(std::string("use of undeclared identifier '") + token_to_string(lex) + "'", lex);
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
   expr.scope->parent = &scope;
   Conditional cond = parse_conditional(scope, lex);
   if (!cond.is_always_true) {
      Expression jump_forward;
      Instruction instr;
      instr.type = Instruction::SUBROUTINE_JUMP;
      instr.is_conditional_jump = true;
      instr.condition = cond;
      instr.func_call_name = "EOS_JUMP"; //End-Of-Scope
      jump_forward.instructions.push_back(instr);
      expr.scope->expressions.push_back(jump_forward);
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != '{') {
      compiler_error(std::string("expected token '{' before token '") + token_to_string(lex) + "'", lex);
   }
   
   parse_scope(std::string(), *expr.scope, lex, '}');
   
   scope.expressions.push_back(expr);
   {
      Expression jump_back;
      Instruction instr;
      instr.type = Instruction::SUBROUTINE_JUMP;
      instr.is_conditional_jump = false;
      instr.condition = cond;
      instr.func_call_name = "SOS_JUMP"; //End-Of-Scope
      jump_back.instructions.push_back(instr);
      expr.scope->expressions.push_back(jump_back);
   }
}

static void parse_preincrement(Scope &scope, stb_lexer &lex) {
   stb_c_lexer_get_token(&lex);
   if (lex.token == CLEX_id) {
      std::string name = std::string(lex.string, strlen(lex.string));
      Variable *var = scope.getVarByName(name);
      if (!var) {
         compiler_error(std::string("use of undeclared identifier '") + token_to_string(lex) + "'", lex);
      } else {
         printf("Found\n");
         Expression expr;
         expr.scope->parent = &scope;
         Instruction instr;
         instr.type = Instruction::INCREMENT;
         instr.lvalue_data = *var;
         expr.instructions.push_back(instr);
         scope.expressions.push_back(expr);
         printf("Found\n");
      }
   } else {
      compiler_error(std::string("expected an identifier before token '") + token_to_string(lex) + "'", lex);
   }
   
   stb_c_lexer_get_token(&lex);
   if (lex.token != ';') {
      compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
   }
   printf("Found\n");
}

static void parse_return(Scope &scope, stb_lexer &lex) {
   //stb_c_lexer_get_token(&lex);
   
   Expression expr = parse_expression("return", scope, lex, false);
   
   scope.expressions.push_back(expr);
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
         printf("preinc \n");
         parse_preincrement(scope, lex);
      } else if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         
         if (name.compare("import") == 0) {
            stb_c_lexer_get_token(&lex);
            if (lex.token == CLEX_dqstring) {
               std::string import_str = std::string(lex.string, strlen(lex.string));
               std::string import_src = load_file(import_str);
               if (import_src.compare("") == 0) {
                  import_str = source_file_name.top().substr(0, source_file_name.top().find_last_of('/')) + "/" + import_str;
                  import_src = load_file(import_str);
                  if (import_src.compare("") == 0) {
                     printf("File not found: %s\n", import_str.c_str());
                     exit(-1);
                  }
               }
               source_code_stack.push(import_src);
               source_file_name.push(import_str);
               const char *text = import_src.c_str();
               int len = strlen(text);
               stb_lexer nlex;
               stb_c_lexer_init(&nlex, text, text+len, (char *) malloc(1<<16), 1<<16);
               parse_scope(name_str, scope, nlex, CLEX_eof);
               source_code_stack.pop();
               source_file_name.pop();
               stb_c_lexer_get_token(&lex);
               if (lex.token != ';') {
                  compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
               }
            }
         } else if (name.compare("cdebug") == 0) {
            stb_c_lexer_get_token(&lex);
            if (lex.token == CLEX_dqstring) {
               std::string pstr = std::string(lex.string, strlen(lex.string));
               printf(pstr.c_str());
               printf("\n");
               stb_c_lexer_get_token(&lex);
               if (lex.token != ';') {
                  compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
               }
            }
         } else if (name.compare("while") == 0) {
            parse_while_loop(scope, lex);
         } else if (name.compare("return") == 0) {
            parse_return(scope, lex);
         } else if (!scope.contains_symbol(name)) {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               if (deref) {
                  compiler_error("attempt to dereference variable in declaration.", lex);
               } else {
                  parse_declaration(name, scope, lex);
               }
            } else {
               compiler_error(std::string("use of undeclared identifier '") + name + "'", lex);
            }
         } else {
            stb_c_lexer_get_token(&lex);
            if(lex.token == ':') {
               compiler_error("identifier already declared.", lex);
            } else {
               //printf("parsing expression\n");
               scope.expressions.push_back(parse_expression(name, scope, lex, deref));
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
   } else if (t.compare("char") == 0) {
      return Variable::CHAR;
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

static Variable parse_const_assign(Variable dst, Scope &scope, stb_lexer &lex) {
   Variable var;
   
   while (lex.token != ';') {
      if (lex.token == CLEX_intlit) {
         var.type = dst.type;
         var.pvalue = lex.int_number;
      } else {
      
      }
      
      stb_c_lexer_get_token(&lex);
   }
   
   return var;
}

static Variable parse_variable(std::string name, Scope &scope, stb_lexer &lex, char delim_token, char opt_delim_token) {
   Variable var;
   var.name = name;
   bool is_pointer = false;
   bool push = true;
   while (lex.token != delim_token) {
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
            var.pvalue = parse_const_assign(var, scope, lex).pvalue;
            break;
         } else {
            scope.variables.push_back(var);
            push = false;
            Expression expr;
            expr.scope->parent = &scope;
            std::vector<Instruction> instrs = parse_rvalue(var, scope, lex);
            for (auto &in : instrs) {
               expr.instructions.push_back(in);
            }
            scope.expressions.push_back(expr);
            break;
         }
      }
      if (lex.token == opt_delim_token) break;
      stb_c_lexer_get_token(&lex);
   }
   
   if (push) {
      scope.variables.push_back(var);
   }
   
   return var;
}

static std::vector<Instruction> parse_rvalue(Variable dst, Scope &scope, stb_lexer &lex) {
   std::vector<Instruction> instructions;
   stb_c_lexer_get_token(&lex);
   Instruction::IType itype = Instruction::ASSIGN;
   while (lex.token != ';') {
      if (lex.token == '|') {
         itype = Instruction::LOG_OR;
      } else if (lex.token == CLEX_intlit) {
         Instruction in;
         in.type = itype;
         in.lvalue_data = dst;
         in.lvalue_data.name = dst.name;
         in.rvalue_data.pvalue = lex.int_number;
         in.rvalue_data.is_type_const = true;
         instructions.push_back(in);
         stb_c_lexer_get_token(&lex);
         break;
      } else if (lex.token == CLEX_dqstring) {
         if (itype != Instruction::ASSIGN) {
            compiler_warning("using string literal for operations other than assignment is undefined.", lex);
         }
         Instruction in;
         in.type = itype;
         in.lvalue_data = dst;
         in.lvalue_data.name = dst.name;
         in.rvalue_data.dqstring = std::string(lex.string, strlen(lex.string));
         in.rvalue_data.type = Variable::DQString;
         instructions.push_back(in);
         stb_c_lexer_get_token(&lex);
         if (lex.token != ';') {
            compiler_error("Expected token ';'", lex);
         }
         break;
      } else if(lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         Variable *rvar = scope.getVarByName(name);
         if (!rvar) {
            stb_c_lexer_get_token(&lex);
            if (lex.token == '(') {
               Function *func = scope.getFuncByName(name);
               if (func) {
                  Instruction in;
                  in.type = Instruction::FUNC_CALL;
                  in.func_call_name = name;
                  in.call_target_params = parse_parameter_list(scope, lex);
                  instructions.push_back(in);
                  
                  in = Instruction();
                  in.type = itype;
                  in.lvalue_data = dst;
                  in.rvalue_data.name = std::string("return_reg");
                  
                  instructions.push_back(in);
               } else {
                  compiler_error(std::string("use of undeclared identifier '") + token_to_string(lex) + "'", lex);
               }
            }
            
         } else {
            Instruction in;
            in.type = itype;
            in.lvalue_data = dst;
            in.rvalue_data = *rvar;
            
            instructions.push_back(in);
         }
      }
      stb_c_lexer_get_token(&lex);
   }
   
   return instructions;
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
      } else if (tname.compare("plain") == 0) {
          stb_c_lexer_get_token(&lex);
         if(lex.token == '(') {
            parse_function(name, scope, lex, false, true);
         }
      } else {
         parse_variable(name, scope, lex);
      }
   } else {
      parse_variable(name, scope, lex);
   }
}

static std::vector<Variable> parse_parameter_list(Scope &scope, stb_lexer &lex) {
   //stb_c_lexer_get_token(&lex);
   std::vector<Variable> plist;
   
   while (lex.token != ')') {
//      print_token(&lex);
      if (lex.token == CLEX_dqstring) {
         Variable var;
         var.type = Variable::DQString;
         var.dqstring = std::string(lex.string, strlen(lex.string));
         plist.push_back(var);
         stb_c_lexer_get_token(&lex);
         if (lex.token != ',' && lex.token != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + token_to_string(lex) + "'", lex);
         }
         //printf("DQString %s\n", var.dqstring.c_str());
         if (lex.token == ')') break;
      } else if (lex.token == CLEX_floatlit) {
         Variable var;
         var.type = Variable::FLOAT_32BIT;
//         var.ptype = ;
         //printf("Int param %ld\n", lex.int_number * mul);
         var.fvalue = lex.real_number;
         var.is_type_const = true;
         plist.push_back(var);
         stb_c_lexer_get_token(&lex);
         if (lex.token != ',' && lex.token != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + token_to_string(lex) + "'", lex);
         }
      } else if (lex.token == '-' || lex.token == CLEX_intlit) {
         int mul = 1;
         if (lex.token == '-') {
            mul = -1;
            stb_c_lexer_get_token(&lex);
            if (lex.token != CLEX_intlit) {
               compiler_error(std::string("expected int literal before token '") + token_to_string(lex) + "'", lex);
            }
         }
         //printf("IntLit\n");
         Variable var;
         var.type = Variable::INT_32BIT;
//         var.ptype = ;
         //printf("Int param %ld\n", lex.int_number * mul);
         var.pvalue = lex.int_number * mul;
         var.is_type_const = true;
         plist.push_back(var);
         stb_c_lexer_get_token(&lex);
         if (lex.token != ',' && lex.token != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + token_to_string(lex) + "'", lex);
         }
      } else if (lex.token == CLEX_id) {
         std::string name = std::string(lex.string, strlen(lex.string));
         stb_c_lexer_get_token(&lex);
         //printf("name %s\n", name.c_str());
         if (lex.token == ':') {
            Variable var = parse_variable(name, scope, lex, ',', ')');
            plist.push_back(var);
            continue;
         } else {
            Variable *var_ref = scope.getVarByName(name);
            
            if (var_ref) {
               plist.push_back(*var_ref);
            } else {
               compiler_error(std::string("488: use of undeclared identifier '") + name + "'", lex);
            }
         }
      }
      if (lex.token == ')') break;
      stb_c_lexer_get_token(&lex);
   }
   
   return plist;
}


static void parse_function(std::string name, Scope &scope, stb_lexer &lex, bool should_inline, bool is_plain) {
   Function func;
   func.should_inline = should_inline;
   func.plain_instructions = is_plain;
   func.name = name;
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
   } else {
      std::string type_name = std::string(lex.string, strlen(lex.string));
      Variable return_type;
      return_type.type = Variable::POINTER;
      return_type.ptype = get_vtype(type_name);
      func.return_info = return_type;
   }
   stb_c_lexer_get_token(&lex);
   if (lex.token != '{') {
      if (lex.token == ';') {
         func.is_not_definition = true;
         scope.functions.push_back(func);
         return;
      } else {
         compiler_error(std::string("unexpected token '") + token_to_string(lex)+ "'", lex);
      }
   }
   
   parse_scope(name, *func.scope, lex, '}');
   scope.functions.push_back(func);
}




static Expression parse_expression(std::string name, Scope &scope, stb_lexer &lex, bool deref) {
   Expression expr;
   expr.scope->parent = &scope;
   
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
            Instruction instr;
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
   else if (lex.token == '=' || name.compare("return") == 0) {
      Instruction instr;
      instr.type = Instruction::ASSIGN;
      Variable *var = scope.getVarByName(name);
      if (var) {
         if (var->is_type_const && !deref) {
            compiler_error("read-only variable is not assignable", lex);
         }
         instr.lvalue_data = *var;
         instr.lvalue_data.type = (deref ? Variable::DEREFERENCED_POINTER : Variable::POINTER);
      } else if (name.compare("return") == 0) {
         instr.lvalue_data.name = name;
         if (scope.is_function) {
            instr.lvalue_data = scope.function->return_info;
            instr.lvalue_data.name = name;
            instr.lvalue_data.type = Variable::POINTER;
         }
      } else {
         compiler_error("Error: undefined reference to " + name, lex);
      }
      
      std::vector<Instruction> instrs = parse_rvalue(instr.lvalue_data, scope, lex);
      //stb_c_lexer_get_token(&lex);
      if (lex.token != ';') {
         compiler_error(std::string("expected token ';' before token '") + token_to_string(lex) + "'", lex);
      }
      for (auto &in : instrs) {
         expr.instructions.push_back(in);
      }
   }
   //scope.expressions.push_back(expr);
   return expr;
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
   source_code_stack.push(src);
   const char *text = src.c_str();
   int len = strlen(text);
   stb_lexer lex;
   stb_c_lexer_init(&lex, text, text+len, (char *) malloc(1<<16), 1<<16);
   Scope globalScope;
   globalScope.functions.push_back(asmInlineFunc());
   parse_scope(std::string(), globalScope, lex, CLEX_eof);
   source_code_stack.pop();
   return globalScope;
}


static std::string load_file(const std::string pathname) {
   std::ifstream t(pathname);
   if (!t.good()) {
      //printf("File not found: %s\n", pathname.c_str());
      return "";
   }
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

//#include "Gen_6502.h"
//
//static void generate_6502(Scope &scope, std::ostream &os) {
//   //os << '\t' << "processor 6502" << std::endl;
//   os << '\t' << "SEG" << std::endl;
//   os << '\t' << "ORG $F000" << std::endl;
//   Gen_6502 g6502;
//   g6502.gen_scope(scope, os);
//   os << '\t' << "ORG $FFFA" << std::endl;
//   os << '\t' << ".word main" << std::endl;
//   os << '\t' << ".word main" << std::endl;
//   os << '\t' << ".word main" << std::endl;
//   os << '\t' << "END" << std::endl;
//}

#include "Gen_386.h"

static void generate_386(Scope &scope, std::ostream &os) {
   //os << ".intel_syntax" << std::endl;
   //generate function extern table (GAS doesnt require this?)
   //os << ".data" << std::endl;
   //generate data section
   //os << ".text" << std::endl;
   os << ".section __TEXT,__text,regular,pure_instructions" << std::endl;
   Gen_386 g386;
   g386.gen_scope(scope, os);
   
   os << ".section __TEXT,__cstring,cstring_literals" << std::endl;
   g386.gen_rodata(os);
}

#include <fstream>
#include <string>
#include <stdio.h>
#include <sstream>

static int error_count = 0;

static void print_line_with_arrow(int line_number, int offset) {
   std::string line;
   std::stringstream ss(source_code_stack.top());
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
	std::cout << source_file_name.top() << ":" << loc.line_number << ":" << loc.line_offset << ": warning: " << msg << std::endl;
   print_line_with_arrow(loc.line_number, loc.line_offset);
}

static void compiler_error(std::string msg, stb_lexer &lex) {
   stb_lex_location loc;
	stb_c_lexer_get_location(&lex, lex.where_firstchar, &loc);
	std::cout << source_file_name.top() << ":" << loc.line_number << ":" << loc.line_offset << ": error: " << msg << std::endl;
   print_line_with_arrow(loc.line_number, loc.line_offset);
   error_count++;
   if (error_count > 5) {
      exit(-1);
   }
}

std::string exec(std::string cmd) {
    printf("%s\n", cmd.c_str());
    FILE* pipe = popen(cmd.c_str(), "r");
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


static std::string output_file;
bool no_link = false;
static std::string link_options = "";

//void assemble_6502(const char *filename) {
//
//   auto create_str = [](const char *str) {
//      char *out = (char *)malloc(strlen(str) + 1);
//      snprintf(out, strlen(str) + 1, "%s", str);
//      return out;
//   };
//
//   char *args[3];
//   args[1] = create_str("-q");
//   args[2] = create_str(filename);
//   main_asm6(3, args);
//}

#include <cstdio>

void assemble_386(std::string path_str) {
   std::string out(path_str);
   out.replace(out.rfind(".s"), 2, ".o");
   if (no_link) {
      if (output_file.compare("") == 0) {
         output_file = out;
      }
      std::cout << exec(std::string("as -arch i386 -g -Q -o ") + output_file + " " + path_str) << std::endl;
   } else {
      std::cout << exec(std::string("as -arch i386 -g -Q -o ") + out + " " + path_str) << std::endl;
      if (output_file.compare("") == 0) {
         output_file = out;
         output_file.replace(output_file.rfind(".o"), 2, "");
      }
      std::string _static = "";
      if (link_options.size() == 0) {
         _static = "-static ";
      }
      std::cout << exec(std::string("ld " + _static + "-arch i386 -e _htn_ctr0_OSX_i386_start  -macosx_version_min 10.10 -o ") + output_file + " " + out + " " + link_options) << std::endl;
      //remove(out.c_str());
   }
   //remove(path_str.c_str());
}

static void print_usage() {
   printf("Usage: htn [options] <sources> \n");
   printf("\nOptions:\n");
   printf("     -m386      IA-32 executable\n");
  // printf("     -m6502     MOS 6502 flat binary, Atari 2600 flavor\n");
   printf("     -o <out>   Specify file for output\n");
   printf("     -c         Stop after compilation, does not invoke linker\n");
}

int main(int argc, char** argv) {
   if (argc < 2) {
      print_usage();
      return 0;
   }
   bool gen386 = true;
   std::string source_path;
   for (int i = 1; i < argc; ++i) {
      std::string arch = argv[i];
      if (arch.compare("-m386") == 0) {
         gen386 = true;
      } else if(arch.compare("-m6502") == 0) {
         //gen386 = false;
      } else if (arch.compare("-c") == 0) {
         no_link = true;
      } else if (arch.compare("-o") == 0) {
         ++i;
         if (i >= argc) {
            printf("Not enough args to support -o switch\n");
            break;
         }
         output_file = argv[i];
      } else if (arch.compare(0, 2, "-l") == 0) {
         link_options += arch + " ";
      } else if (arch.compare("-framework") == 0) {
         link_options += arch + " ";
         ++i;
         if (i >= argc) {
            printf("Not enough args to support -framework\n");
            break;
         }
         std::string fw = argv[i];
         link_options += fw + " ";
      } else if (arch.rfind("-") == 0) {
         printf("Unrecognized option: %s\n", arch.c_str());
      } else {
         source_path = argv[i];
      }
   }
   
   std::string source = load_file(source_path);
   source_file_name.push(source_path);
   Scope scope = parse(source);
   source_file_name.pop();
   if (error_count) {
      return -1;
   }
   source_path.replace(source_path.rfind(".htn"), std::string::npos, ".s");
   std::ofstream ofs(source_path);
   if (gen386) {
      generate_386(scope, ofs);
      ofs.close();
      assemble_386(source_path);
   } else {
//      generate_6502(scope, ofs);
//      ofs.close();
//      assemble_6502(source_path.c_str());
   }
   
   return 0;
}



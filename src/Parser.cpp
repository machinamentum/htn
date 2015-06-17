
#include <cstdio>

#include <string>
#include <fstream>
#include <streambuf>
#include <vector>
#include <cstdint>
#include <stack>
#include <cstring>
#include <fstream>
#include <string>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include "Parser.h"
#include "Code_Gen.h"

static std::stack<std::string> source_code_stack;
std::stack<std::string> source_file_name;
std::string load_file(const std::string pathname);

void print_token(Token tok) {
   printf("%s", (tok.pretty_string() + " ; ").c_str());
}

int error_count = 0;

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

static void compiler_warning(std::string msg, Token &tok) {
	std::cout << source_file_name.top() << ":" << tok.line_number << ":" << tok.line_offset << ": warning: " << msg << std::endl;
   print_line_with_arrow(tok.line_number, tok.line_offset);
}

static void compiler_error(std::string msg, Token &tok) {
	std::cout << source_file_name.top() << ":" << tok.line_number << ":" << tok.line_offset << ": error: " << msg << std::endl;
   print_line_with_arrow(tok.line_number, tok.line_offset);
   error_count++;
   if (error_count > 5) {
      exit(-1);
   }
}

Conditional Parser::parse_conditional(Scope &scope) {
   Conditional cond;
   Token tok = lex.next_token();
   if (tok.type == Token::ID) {
      std::string name = tok.string;
      Variable *lvar = scope.getVarByName(name);
      if (lvar) {
         cond.left = *lvar;
      } else if (name.compare("true") == 0) {
         cond.is_always_true = true;
         tok = lex.next_token();
         if (tok.type != ')') {
            compiler_error(std::string("Conditional laziness error"), tok);
         }
         return cond;
      } else {
         compiler_error(std::string("use of undeclared identifier '") + tok.pretty_string() + "'", tok);
      }
   }
   tok = lex.next_token();
   if (tok.type == '<') {
      cond.condition = Conditional::LESS_THAN;
   }

   tok = lex.next_token();
   if (tok.type == Token::INTLIT) {
      Variable var;
      var.type = Variable::INT_32BIT;
      var.pvalue = tok.int_number;
      var.is_type_const = true;
      cond.right = var;
   }

   tok = lex.next_token();
   if (tok.type != ')') {
      compiler_error(std::string("expected token ')' before token '") + tok.pretty_string() + "'", tok);
   }

   return cond;
}

void Parser::parse_while_loop(Scope &scope) {
   Token tok = lex.next_token();
   if (tok.type != '(') {
       compiler_error(std::string("expected token '(' before token '") + tok.pretty_string() + "'", tok);
   }

   Expression expr;
   expr.scope->parent = &scope;
   Conditional cond = parse_conditional(scope);
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
   tok = lex.next_token();
   if (tok.type != '{') {
      compiler_error(std::string("expected token '{' before token '") + tok.pretty_string() + "'", tok);
   }

   parse_scope(std::string(), *expr.scope, '}');

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

void Parser::parse_preincrement(Scope &scope) {
   Token tok = lex.next_token();
   if (tok.type == Token::ID) {
      std::string name = tok.string;
      Variable *var = scope.getVarByName(name);
      if (!var) {
         compiler_error(std::string("use of undeclared identifier '") + tok.pretty_string() + "'", tok);
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
      compiler_error(std::string("expected an identifier before token '") + tok.pretty_string() + "'", tok);
   }

   tok = lex.next_token();
   if (tok.type != ';') {
      compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
   }
   printf("Found\n");
}

void Parser::parse_return(Scope &scope, Token &tok) {
   // tok = lex.next_token();

   printf("START_RETURN\n\n");

   Expression expr = parse_expression("return", scope, tok, false);

   scope.expressions.push_back(expr);
   printf("\n\nEND_RETURN\n");
}

void Parser::parse_scope(std::string name_str, Scope &scope, long delim_token) {
   printf("START_SCOPE\n\n");
   bool deref = false;
   Token tok = lex.next_token();
   while (tok.type != Token::EOF) {
      if (tok.type == delim_token) {
         break;
      }
      if (tok.type == '*') {
         deref = true;
      } else if (tok.type == Token::PLUSPLUS) {
         printf("preinc \n");
         parse_preincrement(scope);
      } else if(tok.type == Token::ID) {
         std::string name = tok.string;

         if (name.compare("import") == 0) {
            tok = lex.next_token();
            if (tok.type == Token::DQSTRING) {
               std::string import_str = tok.string;
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
               Parser par = Parser(import_src);
               par.parse_scope(name_str, scope, Token::EOF);
               source_code_stack.pop();
               source_file_name.pop();
               tok = lex.next_token();
               if (tok.type != ';') {
                  compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
               }
            }
         } else if (name.compare("cdebug") == 0) {
            tok = lex.next_token();
            if (tok.type == Token::DQSTRING) {
               std::string pstr = tok.string;
               printf("%s\n", pstr.c_str());
               tok = lex.next_token();
               if (tok.type != ';') {
                  compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
               }
            }
         } else if (name.compare("while") == 0) {
            parse_while_loop(scope);
         } else if (name.compare("return") == 0) {
            parse_return(scope, tok);
         } else if (!scope.contains_symbol(name)) {
            tok = lex.next_token();
            if(tok.type == ':') {
               if (deref) {
                  compiler_error("attempt to dereference variable in declaration.", tok);
               } else {
                  parse_declaration(name, scope);
               }
            } else {
               compiler_error(std::string("use of undeclared identifier '") + name + "'", tok);
            }
         } else {
            tok = lex.next_token();
            if(tok.type == ':') {
               compiler_error("identifier already declared.", tok);
            } else {
               //printf("parsing expression\n");
               scope.expressions.push_back(parse_expression(name, scope, tok, deref));
               deref = false;
            }
         }
      } else {
         print_token(tok);
         printf("  ");
      }

      tok = lex.next_token();
   }

   printf("\n\nEND_SCOPE\n");
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

   return Variable::UNKNOWN;
}

Variable Parser::parse_const_assign(Variable dst, Scope &scope, Token &tok) {
   Variable var;

   while (tok.type != ';') {
      if (tok.type == Token::INTLIT) {
         var.type = dst.type;
         var.pvalue = tok.int_number;
      } else {

      }

      tok = lex.next_token();
   }

   return var;
}

Variable Parser::parse_variable(std::string name, Scope &scope, Token &tok, char delim_token, char opt_delim_token) {
   Variable var;
   var.name = name;
   bool is_pointer = false;
   bool push = true;
   while (tok.type != delim_token) {
      if (tok.type == '*') {
         is_pointer = true;
         var.type = Variable::POINTER;
      } else if (tok.type == Token::ID) {
         std::string vtype = tok.string;
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
      } else if (tok.type == '=') {
         if (var.is_type_const) {
            var.pvalue = parse_const_assign(var, scope, tok).pvalue;
            break;
         } else {
            scope.variables.push_back(var);
            push = false;
            Expression expr;
            expr.scope->parent = &scope;
            std::vector<Instruction> instrs = parse_rvalue(var, scope, tok);
            for (auto &in : instrs) {
               expr.instructions.push_back(in);
            }
            scope.expressions.push_back(expr);
            break;
         }
      }
      if (tok.type == opt_delim_token) break;
      tok = lex.next_token();
   }

   if (push) {
      scope.variables.push_back(var);
   }

   return var;
}

std::vector<Instruction> Parser::parse_rvalue(Variable dst, Scope &scope, Token &tok) {
   std::vector<Instruction> instructions;
   tok = lex.next_token();
   Instruction::IType itype = Instruction::ASSIGN;
   while (tok.type != ';') {
      if (tok.type == '|') {
         itype = Instruction::BIT_OR;
      } else if (tok.type == Token::INTLIT) {
         Instruction in;
         in.type = itype;
         in.lvalue_data = dst;
         in.lvalue_data.name = dst.name;
         in.rvalue_data.type = Variable::INT_32BIT;
         in.rvalue_data.pvalue = tok.int_number;
         in.rvalue_data.is_type_const = true;
         instructions.push_back(in);
         tok = lex.next_token();
         break;
      } else if (tok.type == Token::DQSTRING) {
         if (itype != Instruction::ASSIGN) {
            compiler_warning("using string literal for operations other than assignment is undefined.", tok);
         }
         Instruction in;
         in.type = itype;
         in.lvalue_data = dst;
         in.lvalue_data.name = dst.name;
         in.rvalue_data.dqstring = tok.string;
         in.rvalue_data.type = Variable::DQString;
         instructions.push_back(in);
         tok = lex.next_token();
         if (tok.type != ';') {
            compiler_error("Expected token ';'", tok);
         }
         break;
      } else if(tok.type == Token::ID) {
         std::string name = tok.string;
         Variable *rvar = scope.getVarByName(name);
         if (!rvar) {
            tok = lex.next_token();
            if (tok.type == '(') {
               Function *func = scope.getFuncByName(name);
               if (func) {
                  Instruction in;
                  in.type = Instruction::FUNC_CALL;
                  in.func_call_name = name;
                  in.call_target_params = parse_parameter_list(scope, tok);
                  instructions.push_back(in);

                  in = Instruction();
                  in.type = itype;
                  in.lvalue_data = dst;
                  in.rvalue_data = REG_RETURN;

                  instructions.push_back(in);
               } else {
                  compiler_error(std::string("use of undeclared identifier '") + tok.pretty_string() + "'", tok);
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
      tok = lex.next_token();
   }

   return instructions;
}

void Parser::parse_declaration(std::string name, Scope &scope) {
   Token tok = lex.next_token();
   if(tok.type == '(') {
      parse_function(name, scope, tok);
   } else if(tok.type == Token::ID) {
      std::string tname = tok.string;
      if (tname.compare("inline") == 0) {
          tok = lex.next_token();
         if(tok.type == '(') {
            parse_function(name, scope, tok, true);
         }
      } else if (tname.compare("plain") == 0) {
          tok = lex.next_token();
         if(tok.type == '(') {
            parse_function(name, scope, tok, false, true);
         }
      } else {
         parse_variable(name, scope, tok);
      }
   } else {
      parse_variable(name, scope, tok);
   }
}

std::vector<Variable> Parser::parse_parameter_list(Scope &scope, Token &tok) {
   tok = lex.next_token();
   std::vector<Variable> plist;

   while (tok.type != ')') {
//      print_token(&lex);
      if (tok.type == Token::DQSTRING) {
         Variable var;
         var.type = Variable::DQString;
         var.dqstring = tok.string;
         plist.push_back(var);
         tok = lex.next_token();
         if (tok.type != ',' && tok.type != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + tok.pretty_string() + "'", tok);
         }
         //printf("DQString %s\n", var.dqstring.c_str());
         if (tok.type == ')') break;
      } else if (tok.type == Token::FLOATLIT) {
         Variable var;
         var.type = Variable::FLOAT_32BIT;
//         var.ptype = ;
         //printf("Int param %ld\n", tok.int_number * mul);
         var.fvalue = tok.real_number;
         var.is_type_const = true;
         plist.push_back(var);
         tok = lex.next_token();
         if (tok.type != ',' && tok.type != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + tok.pretty_string() + "'", tok);
         }
      } else if (tok.type == '-' || tok.type == Token::INTLIT) {
         int mul = 1;
         if (tok.type == '-') {
            mul = -1;
            tok = lex.next_token();
            if (tok.type != Token::INTLIT) {
               compiler_error(std::string("expected int literal before token '") + tok.pretty_string() + "'", tok);
            }
         }
         //printf("IntLit\n");
         Variable var;
         var.type = Variable::INT_32BIT;
//         var.ptype = ;
         //printf("Int param %ld\n", tok.int_number * mul);
         var.pvalue = tok.int_number * mul;
         var.is_type_const = true;
         plist.push_back(var);
         tok = lex.next_token();
         if (tok.type != ',' && tok.type != ')') {
            compiler_error(std::string("expected token ',' or ')' before token '") + tok.pretty_string() + "'", tok);
         }
      } else if (tok.type == Token::ID) {
         std::string name = tok.string;
         tok = lex.next_token();
         //printf("name %s\n", name.c_str());
         if (tok.type == ':') {
            Variable var = parse_variable(name, scope, tok, ',', ')');
            plist.push_back(var);
            continue;
         } else {
            Variable *var_ref = scope.getVarByName(name);

            if (var_ref) {
               plist.push_back(*var_ref);
            } else {
               compiler_error(std::string("488: use of undeclared identifier '") + name + "'", tok);
            }
         }
      }
      if (tok.type == ')') break;
      tok = lex.next_token();
   }

   return plist;
}


void Parser::parse_function(std::string name, Scope &scope, Token &tok, bool should_inline, bool is_plain) {
   Function func;
   func.should_inline = should_inline;
   func.plain_instructions = is_plain;
   func.name = name;
   func.scope->parent = &scope;
   if (tok.type != ')') {
      func.parameters = parse_parameter_list(*func.scope, tok);
   }

   tok = lex.next_token();
   if (tok.type != Token::ARROW) {
      compiler_error(std::string("expected token '->' before token '") + tok.pretty_string() + "'", tok);
   }
   tok = lex.next_token();
   if (tok.type != Token::ID) {
      compiler_error(std::string("unexpected token '") + tok.pretty_string() + "'", tok);
   } else {
      std::string type_name = tok.string;
      Variable return_type;
      return_type.type = Variable::POINTER;
      return_type.ptype = get_vtype(type_name);
      func.return_info = return_type;
   }
   tok = lex.next_token();
   if (tok.type != '{') {
      if (tok.type == ';') {
         func.is_not_definition = true;
         scope.functions.push_back(func);
         return;
      } else {
         compiler_error(std::string("unexpected token '") + tok.pretty_string() + "'", tok);
      }
   }

   parse_scope(name, *func.scope, '}');
   scope.functions.push_back(func);
}




Expression Parser::parse_expression(std::string name, Scope &scope, Token &tok, bool deref) {
   Expression expr;
   expr.scope->parent = &scope;

   if (tok.type == '(') {
      Function *tfunc = scope.getFuncByName(name);
      if (tfunc) {
         if (tfunc->should_inline) {
            parse_parameter_list(scope, tok); //TODO(josh)
            for (Expression &expr : tfunc->scope->expressions) {
               scope.expressions.push_back(expr);
            }
            tok = lex.next_token();
            if (tok.type != ';') {
               compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
            }
         } else {
            Instruction instr;
            instr.type = Instruction::FUNC_CALL;
            instr.func_call_name = name;
            instr.call_target_params = parse_parameter_list(scope, tok);

            tok = lex.next_token();
            if (tok.type != ';') {
               compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
            }
            expr.instructions.push_back(instr);
         }
      } else {
         compiler_error(std::string("use of undeclared identifier '") + name + "'", tok);
      }
   }
   /*else if (tok.type == ':') {
      scope.variables.push_back(parse_variable(name, scope, lex));
   }*/
   else if (tok.type == '=' || name.compare("return") == 0) {
      Instruction instr;
      instr.type = Instruction::ASSIGN;
      Variable *var = scope.getVarByName(name);
      if (var) {
         if (var->is_type_const && !deref) {
            compiler_error("read-only variable is not assignable", tok);
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
         compiler_error("Error: undefined reference to " + name, tok);
      }

      std::vector<Instruction> instrs = parse_rvalue(instr.lvalue_data, scope, tok);
      // tok = lex.next_token();
      if (tok.type != ';') {
         compiler_error(std::string("expected token ';' before token '") + tok.pretty_string() + "'", tok);
      }
      for (auto &in : instrs) {
         expr.instructions.push_back(in);
      }
   }
   //scope.expressions.push_back(expr);
   return expr;
}

Function asmInlineFunc() {
   Function func;
   func.name = "__asm__";

   Variable dqs;
   dqs.type = Variable::DQString;
   func.parameters.push_back(dqs);
   return func;
}

Scope Parser::parse(std::string src) {
   source_code_stack.push(src);
   Parser par = Parser(src);
   Scope globalScope;
   globalScope.functions.push_back(asmInlineFunc());
   par.parse_scope(std::string(), globalScope, Token::EOF);
   source_code_stack.pop();
   return globalScope;
}

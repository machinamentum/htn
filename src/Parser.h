
#ifndef PARSER_H
#define PARSER_H

#include "Lexer.h"
#include "Code_Structure.h"

struct Parser {

   Lexer lex;
   Parser(std::string src) : lex(NULL, NULL){
      char *string = (char *)malloc(src.length());
      for (size_t i = 0; i < src.length(); ++i) {
         string[i] = src[i];
      }
      lex = Lexer(string, string + src.length());
   }

   Conditional parse_conditional(Scope &scope);
   void parse_while_loop(Scope &scope);
   void parse_preincrement(Scope &scope);
   void parse_return(Scope &scope, Token &tok) ;
   void parse_scope(std::string name, Scope &parent, long delim_token = 0);
   Struct parse_struct(Scope &scope);
   Variable parse_const_assign(Variable dst, Scope &scope, Token &tok);
   Variable parse_variable(std::string name, Scope &scope, Token &tok, char delim_token = ';', char opt_delim_token = ';');
   std::vector<Instruction> parse_rvalue(Variable dst, Scope &scope, Token &tok);
   void parse_declaration(std::string name, Scope &scope);
   std::vector<Variable> parse_parameter_list(Scope &scope, Token &tok);
   void parse_function(std::string name, Scope &scope, Token &tok, bool should_inline = false, bool is_plain = false);
   Expression parse_expression(std::string name, Scope &scope, Token &tok, bool deref);
   static Scope parse(std::string src);
};

#endif


#ifndef LEXER_H
#define LEXER_H

#include <string>
#undef EOF

struct Token {

   enum TokenType {
      EOF = 256,
      PARSE_ERROR,
      INTLIT,
      FLOATLIT,
      ID,
      DQSTRING,
      SQSTRING,
      CHARLIT,
      EQ,
      NOTEQ,
      LESSEQ,
      GREATEREQ,
      ANDAND,
      OROR,
      SHL,
      SHR,
      PLUSPLUS,
      MINUSMINUS,
      PLUSEQ,
      MINUSEQ,
      MULEQ,
      DIVEQ,
      MODEQ,
      ANDEQ,
      OREQ,
      XOREQ,
      ARROW,
      EQARROW,
      SHLEQ,
      SHREQ,
      FIRST_UNUSED_TOKEN // ???
   };

   long type;
   double real_number;
   long int_number;
   std::string string;

   int line_number;
   int line_offset;

   char *new_parse_loc = NULL;

   const std::string pretty_string() const {
      switch (type) {
         case EOF: return std::string("eof");
         case PARSE_ERROR: return std::string("err");
         case INTLIT: return std::to_string(int_number);
         case FLOATLIT: return std::to_string(real_number);
         case ID: return std::string("_" + string);
         case DQSTRING: return std::string("\"" + string + "\"");
         case SQSTRING: return std::string("\'" + string + "\'");
         case CHARLIT: return std::string("\'" + std::to_string((char)int_number) + "\'");
         case EQ: return std::string("==");
         case NOTEQ: return std::string("!=");
         case LESSEQ: return std::string("<=");

         case ARROW: return std::string("->");
         default: return std::string() + (char)type;
      }
   }

};

struct Lexer {

   char *input_stream;
   char *parse_loc;
   char *eof;
   int current_line;
   int current_offset;

   Lexer(const char *input, const char *eof);

   Token next_token();
   Token peek_token();
   bool is_eof();
};


#endif


#include "Lexer.h"

#include <cstdio>

static Token get_token(Lexer &lex);
static bool is_white(int x);
void print_token(Token tok);

Lexer::Lexer(const char *input, const char *eof) {
   this->input_stream = (char *)input;
   this->parse_loc = (char *)input;
   this->eof = (char *)eof;
   current_line = 1;
   current_offset = 0;
}

bool Lexer::is_eof() {
   Token tok = get_token(*this);
   return tok.type == Token::EOF;
}

Token Lexer::next_token() {
   Token tok = get_token(*this);
   if (tok.new_parse_loc) {
      parse_loc = tok.new_parse_loc;
   }
   // if (tok.type != Token::EOF) {
   //    print_token(tok);
   //    printf("\n");
   // }
   return tok;
}

Token Lexer::peek_token() {
   Token tok = get_token(*this);
   return tok;
}

static bool is_white(int x) {
   return x == ' ' || x == '\t' || x == '\f';
}

static bool is_newline(int x) {
   return x == '\n' || x == '\r';
}

static int do_newline(Lexer &lex, const char *p) {
   int off = p[0] + p[1] == '\r' + '\n' ? 2 : 1;
   ++lex.current_line;
   lex.current_offset = 0;
   return off;
}

static Token token(Lexer &lex, int off, char *npl, long type, std::string str = std::string(), long int_num = 0, double real_number = 0.0) {
   lex.current_offset += off;
   Token tok;
   tok.string = std::string(str);
   tok.int_number = int_num;
   tok.real_number = real_number;
   tok.type = type;
   tok.line_number = lex.current_line;
   tok.line_offset = lex.current_offset;
   tok.new_parse_loc = npl;
   return tok;
}

static Token parse_error() {
   Token tok;
   tok.type = Token::PARSE_ERROR;
   return tok;
}

static Token eof() {
   Token tok;
   tok.type = Token::EOF;
   return tok;
}

static Token get_token(Lexer &lex) {

   char *p = lex.parse_loc;

   while (p != lex.eof && (is_white(*p) || is_newline(*p))) {
      if (*p == '\r' || *p == '\n') {
         p += do_newline(lex, p);
      } else if (p != lex.eof && p[0] == '/' && p[1] == '/') {
         while (p != lex.eof && *p != '\r' && *p != '\n') {
            ++p;
            ++lex.current_offset;
         }
         continue;
      } else {
         ++p;
         ++lex.current_offset;
      }
   }

   if (p == lex.eof) return eof();

   switch (*p) {
      default:
         if ( (*p >= 'a' && *p <= 'z')
             || (*p >= 'A' && *p <= 'Z')
             || *p == '_' || (unsigned char) *p >= 128
             || *p == '$' ) {

            int n = 0;
            std::string string = std::string();
            do {
               string += p[n];
               ++n;
            } while (
                  (p[n] >= 'a' && p[n] <= 'z')
               || (p[n] >= 'A' && p[n] <= 'Z')
               || (p[n] >= '0' && p[n] <= '9') // allow digits in middle of identifier
               || p[n] == '_' || (unsigned char) p[n] >= 128
               || p[n] == '$'
            );

            return token(lex, n, p+(n), Token::ID, string);
         }
         if (*p == 0) return eof();

         single_char:
            return token(lex, 1, p+1, *p);

      case '\"': {
         std::string string;
         char * q = p;
         auto parse_char = [&lex, &q](char *in) -> char {

            if (*in == '\\' && in + 1 != lex.eof) {
               q += 2;
               switch (in[1]) {
                  case '\\': return '\\';
                  case '\'': return '\'';
                  case '"': return '"';
                  case 't': return '\t';
                  case 'f': return '\f';
                  case 'n': return '\n';
                  case 'r': return '\r';
                  case '0': return '\0'; //TODO ocatal constants
                  case 'x': case 'X': return -1; //TODO hex constants
                  case 'u': return -1; //TODO unicode constants
               }
            }
            ++q;
            return *in;
         };
         ++q;
         while (q != lex.eof && *q != '\"') {
            string += parse_char(q);
         }
         ++q;
         return token(lex, q - p, q, Token::DQSTRING, string);
      }
      case '+':
         if (p + 1 != lex.eof) {
            if (p[1] == '+') return token(lex, 2, p+2, Token::PLUSPLUS);
            if (p[1] == '=') return token(lex, 2, p+2, Token::PLUSEQ);
         }
         goto single_char;
      case '-':
         if (p + 1 != lex.eof) {
            if (p[1] == '>') return token(lex, 2, p+2, Token::ARROW);
            if (p[1] == '-') return token(lex, 2,  p+2, Token::MINUSMINUS);
            if (p[1] == '=') return token(lex, 2, p+2, Token::MINUSEQ);
         }
         goto single_char;

      case '0':
         if (p + 1 != lex.eof) {
            //hex
            if (p[1] == 'x' || p[1] == 'X') {
               char *q;
               long number = strtol((char *) p + 2,  &q, 16);
               if (q == p + 2) return parse_error();
               return token(lex, q - p, q, Token::INTLIT, std::string(), number);
            }

            //bin
            if (p[1] == 'b' || p[2] == 'B') {
               char *q;
               long number = strtol((char *) p + 2,  &q, 2);
               if (q == p + 2) return parse_error();
               return token(lex, q - p, q, Token::INTLIT, std::string(), number);
            }
         }

      case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
         {
            char *q = p;
            while (q != lex.eof && (*q >= '0' && *q <= '9'))
               ++q;
            if (q != lex.eof) {
               if (*q == '.' || *q == 'e' || *q == 'E') {
                  double real_number = strtod((char *) p, (char**) &q);
                  return token(lex, q - p, q, Token::FLOATLIT, std::string(), 0, real_number);
               }
            }
         }
         if (p[0] == '0') {
            char *q = p;
            long num = strtol((char *) p, (char **) &q, 8);
            return token(lex, q - p, q, Token::INTLIT, std::string(), num);
         }

         {
            char *q = p;
            long num = strtol((char *) p, (char **) &q, 10);
            return token(lex, q - p, q, Token::INTLIT, std::string(), num);
         }

      case '*':
         if (p + 1 != lex.eof) {
            if (p[1] == '=') return token(lex, 2, p+2, Token::MULEQ);
         }
         goto single_char;

   }

}

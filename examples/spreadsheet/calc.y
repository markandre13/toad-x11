/*
 * TOAD Spreadsheet Example 
 * Copyright (C) 1996-2002 by Mark-André Hopf <mhopf@mark13.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

%{ 

#include <iostream>
#include <sstream>

using namespace std;

extern "C" {
  static int yyerror(const char*);
  static int yywrap();
  static int yylex(); 
};
        
extern char *yytext;
        
%}

%start formula

%union {
  int decimal;
}

%type <decimal> formula
%type <decimal> decimal
%type <decimal> expr
%type <decimal> term
%type <decimal> factor

%token TKN_DECIMAL

%%

formula
  : expr
    { cerr << $1 << endl; }
  ;

expr
  : expr '+' term
    { $$ = $1 + $3;
      // cerr << $1 << "+" << $3 << " -> " << $$ << endl;
    }
  | expr '-' term
    { $$ = $1 - $3;
      // cerr << $1 << "-" << $3 << " -> " << $$ << endl;
    }
  | term
    { $$ = $1; }
  ;
  
term
  : term '*' factor
    { $$ = $1 * $3;
      // cerr << $1 << "*" << $3 << " -> " << $$ << endl;
    }
  | term '/' factor
    { $$ = $1 / $3;
      // cerr << $1 << "/" << $3 << " -> " << $$ << endl;
    }
  | factor
    { $$ = $1; }
  ;

factor
  : decimal
    { $$ = $1; }
  | '(' expr ')'
    { $$ = $2; }
  ;
  
  
decimal
  : TKN_DECIMAL
    { // cerr << "decimal " << yytext << endl; 
      $$ = atoi(yytext);
    }
  ;
  
%%

static istream *yy_istream;

#define YY_INPUT(buffer, result, max_size) \
  { yy_istream->read(buffer, max_size); \
    result = yy_istream->gcount(); }

#include "calc.l.cc"

void
parse(istream &in)
{
  YY_FLUSH_BUFFER;
  yy_istream = &in;
  if (yyparse()) {
  }
}

int
mainx(int argc, char ** argv)
{
//  yydebug = 0;
  istringstream code(argv[1]);
  try {
    parse(code);
  }
  catch(exception &e) {
    cerr << "error: " << e.what() << endl;
  }
  return 0;
}

int
yyerror(const char *text)
{
  cerr << "yyerror: " << text << endl;
  return 0;   
}
  
int
yywrap()
{
  return 1;
}

/*
 * Attribute-Type-Value Object Language Parser
 * Copyright (C) 2001-2003 by Mark-André Hopf <mhopf@mark13.de>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the authors nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "atvparser.hh"

#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <iostream>

using namespace std;
using namespace atv;

TATVInterpreter::~TATVInterpreter()
{
}

bool
TATVInterpreter::interpret(TATVParser&)
{
  return false;
}

TATVParser::TATVParser(istream *stream)
{
  _eof = false;
  verbose = false;
  interpreter = NULL;
  depth = 0;
  line = 1;
  in = stream;
  istate = 0;
  position = 0;
  running = false;
}

#define TKN_ERROR 257
#define TKN_STRING 258

void
TATVParser::push()
{
  TStackElement e;
  e.interpreter = interpreter;
  e.position = position;
  e.istate = istate;
  stack.push(e);
}

void
TATVParser::pop()
{
  if (!stack.empty()) {
    interpreter = stack.top().interpreter;
    position    = stack.top().position;
    istate      = stack.top().istate;
    stack.pop();
  }
}

void
TATVParser::unexpectedToken(int t)
{
  err << "unexpected token ";

  switch(t) {
    case TKN_STRING:
      err << "string '" << yytext;
      break;
    default:
      err << '\'' << (char)t;
  }
  err << " in line " << line << ':' << endl
      << line1 << line2 << endl;
  for(unsigned i=0; i<line1.size(); ++i)
    err << ' ';
  err << "^ around here" << endl;
}

/**
 * When 'err' isn't empty, set it otherwise append " in line ___".
 *
 * The condition allows the semantic layer to specify more specific error messages.
 */
void
TATVParser::semanticError() 
{
  if (err.str().size()==0) {
    err << "syntax/semantic error";
  }
  err << " in line " << line << ':' << endl
      << line1 << line2 << endl;
  for(unsigned i=0; i<line1.size(); ++i)
    err << ' ';
  err << "^ around here" << endl;
}

void
TATVParser::failed(const char * file, unsigned line, const char *function)
{
  cerr << file << ", " << line << ": " << function \
       << " failed to interpret ATV triple ('" \
       << attribute << "'[" <<position<<"] , '" \
       << type << "', '" \
       << value << "') for what=" << getWhatName() << endl;
}

const char *
TATVParser::getWhatName() const
{
  static const char * whatname[] = {
    "ATV_START", "ATV_VALUE", "ATV_GROUP",  "ATV_FINISHED"
  };
  if (what < 0 || what >= (sizeof(whatname)/sizeof(*whatname)))
    return "(TATVParser.what out of range)";
  return whatname[what];
}

bool
TATVParser::parse()
{  
  int t;
  int state = 0;

  string unknown;

  if (running) {
    err << "bool TATVParser::parse() recursion";
    return false;
  }
  
  running = true;

  while( running && (t=yylex()) != EOF ) {
    if (t==TKN_ERROR) {
      err << " in line " << line << ':' << endl
          << line1 << line2 << endl;
      for(unsigned i=0; i<line1.size(); ++i)
        err << ' ';
      err << "^ around here" << endl;
      return false;
    }
#if 0
    switch(t) {
      case TKN_STRING:
        printf("%i ['%s']\n", state, yytext.c_str() );
        break;
      default:
        printf("%i ['%c']\n", state, t);
    }
#endif
    switch(state) {
      case 0:
        switch(t) {
          case TKN_STRING:
            unknown = yytext;
            attribute.clear();
            type.clear();
            state = 1;
            break;
          case '{':
            attribute.clear();
            type.clear();
            if (!startGroup()) {
              return false;
            }
            break;
          case '}':
            if (!endGroup()) {
              return false;
            }
            break;
          default:
            unexpectedToken(t);
            return EXIT_FAILURE;
        }
        break;
      case 1: // string ?
        switch(t) {
          case '=':
            attribute = unknown;
            state = 2;
            break;
          case '{':
            type = unknown;
            if (!startGroup()) {
              return false;
            }
            state = 0;
            break;
          case '}':
            value = unknown;
            if (!single()) {
              return false;
            }
            if (!endGroup()) {
              return false;
            }
            state = 0;
            break;
          case TKN_STRING:
            value = unknown;
            if (!single()) {
              return false;
            }
            unknown = yytext;
            attribute.clear();
            type.clear();
            break;
          default:
            unexpectedToken(t);
            return false;
        }
        break;
      case 2: // attribute '=' ?
        switch(t) {
          case TKN_STRING:
            unknown = yytext;
            state = 3;
            break;
          case '{':
            if (!startGroup()) {
              return false;
            }
            state = 0;
            break;
          default:
            unexpectedToken(t);
            return false;
        }
        break;
      case 3: // attribute '=' string ?
        switch(t) {
          case '{': // attribute '=' string '{'
            type = unknown;
            if (!startGroup()) {
              return false;
            }
            state = 0;
            break;
          case TKN_STRING:
            value = unknown;
            if (!single()) {
              return false;
            }
            unknown = yytext;
            attribute.clear();
            type.clear();
            state = 1;
            break;
          case '}':
            value = unknown;
            if (!single()) {
              return false;
            }
            if (!endGroup()) {
              return false;
            }
            state = 0;
            break;
          default:
            unexpectedToken(t);
            return false;
        }
        break;
    }
  }
  if (t==EOF && state!=0) {
      err << "incomplete atv triple";
      err << " in line " << line << ':' << endl
          << line1 << line2 << endl;
      for(unsigned i=0; i<line1.size(); ++i)
        err << ' ';
      err << "^ around here" << endl;
      return false;
  } 
  return true;
}

int
TATVParser::yylex()
{
  int c;
  int state = 0;
  
  yytext.clear();
  while(true) {
    c = in->get();
    if (c==EOF)
      _eof=true;
    if (c=='\n') {
      ++line;
      line1.clear();
      line2.clear();
    } else {
      line2+=c;
    }
    switch(state) {
      case 0:
        switch(c) {
          case ' ':
          case '\t':
          case '\r':
          case '\n':
            break;
          case '\"':
            state = 2;
            break;
          case '/':
            state = 4;
            break;
          case '{':
          case '}':
          case '=':
          case EOF:
            return c;
          default:
            yytext+=c;
            state = 1;
            break;
        }
        break;
      case 1:
        switch(c) {
          case '\n':
            --line; /* we do ungetc, don't count this line 2 times */
          case ' ':
          case '\t':
          case '\r':
          case '{':
          case '}':
          case '=':
          case '/':
          case EOF:
            in->putback(c);
            return TKN_STRING;
          default:
            yytext+=c;
        }
        break;
      case 2:
        switch(c) {
          case EOF:
            err << "unterminated string or character constant";
            return TKN_ERROR;
          case '\"':
            return TKN_STRING;
          case '\\':
            state = 3;
            break;
          default:
            yytext+=c;
        }
        break;
      case 3:
        yytext+=c;
        state = 2;
        break;
      case 4:
        switch(c) {
          case '/':
            state = 5;
            break;
          case '*':
            state = 6;
            break;
          default:
            err << "expected '/*' or '//'";
            return TKN_ERROR;
        }
        break;
      case 5:
        switch(c) {
          case '\n':
            state = 0;
            break;
        }
        break;
      case 6:
        switch(c) {
          case '*':
            state = 7;
            break;
        }
        break;
      case 7:
        switch(c) {
          case '/':
            state = 0;
            break;
          default:
            state = 6;
        }
    }
  }
}

bool
TATVParser::single()
{
  if (verbose) {
    cerr << "single: ";
    for(unsigned i=0; i<=depth; ++i)
      cerr << "  ";
    cerr << "atv=(\""<< attribute << "\", \"" << type << "\", \"" << value << "\")" << endl;
  }
  if (interpreter) {
    what = ATV_VALUE;
    if (!interpreter->interpret(*this)) {
      if (debug) {
        cerr << "  failed (value; "
                "attribute=\"" << attribute << "\"; "
                "position=\"" << position << "\"; "
                "type=\"" << type << "\"; "
                "value=\"" << value << "\")" << endl;
      }
      semanticError();
      return false;
    } else {
      if (debug)
        cerr << "  parsed" << endl;
    }
  }
  ++position;
  line1+=line2;
  line2.clear();
  return true;
}

bool
TATVParser::startGroup()
{
  if (verbose) {
    cerr << "group: ";
    for(unsigned i=0; i<depth; ++i)
      cerr << "  ";
    cerr << "atv=(\""<< attribute << "\", \"" << type << "\", ...)" << endl;
  }
  ++position;
  push();
  if (interpreter) {
    TATVInterpreter *oldintp = interpreter;
    what = ATV_GROUP;
    if (!interpreter->interpret(*this)) {
      if (debug)
        cerr << "  failed (group; "
                "attribute=\"" << attribute << "\"; "
                "position=\"" << position << "\"; "
                "type=\"" << type << "\"; "
                "value=\"" << value << "\")" << endl;
      semanticError();
      return false;
    }
    if (debug)
        cerr << "parsed group" << endl;
    if (oldintp != interpreter) {
      what = ATV_START;
      value.clear();
      interpreter->interpret(*this);
    }
  }
  position = 0;
  ++depth;
  line1+=line2;
  line2.clear();
  return true;
}

bool
TATVParser::endGroup()
{
  --depth;
  if (verbose) {
    for(unsigned i=0; i<depth; ++i)
      cout << "  ";
    cout << "}" << endl;
  }
  if (interpreter) {
    what = ATV_FINISHED;
    if (!interpreter->interpret(*this)) {
      semanticError();
      return false;
    }
  }
  pop();
  return true;
}

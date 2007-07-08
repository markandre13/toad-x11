/*
 * Attribute-Type-Value Object Language Parser
 * Copyright (C) 2001-2004 by Mark-André Hopf <mhopf@mark13.org>
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
#include <locale>

using namespace std;
using namespace atv;

#ifdef __WIN32__
#define clear erase
#endif


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
  debug = false;
  interpreter = NULL;
  what = ATV_START;
  depth = 0;
  line = 1;
  istate = 0;
  position = 0;
  running = false;
  setIStream(stream);
}

void
TATVParser::setIStream(std::istream *stream) {
  in = stream;
  if (in)
    in->imbue(locale("C"));
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

bool
TATVParser::pop()
{
  if (!stack.empty()) {
    interpreter = stack.top().interpreter;
    position    = stack.top().position;
    istate      = stack.top().istate;
    stack.pop();
    return true;
  }
  return false;
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
  err << " in line " << line;
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
  err << " in line " << line << endl;
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
  int t=EOF;

  if (running && interpreter) {
    err << "bool TATVParser::parse() recursion";
    return false;
  }
  if (!running) {
    state = 0;
  }
  
  running = true;
  unsigned startdepth = depth;

  if (!interpreter) {
    switch(what) {
      case ATV_GROUP:
        ++depth;
        break;
      case ATV_FINISHED:
        --depth;
        break;
    }
  } else {
    if (depth!=0) {
      what = ATV_START;
      value.clear();
      interpreter->interpret(*this);
    }
  }
//cout << "(parse started in state " << state << ", unknown='" << unknown << "')";

  while( running ) {
    if (state < 10 ) {
      t = yylex();
      if (t==TKN_ERROR) {
        err << " in line " << line << endl;
        return false;
      }
#if 0
      switch(t) {
        case TKN_STRING:
          printf("state=%i depth=%i startdepth=%i ['%s']\n", state, depth, startdepth, yytext.c_str() );
          break;
        default:
          printf("state=%i depth=%i startdepth=%i ['%c']\n", state, depth, startdepth, t);
      }
#endif
    }
    switch(state) {
      case 0:
        switch(t) {
          case TKN_STRING:
            unknown = yytext;
            state = 1;
            break;
          case '{':
            attribute.clear();
            type.clear();
            value.clear();
            if (!startGroup()) {
              return false;
            }
            if (!interpreter)
              return true;
            break;
          case '}':
            state = 0;
            if (!endGroup()) {
              return false;
            }
            if (interpreter && depth==startdepth) {
              what = ATV_FINISHED;
              attribute.clear();
              type.clear();
              value.clear();
              state = 12;
              ++depth;
              return true;
            }
            break;
          case EOF:
            attribute.clear();
            type.clear();
            value.clear();
            what = ATV_FINISHED;
            if (!interpreter)
              return false;
            break;
          default:
            unexpectedToken(t);
            return false;
        }
        break;
      case 1: // string ?
        attribute.clear();
        type.clear();
        value.clear();
        switch(t) {
          case '=':
            attribute = unknown;
            state = 2;
            break;
          case '{':
            type = unknown;
            state = 0;
            if (!startGroup()) {
              return false;
            }
            break;
          case '}':
            value = unknown;
            state = 10;
            if (!single()) {
              return false;
            }
            if (interpreter && depth==startdepth) {
              if (interpreter) {
                if (!interpreter->interpret(*this)) {
                  semanticError();
                  return false;
                }
              }
              what = ATV_FINISHED;
              attribute.clear();
              type.clear();
              value.clear();
              state = 12;
              ++depth;
              return true;
            }
            break;
          case TKN_STRING:
//cout << "+++++++++, unknown=" << unknown << ", yytext=" << yytext << ", value=" << value << endl;
            value = unknown;
            unknown = yytext;
            if (!single()) {
              return false;
            }
//cout << "---------, unknown=" << unknown << ", yytext=" << yytext << ", value=" << value << endl;
            if (!interpreter)
              return true;
            break;
          case EOF:
            value = unknown;
            state = 11;
            if (!single()) {
              return false;
            }
            if (!interpreter)
              return true;
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
            state = 0;
            if (!startGroup()) {
              return false;
            }
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
            state = 0;
            if (!startGroup()) {
              return false;
            }
            break;
          case TKN_STRING:
            value = unknown;
            state = 1;
            unknown = yytext;
            if (!single()) {
              return false;
            }
            if (!interpreter) {
//cout << __FILE__ << ":" << __LINE__ << endl;
              return true;
            }
            break;
          case '}':
            value = unknown;
            state=10;
            if (!single()) {
              return false;
            }
            if (interpreter && depth==startdepth) {
              what = ATV_FINISHED;
              attribute.clear();
              type.clear();
              value.clear();
              if (interpreter) {
                if (!interpreter->interpret(*this)) {
                  semanticError();
                  return false;
                }
              }
              state = 12;
              ++depth;
              return true;
            }
            if (!interpreter) {
              what = ATV_VALUE;
              return true;
            }
            break;
          case EOF:
            value = unknown;
            state = 0;
            if (!single()) {
              return false;
            }
            break;
          default:
            unexpectedToken(t);
            return false;
        }
        break;
      case 10:
        what = ATV_FINISHED;
        attribute.clear();
        type.clear();
        value.clear();
        state=0;
        if (!endGroup()) {
          return false;
        }
        break;
      case 11:
        what = ATV_FINISHED;
        attribute.clear();
        type.clear();
        value.clear();
        state=0;
        return false;
      case 12:
        what = ATV_FINISHED;
        attribute.clear();
        type.clear();
        value.clear();
        state=0;
        depth++;
        return true;
    }
    if (t==EOF)
      break;
    if (!interpreter && (state==0 || state==10))
      return true;
  }
  if (t==EOF && state!=0) {
      err << "incomplete atv triple in line " << line << endl;
      return false;
  } 
  return true;
}

int
TATVParser::yylex()
{
  int c;
  int hex;
  int state = 0;
  
  yytext.clear();
  while(true) {
    c = get();
//printf("lex: %d '%c'\n", state, c);
    if (c==EOF)
      _eof=true;
    if (c=='\n') {
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
      case 1: // ?...
        switch(c) {
          case '\n':
          case ' ':
          case '\t':
          case '\r':
          case '{':
          case '}':
          case '=':
          case '/':
          case EOF:
            putback(c);
            return TKN_STRING;
          default:
            yytext+=c;
        }
        break;
      case 2: // "?
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
      case 3: // "..\?
        switch(c) {
          case 'x':
          case 'X':
            hex = 0;
            state = 8;
            break;
          case EOF:
            err << "unterminated string or character constant";
            return TKN_ERROR;
          default:
            yytext+=c;
            state = 2;
        }
        break;
      case 4: // /?
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
      case 5: // //?
        switch(c) {
          case '\n':
            state = 0;
            break;
          case EOF:
            return EOF;
        }
        break;
      case 6: // /*..?
        switch(c) {
          case '*':
            state = 7;
            break;
          case EOF:
            err << "unexpected end of file in comment";
            return TKN_ERROR;
        }
        break;
      case 7: // /*..*?
        switch(c) {
          case '*':
            break;
          case '/':
            state = 0;
            break;
          case EOF:
            err << "unexpected end of file in comment";
            return TKN_ERROR;
          default:
            state = 6;
        }
        break;
      case 8: // \x?
        if (c>='0' && c<='9') {
          hex += c-'0';
        } else
        if (c>='a' && c<='f') {
          hex += 10+c-'a';
        } else
        if (c>='A' && c<='F') {
          hex += 10+c-'A';
        } else {
           err << "expected hexadecimal digit";
           return TKN_ERROR;
        }
        hex<<=4;
        state = 9;
        break;
      case 9: // \x??
        if (c>='0' && c<='9') {
          hex += c-'0';
        } else
        if (c>='a' && c<='f') {
          hex += 10+c-'a';
        } else
        if (c>='A' && c<='F') {
          hex += 10+c-'A';
        } else {
           err << "expected hexadecimal digit";
           return TKN_ERROR;
        }
        yytext += hex;
        state = 2;
        break;
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
  what = ATV_VALUE;
  if (interpreter) {
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

//#error "write test cases for interpreter = 0!!!"

bool
TATVParser::startGroup()
{
//printf("startGroup\n");
  if (verbose) {
    cerr << "group: ";
    for(unsigned i=0; i<depth; ++i)
      cerr << "  ";
    cerr << "atv=(\""<< attribute << "\", \"" << type << "\", ...)" << endl;
  }
  ++position;
  what = ATV_GROUP;
  if (interpreter) {
    push();
    TATVInterpreter *oldintp = interpreter;
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
    if (oldintp != interpreter && interpreter) {
      what = ATV_START;
      value.clear();
      interpreter->interpret(*this);
    }
    if (what==ATV_FINISHED) {
      if (interpreter)
        --depth;
      pop();
      return true;
    }
    if (interpreter)
      ++depth;
  }
  position = 0;
  line1+=line2;
  line2.clear();
  return true;
}

bool
TATVParser::endGroup()
{
  if (verbose) {
    for(unsigned i=0; i<depth; ++i)
      cerr << "  ";
    cerr << "}" << endl;
  }
  what = ATV_FINISHED;
  attribute.clear();
  type.clear();
  value.clear();
  if (interpreter) {
    if (depth==0) {
      cerr << "unexpected end of group" << endl;
      return false;
    }
    --depth;
    if (!interpreter->interpret(*this)) {
      semanticError();
      return false;
    }
    if (!pop()) {
      return false;
    }
  }
  return true;
}

/**
 * Interpret the group as a C/C++ style code section and return
 * its content (except comments) as a text string.
 *
\pre
bool
regvariable_t::interpret(TATVParser &p)
{
  switch(p.what) {
    ...
    case ATV_GROUP:
       if (p.attribute == "script" && p.type.empty())
         return p.getCode(&script);
       ...
       break;
    ...
  }
}
\endpre
 */
bool
TATVParser::getCode(string *code)
{
  unsigned state = 1;
  unsigned depth = 0;
  unsigned startline = line;
  unsigned startstring = 0;
  while(state) {
    int c = get();
    if (c==EOF) {
      if (state==2 || state==3) {
        err << "Unexpected end file in string beginning at line "
            << startstring;
      } else {
        err << "Unexpected end of file in code section at beginning at line " 
            << startline;
      }
      return false;
    }

//printf("state %u depth=%u '%c' '%s'\n", state, depth, c, code->c_str());

    switch(state) {
      case 1:
        switch(c) {
          case '/':
            state = 4;
            break;
          case '"':
            code->append(1, c);
            state = 2;
            startstring = line;
            break;
          case '{':
            code->append(1, c);
            ++depth;
            break;
          case '}':
            if (depth==0) {
              state = 0;
            } else {
              code->append(1, c);
              --depth;
            }
            break;
          default:
            code->append(1, c);
        }
        break;
      case 2:
        switch(c) {
          case '"':
            code->append(1, c);
            state = 1;
            break;
          case '\\':
            code->append(1, c);
            state = 3;
            break;
          default:
            code->append(1, c);
        }
        break;
      case 3:
        code->append(1, c);
        state = 2;
        break;
        
      case 4:
        switch(c) {
          case '/':
            state = 7;
            break;
          case '*':
            state = 5;
            break;
          default:
            code->append(1, '/');
            putback(c);
            state = 1;
        }
        break;
        
      case 5: // C style comment
        if (c=='*')
          state = 6;
        break;
      case 6:
        switch(c) {
          case '*':
            break;
          case '/':
            state = 1;
            break;
          default:
            state = 5;
        }
        break;
        
      case 7: // C++ style comment
        if (c=='\n')
          state = 1;
        break;
    }
  }

  if (verbose) {
    for(unsigned i=0; i<depth; ++i)
      cerr << "  ";
    cerr << "} // code" << endl;
  }
  
  ++this->depth;
  what=ATV_FINISHED;

//printf("got code: <begin>\n%s\n<end>\n", code->c_str());

  return true;
}

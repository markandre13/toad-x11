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

#ifndef _ATV_ATVPARSER_HH
#define _ATV_ATVPARSER_HH

#include <string>
#include <iostream>
#include <sstream>
#include <stack>

namespace atv {

enum EATVWhat
{
  ATV_START,
  ATV_VALUE,
  ATV_GROUP,
  ATV_FINISHED,
};

class TATVParser;

/**
 * \class TATVInterpreter
 *
 * This class is a lightweight interface which is called by TATVParser.
 *
 * \sa TATVParser
 */
class TATVInterpreter
{
  public:
    virtual ~TATVInterpreter();
    virtual bool interpret(TATVParser&);
};

#define ATV_FAILED(p) p.failed(__FILE__, __LINE__, __PRETTY_FUNCTION__);

/**
 * \class TATVParser
 *
 * \sa TATVInterpreter
 *
 * \note
 *   \li
 *     This is my answer to YAML (http://www.yaml.org/).
 *   \li
 *     The code follows style of YACC and LEX created code.
 */
class TATVParser
{
  public:
    TATVParser(std::istream *stream = NULL);
    std::string attribute, type, value;
    EATVWhat what;
    const char * getWhatName() const;
    void failed(const char * file, unsigned line, const char *function);
    
    void setIStream(std::istream *stream);
    
    /**
     * Start parsing the input stream.
     */
    bool parse();
    
    /**
     * Stop parsing the input stream.
     */
    void stop() {
      running = false;
    }
    
    void setVerbose(bool verbose) { this->verbose = verbose; }
    bool isVerbose() const { return verbose; }

    void setDebug(bool debug) { this->debug = debug; }
    bool isDebug() const { return debug; }
    
    /**
     * To set another interpreter, ie. after 'what==ATV_GROUP'.
     */
    void setInterpreter(TATVInterpreter *i) { interpreter = i; }
    TATVInterpreter * getInterpreter() const { return interpreter; }
    
    /**
     * Return the position of the current element inside its group.
     */
    int getPosition() const { return position; }
    /**
     * Return the nesting level of groups.
     */
    unsigned getDepth() const { return depth; }
    
    /**
     * To set a user defined state, ie. after 'what==ATV_GROUP'.
     */
    void setInterpreterState(unsigned state) { this->istate = state; }
    unsigned getInterpreterState() const { return istate; }
    
    /**
     * Provides an error text in case 'parse()' returns 'false'.
     */
    std::string getErrorText() const { return err.str(); }
    std::stringstream err;

    bool good() const { return !err.str().empty() && !_eof; }
    bool eof() const { return _eof; }
    bool fail() const { return err.str().empty(); }
    operator bool() const { return err.str().empty(); }
    bool operator!() const { return !err.str().empty(); }
    
  protected:
    bool single();
    bool startGroup();
    bool endGroup();
    
    TATVInterpreter * interpreter;
    void push();
    void pop();

    struct TStackElement
    {
      TATVInterpreter* interpreter;
      int position;
      unsigned istate;
    };
    typedef std::stack<TStackElement> TStack;
    TStack stack;
    
    bool _eof;
    volatile bool running;

    bool verbose;
    bool debug;

    /* syntax */

    int yylex();
    void unexpectedToken(int t);
    void semanticError();

    /* last string we got but don't know what it is */
    std::string unknown;
    
    /* nesting */
    unsigned depth;

    /* element number */
    int position;

    /* lexical analyser */
  
    /*! current line number */
    unsigned line;
    
    unsigned istate;
  
    /*! text parsed in this line and send to the interpreter */
    std::string line1;
    /*! test parsed in this line and not yet send to the interpreter */
    std::string line2;
    std::istream *in;
    std::string yytext;
};

} // namespace atv

namespace toad {
  using namespace atv;
} // namespace toad

#endif

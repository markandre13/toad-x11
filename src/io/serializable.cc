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

#include "serializable.hh"

#include <iostream>
#include <sstream>

using namespace std;
using namespace atv;

static TObjectStore defaultstore;

/**
 * the default restore method for all serializable objects.
 *
 * \return 'true' on ATV_START and ATV_FINISHED, 'false' for all
 *         other calls.
 */
bool
TSerializable::restore(TInObjectStream &in)
{
  if (in.what==ATV_START || in.what==ATV_FINISHED)
    return true;
  return false;
}

void
TSerializable::store(TOutObjectStream&) const
{
}

TObjectStore&
atv::getDefaultStore()
{
  return defaultstore;
}

void 
TObjectStore::registerObject(TSerializable *obj)
{
  buffer[obj->getClassName()]=obj;
}

void
TObjectStore::unregisterAll()
{
#if 1
  TSerializableBuffer::iterator p, e;
  p = buffer.begin();
  e = buffer.end();
  while(p!=e) {
    delete (*p).second;
    ++p;
  }
  buffer.clear();
#endif
}

bool
TObjectStore::isRegistered(const string &type) const
{
  return buffer.find(type.c_str())!=buffer.end();
}

/**
 * Find an object with the given typename and return a clone.
 */
TSerializable*
TObjectStore::clone(const string &type)
{
  TSerializable *s = 0;
  TSerializableBuffer::iterator ptr;
  ptr = buffer.find(type.c_str());
  if (ptr!=buffer.end()) {
    s = static_cast<TSerializable*>(ptr->second->clone());
//    cerr << "in " << this << " found type " << type << endl;
    if (!s) {
      cerr << "failed to clone type, got NULL" << endl;
    }
  } else {
    cerr << "in " << this << " unknown type " << type << endl;
    ptr = buffer.begin();
    while(ptr!=buffer.end()) {
      cout << "  know " << (*ptr).first << endl;
      ++ptr;
    }
  }
  return s;
}

// TOutObjectStream
//---------------------------------------------------------------------------

void
TOutObjectStream::setOStream(std::ostream *out)
{
  out->imbue(locale("C"));
  init(out->rdbuf()); // redirect our input to 'out'
}

void
TOutObjectStream::store(const TSerializable *s)
{
  indent();
  (*this) << s->getClassName();
  startGroup();
  s->store(*this);
  endGroup();
}

void
TOutObjectStream::indent()
{
  // if (!newline)
    (*this) << std::endl;
    ++line;
  for(unsigned i=0; i<depth; ++i)
    (*this) << "  ";
}

void
TOutObjectStream::attribute(const string &)
{
}

void
TOutObjectStream::type(const string &)
{
}

void
TOutObjectStream::value(const string &)
{
}

void
TOutObjectStream::startGroup()
{
  (*this) << '{';
  gline = line;
  ++depth;
}

void
TOutObjectStream::endGroup()
{
  --depth;
  if (line!=gline) {
    indent();
    (*this) << '}';
  } else {
    (*this) << " }";
  }
}

// TInObjectStream
//---------------------------------------------------------------------------
TInObjectStream::TInObjectStream(std::istream *stream, TObjectStore *store):
  TATVParser(stream)
{
  _eof = false;
  if (store)
    this->store = store;
  else 
    this->store = &defaultstore;
  setInterpreter(this);
  verbose = false;
  debug = false;
  obj = 0;
}      

bool
TInObjectStream::interpret(TATVParser &p)
{
  switch(p.getInterpreterState()) {
    case 0:
      switch(p.what) {
        case ATV_GROUP:
          if (!store) {
            err << "no TObjectStore to recreate objects" << endl;
            return false;
          }
          obj = store->clone(p.type);
          if (isVerbose())
            cerr << "created new object " << p.type << endl;
          if (!obj) {
            p.err << "encountered unknown type '" << p.type << "', ";
            return false;
          }
          p.setInterpreterState(1);
//          cerr << "created and parsing " << type << endl;
          return true;
        default:
          p.err << "expected start of group";
      } break;
    case 1:
      switch(p.what) {
        case ATV_VALUE:
        case ATV_GROUP:
          if (!obj->restore(*this)) {
            p.err << (obj ? obj->getClassName() : "(NULL)") << " failed on ";
            if (p.what==ATV_VALUE)
              p.err << "ATV_VALUE";
            else
              p.err << "ATV_GROUP";
            p.err << " with ATV triple ('" 
                  << p.attribute << "', '" 
                  << p.type << "', '" 
                  << p.value << "')";
            return false;
          }
          return true;
        case ATV_FINISHED:
          if (!obj->restore(*this)) {
            p.err << obj->getClassName() << " failed on ATV_FINISHED";
            return false;
          }
          p.stop();
          return true;
      }
  }
  p.err << "unknown type '" << p.type << "'";
  return false;
}

TSerializable *
TInObjectStream::restore()
{
  if (isVerbose())
    cerr << "TInObjectStream::restore entered" << endl;
  obj = 0;
  if (!parse()) {
    if (isVerbose())
      cerr << "TInObjectStream::restore found nothing" << endl;
    return 0;
  }
  if (isVerbose())
    cerr << "TInObjectStream::restore returns object" << endl;
  return obj;
}

/*
 * helper functions to retrieve implicit types
 * (non implicit types are returned via a pointer)
 */

void
TOutObjectStream::writeQuoted(const char *p, unsigned n)
{
  write("\"", 1);

  const char * e = p+n;
  n = 0;
  const char * l = p;
  while(p<e) { 
    if (*p=='\"') {
      if (n) {     
        write(l, n);
      }
      write("\\\"", 2);
      ++p;
      l = p;
      n = 0;
    } else {
      ++n;    
      ++p;  
    }
  }  
  if (n)
    write(l, n);
  write("\"", 1);
}

// TSerializable
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, const TSerializable &s)
{
  out.startGroup();
  s.store(out);
  out.endGroup();
}

void
store(TOutObjectStream &out, const TSerializable *s)
{
  if (!s) {
    out << " NULL {}";
  } else {
    // #warning "should call out.indent() when called directly"
    out.indent();
    out << s->getClassName() << " ";
    out.startGroup();
    s->store(out);
    out.endGroup();
  }
}

// int
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, int value)
{
  out << ' ' << value;
}

bool
restore(TInObjectStream &in, int *value)
{
  if (in.what != ATV_VALUE)
    return false;
  char *endptr;
  *value = strtol(in.value.c_str(), &endptr, 10);
  if (endptr!=0 && *endptr!=0)
    return false;
  return true;
}

// unsigned
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, unsigned value)
{
  out << ' ' << value;
}

bool
restore(TInObjectStream &in, unsigned *value)
{
  if (in.what != ATV_VALUE)
    return false;
  *value = atoi(in.value.c_str());
  return true;
}

// float
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, const float &value)
{
  out << ' ' << value;
}

bool
restore(TInObjectStream &in, float *value)
{
  if (in.what != ATV_VALUE)
    return false;
  char *endptr;
  *value = strtod(in.value.c_str(), &endptr);
  if (endptr!=0 && *endptr!=0)
    return false;
  return true;
}

// double
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, const double &value)
{
  out << ' ' << value;
}

bool
restore(TInObjectStream &in, double *value)
{
  if (in.what != ATV_VALUE)
    return false;
#if 1
  istringstream vs(in.value);
  vs.imbue(locale("C"));
  vs >> *value;
  if (static_cast<unsigned>(vs.tellg()) != in.value.size())
    return false;
#else
  char *endptr;
  setlocale(LC_NUMERIC, "C");
  *value = strtod(in.value.c_str(), &endptr);
  setlocale(LC_NUMERIC, "");
  if (endptr!=0 && *endptr!=0)
    return false;
#endif
  return true;
}

// bool
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, bool value)
{
  out << ' ' << (value ? "true" : "false");
}

bool
restore(TInObjectStream &in, bool *value)
{
  if (in.what != ATV_VALUE)
    return false;
  *value = in.value.compare("true")==0;
  return true;
}

// char
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, char value)
{
  out << ' ' << (unsigned)value;
}

bool
restore(TInObjectStream &in, char *value)
{
  if (in.what != ATV_VALUE)
    return false;
  if (!in.attribute.empty())
    return false;
  *value = atoi(in.value.c_str());
  return true;
}

// unsigned char
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, unsigned char value)
{
  out << ' ' << (unsigned)value;
}

bool
restore(TInObjectStream &in, unsigned char *value)
{
  if (in.what != ATV_VALUE)
    return false;
  if (!in.attribute.empty())
    return false;
  *value = atoi(in.value.c_str());
  return true;
}

// string
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, const string &value)
{
  out << ' ';
  out.writeQuoted(value);
}

bool 
restore(TInObjectStream &in, string *value)
{
  if (in.what != ATV_VALUE)
    return false;
  *value = in.value;
  return true;
}

// char*
//---------------------------------------------------------------------------
void
store(TOutObjectStream &out, const char *value)
{
  out << ' ';
  out.writeQuoted(value, strlen(value));
}

void
storeCStr(TOutObjectStream &out, const char *value, unsigned n)
{
  out << ' ';
  out.writeQuoted(value, n);
}

bool 
restore(TInObjectStream &in, char **value)
{
  if (in.what != ATV_VALUE)
    return false;
  *value = strdup(in.value.c_str());
  return true;
}

// char*
//---------------------------------------------------------------------------

namespace {

int
base64_decode_6(char v)
{
  if (v>='A' && v<='Z')
    return v-'A';
  if (v>='a' && v<='z')
    return v-'a'+26;   
  if (v>='0' && v<='9')
    return v-'0'+52;
  switch(v) {
    case '+': return 62;
    case '/': return 63;
    case '=': return 64;
  }
  return -1;
}

int
base64_decode_24(const char **in, int *n)
{
  int v=0, b, i;
  *n = 0;
  for(i=0; i<4; i++) {
    b = base64_decode_6(**in);
    ++(*in);
    if (b<0)
      break;
    if (b!=64)
      *n+=6;  
    else
      b=0;
    v<<=6;
    v|=b;
  }
  return v;
}

int
base64_decode(const char *in, char *out, unsigned *len)
{
  int n;
  int d;
  char *start = out;
  while(true) {
    d = base64_decode_24(&in, &n);
    if (n>=8) {
      *out = (char) (d>>16)&255;
      ++out;
    }
    if (n>=16) {
      *out = (char) (d>> 8)&255;
      ++out;
    }
    if (n>=24) {
      *out = (char) (d   )&255;
      ++out;
    }
    if (n<24)
      break;
  }
  *out = 0;
  *len = out - start;
  return 0;
}

char
base64_encode6(int v)
{
  if (v<26)
    return v+'A';
  if (v<52)
    return v-26+'a';
  if (v<62)
    return v-52+'0';
  if (v==62)
    return '+';
  return '/';
}

void
base64_encode24(ostream &out, int d, int n)
{
  if (n==0)
    return;
  int m = 0;
  
  d<<=(24-n);
  int o=24;
  
  while(n>0) {
    if (o>18)
      out << base64_encode6((d>>18)&63);
    else if (o>12)
      out << base64_encode6((d>>12)&63);
    else if (o>6)
      out << base64_encode6((d>> 6)&63);
    else
      out << base64_encode6((d    )&63);
    ++m;
    n-=6;
    o-=6;
  }
  while(m<4) {
    out << '=';
    ++m;
  }
}

int
base64_encode(ostream &out, const char * ptr, unsigned len)
{
  unsigned i, d, n;
  while(true) {
    d = 0;
    n = 0;
    for(i=0; i<3; ++i) {
      if (!len)
        break;
      --len;
      d<<=8;
      d|=(unsigned char)*ptr;
      ++ptr;
      n+=8;
    }
    base64_encode24(out, d, n);
    if (i<3)
      break;
  }
  return 0;
}

}

void
storeRaw(TOutObjectStream &out, const char *ptr, unsigned n)
{
  out << " BASE64";
  out.startGroup();
  out.indent();
  while(true) {
    out << '"';
    if (n>=48) {
      base64_encode(out, ptr, 48);
      n-=48;
      ptr+=48;
    } else {
      base64_encode(out, ptr, n);
      n = 0;
      out << '"';
      break;
    }
    out << '"';
    out.indent();
  }
  out.endGroup();
}

namespace {

class TATVBase64Interpreter:
  public TATVInterpreter
{
    char **ptr;
    unsigned *len;
    
    unsigned size;
    
  public:
    bool interpret(TATVParser &p);
    void setLocation(char **p, unsigned *l=0) {
      ptr = p;
      len = l;
    }
};

bool
TATVBase64Interpreter::interpret(TATVParser &in)
{
  if (in.what==ATV_START) {
    size = 0;
    *ptr = 0;
    return true;
  }
  if (in.what==ATV_VALUE) {
    char buffer[in.value.size()*4/3+1];
    unsigned n;
    base64_decode(in.value.c_str(), buffer, &n);
    *ptr = (char*)realloc(*ptr, size+n+1);
    memcpy(*ptr + size, buffer, n);
    size+=n;
    if (len)
      *len = size;
  }
  return true;
}

TATVBase64Interpreter base64;

}

bool 
restoreRaw(TInObjectStream &in, char **value, unsigned *n)
{
  if (in.what != ATV_GROUP)
    return false;
  if (in.type != "BASE64" ) {
    in.err << "expected BASE64";
    ATV_FAILED(in);
    return false;
  }
  *value = 0;
  *n = 0;
  base64.setLocation(value, n);
  in.setInterpreter(&base64);
  return true;
}

//---------------------------------------------------------------------------

bool
finished(TInObjectStream &in)
{
  if (in.what != ATV_FINISHED)
    return false;
  return true;
}

TATVNullInterpreter atv::null;

bool
TATVNullInterpreter::interpret(TATVParser&)
{
  return true;
}


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

#ifndef _ATV_SERIALIZE_HH
#define _ATV_SERIALIZE_HH

#include <toad/io/atvparser.hh>
#ifdef __WIN32__
#include <ostream.h>
#else
#include <ostream>
#endif
#include <map>
#include <string>
#include <cstring>

namespace atv {

class TSerializable;

class TOutObjectStream:
  public std::ostream
{
  public:
    TOutObjectStream(): std::ostream(NULL) { depth=0; line=0; }
    TOutObjectStream(std::ostream* out): std::ostream(NULL) { 
      depth=0; line=0; 
      setOStream(out);
    }
    void setOStream(std::ostream *stream);
    void store(const TSerializable*);
    
    void indent();
    void attribute(const std::string&);
    void type(const std::string&);
    void value(const std::string&);

    /**
     * start a new group
     */
    void startGroup();

    /**
     * end group
     */
    void endGroup();
    
    void writeQuoted(const char *p, unsigned n);
    void writeQuoted(const std::string &s) {
      writeQuoted(s.c_str(), s.size());
    }

  protected:    
    unsigned depth;
    unsigned line;
    unsigned gline;
};

class TObjectStore
{
    struct TCompare {
      bool operator()(const char *a, const char *b) const {
        return strcmp(a, b)<0;
      }
    };
    typedef std::map<const char*,TSerializable*,TCompare> TSerializableBuffer;
    TSerializableBuffer buffer;
  public:
    ~TObjectStore() { unregisterAll(); }
    void registerObject(TSerializable *obj);
    bool isRegistered(const std::string &type) const;
    void unregisterAll();
    TSerializable* clone(const std::string &type);
};

TObjectStore& getDefaultStore();

class TInObjectStream:
  public TATVParser, public TATVInterpreter
{
    TObjectStore *store;
  public:
    TInObjectStream(std::istream *stream = NULL, TObjectStore *store=NULL);
    TSerializable* restore();
    
    bool interpret(TATVParser &p);
    TSerializable *obj;
    
    TSerializable* clone(const std::string &type) {
      if (store) {
        if (isDebug())
          std::cerr << "cloned " << type << std::endl;
        return store->clone(type);
      }
      return NULL;
    }
};

class TCloneable
{
  public:
    virtual TCloneable* clone() const = 0;
};

class TSerializable:
  public TCloneable, public TATVInterpreter
{
  private:
    bool interpret(TATVParser &p) { return restore(static_cast<TInObjectStream&>(p)); }
  public:
    virtual const char * getClassName() const = 0;
    virtual void store(TOutObjectStream&) const;
    virtual bool restore(TInObjectStream&);
};

/**
 * A macro to ease the declaration of TSerializable derived classes.
 *
 * There a 4 methods which must be implemented for each class, and most
 * of the required definition for these methods is identical to using
 * this macro may help you to prevent making error and avoid typing too
 * much.
 *
 * An example:
 * \pre
class TResource:
public TSerializable
{  
  typedef TSerializable super;
  SERIALIZABLE_INTERFACE(toad::, TResource)
    
  string test;
}
   
void
TResource::store(TOutObjectStream &out) const
{
  ::store(out, "test", test);
}

bool
TResource::restore(TInObjectStream &in)
{
  if (
    ::restore(in, "test", &test) ||
    super::restore(in)
  ) return true;      
  ATV_FAILED(in);
  return false;
}
           
   \endpre
 *
 */
#define SERIALIZABLE_INTERFACE(PREFIX, CLASS) \
  public:\
    TCloneable* clone() const { return new CLASS(*this); }\
    const char * getClassName() const { return #PREFIX #CLASS ;} \
  protected:\
    void store(TOutObjectStream&) const;\
    bool restore(TInObjectStream&);

/**
 * A variant of SERIALIZABLE_INTERFACE but with public store and
 * restore method. Used ie. for TFigureModel and TFGroup where
 * group is a wrapper class for TFigureModel.
 */
#define SERIALIZABLE_INTERFACE_PUBLIC(PREFIX, CLASS) \
  public:\
    TCloneable* clone() const { return new CLASS(*this); }\
    const char * getClassName() const { return #PREFIX #CLASS ;} \
    void store(TOutObjectStream&) const;\
    bool restore(TInObjectStream&);

class TATVNullInterpreter:
  public TATVInterpreter
{
  public:
    bool interpret(TATVParser&);
};

extern TATVNullInterpreter null;

} // namespace atv

//#warning "should avoid implicit type conversation on the last parameter"
//#warning "should offer all C and C++ types"

// TSerializable

// reference => store without type name
void store(atv::TOutObjectStream &out, const atv::TSerializable &s);

// pointer => store with type name
void store(atv::TOutObjectStream &out, const atv::TSerializable *s);

// int
void store(atv::TOutObjectStream &out, int value);
bool restore(atv::TInObjectStream &in, int *value);

// unsigned
void store(atv::TOutObjectStream &out, unsigned value);
bool restore(atv::TInObjectStream &in, unsigned *value);

// float
void store(atv::TOutObjectStream &out, const float &value);
bool restore(atv::TInObjectStream &in, float *value);

// double
void store(atv::TOutObjectStream &out, const double &value);
bool restore(atv::TInObjectStream &in, double *value);

// bool
void store(atv::TOutObjectStream &out, bool value);
bool restore(atv::TInObjectStream &in, bool *value);

// char
void store(atv::TOutObjectStream &out, unsigned char value);
bool restore(atv::TInObjectStream &in, unsigned char *value);

// unsigned char
void store(atv::TOutObjectStream &out, unsigned char value);
bool restore(atv::TInObjectStream &in, unsigned char *value);

// string
void store(atv::TOutObjectStream &out, const std::string &value);
bool restore(atv::TInObjectStream &in, std::string *value);

// char* (as C string)
void store(atv::TOutObjectStream &out, const char *value);
void storeCStr(atv::TOutObjectStream &out, const char *value, unsigned n);
bool restore(atv::TInObjectStream &in, std::string *value);

// char*, size (binary as BASE64)
void storeRaw(atv::TOutObjectStream &out, const char *value, unsigned n);
bool restoreRaw(atv::TInObjectStream &in, char **value, unsigned *n);

inline void
storeRaw(atv::TOutObjectStream &out, const char * attr, const char *v, unsigned n) {
  out.indent();
  out << attr << " =";
  storeRaw(out, v, n);
}

inline bool
restoreRaw(atv::TInObjectStream &in, int pos, char **v, unsigned *n) {
  if (in.getPosition()!=pos)
    return false;
  return restoreRaw(in, v, n);
}

inline bool 
restoreRaw(atv::TInObjectStream &in, const char *attribute, char **v, unsigned *n) {
  if (in.attribute != attribute)
    return false;
  return restoreRaw(in, v, n);
}

// vector<T>

// list<T>

// map<string,T>

bool finished(atv::TInObjectStream &in);

template <class T>
void store(atv::TOutObjectStream &out, const char * attribute, const T value) {
  out.indent();
  out << attribute << " =";
  store(out, value);
}


template <class T>
bool restore(atv::TInObjectStream &in, int pos, T value) {
//  std::cerr << __FILE__ << ':' << __LINE__ << std::endl;
  if (in.getPosition()!=pos)
    return false;
  return restore(in, value);
}

template <class T>
bool restore(atv::TInObjectStream &in, const char *attribute, T value) {
//  std::cerr << __FILE__ << ':' << __LINE__ << std::endl;
  if (in.attribute != attribute)
    return false;
  return restore(in, value);
}

template <class T>
bool restorePtr(atv::TInObjectStream &in, T *value) {
  if (in.what != atv::ATV_GROUP) {
    return false;
  }
  if (in.type == "NULL") {
    *value = 0;
    in.setInterpreter(&atv::null);
    return true;
  }
  atv::TSerializable *s = in.clone(in.type);
  if (in.isVerbose())
    std::cerr << "created new object type " << in.type << std::endl;
  if (!s) {
    in.err << "unknown type '" << in.type << "'\n";
    ATV_FAILED(in);
    return false;
  }
  T t = dynamic_cast<T>(s);
  if (!t) {
    ATV_FAILED(in);
    delete s;
    return false;
  }
  *value = t;
  in.setInterpreter(s);
  return true;
}

template <class T>
bool restorePtr(atv::TInObjectStream &in, const char *attribute, T *value) {
  if (in.attribute != attribute)
    return false;
  return restorePtr(in, value);
}

namespace toad {
  using namespace atv;
} // namespace toad

#endif

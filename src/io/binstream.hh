/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef TBinaryFile
#define TBinaryFile TBinStream

#include <iostream>
#include <string>
#include <stdint.h>
#include <toad/debug.hh>
#include <toad/types.hh>

namespace toad {

using namespace std;

class TBinStreamBase
{
  public:
    TBinStreamBase() {
      _endian = LITTLE;
    }
    virtual ~TBinStreamBase();
  
    enum EEndian { LITTLE, BIG };
    void setEndian(EEndian e) {
      _endian = e;
    }
    EEndian getEndian() const {
      return _endian;
    }

    #ifdef TOAD_DEBUG
    static unsigned debug_output;
    #endif
  protected:
    EEndian _endian;
};

class TInBinStream:
  public virtual TBinStreamBase
{
  protected:
    istream *in;

  public:
    TInBinStream(istream*);

    ulong tellRead()  { return in->tellg(); } // get
    void seekRead(ulong p, ios::seekdir d=ios::beg) { in->seekg(p,d); }
    ulong readCount() { return in->gcount(); }
    bool eof() { return in->eof();}

    void read(signed char* buffer, size_t count);
    void read(unsigned char* buffer, size_t count);
    void readString(char*,int len);
    void readString(unsigned char *buf,int len)
      {return readString((char*)buf,len);}
    void readString(string*,int len);
    string readString(int len);
    bool compareString(const char*,int len);
    void unget(){in->unget();}
    
    signed int readSByte();             // 8 bit signed integer
    unsigned int readByte();            // 8 bit unsigned integer
    signed int readSWord();             // 16 bit signed integer
    uint16_t readWord();            	// 16 bit unsigned integer
    signed long readSDWord();           // 32 bit signed integer
    uint32_t readDWord();          	// 32 bit unsigned integer
    uint64_t readQWord();		// 64 bit unsigned integer
    double readDouble();                // 64bit IEEE 754 coded floating-point
    float readFloat();                  // 32bit IEEE 754 coded floating-point
    int readInt() { return readSDWord(); }
};

class TOutBinStream:
  public virtual TBinStreamBase
{
  protected:
    ostream *out;

  public:
    TOutBinStream(ostream*);

    void flush() { out->flush(); }
    ulong tellWrite() { return out->tellp(); }  // put
    void seekWrite(ulong p, ios::seekdir d=ios::beg) { out->seekp(p,d); }

    void write(const signed char* buffer, size_t count);
    void write(const unsigned char* buffer, size_t count);
    void write(signed char* buffer, size_t count);
    void write(unsigned char* buffer, size_t count);

    void writeSByte(signed int);        // 8bit signed integer
    void writeByte(unsigned int);       // 8bit unsigned integer
    void writeSWord(signed int);        // 16bit signed integer
    void writeWord(uint16_t);           // 16bit unsigned integer
    void writeSDWord(signed long);      // 32bit signed integer
    void writeDWord(uint32_t);          // 32bit unsigned integer
    void writeQWord(uint64_t);          // 64bit unsigned integer
    void writeDouble(double);           // 64bit IEEE 754 coded floating-point
    void writeFloat(float);             // 32bit IEEE 754 coded floating-point
    void writeString(const char*, unsigned len);
    void writeString(const char*);
    void writeString(const string&);

    void writeInt(int d) { writeDWord(d); };
};

class TBinStream:
  public TInBinStream, public TOutBinStream
{
  public:
    TBinStream(iostream *io):TInBinStream(io), TOutBinStream(io) {};
};

} // namespace toad

#endif

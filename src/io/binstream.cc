/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf@mark13.de>
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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <stdexcept>

#include <toad/io/binstream.hh>
#include <toad/debug.hh>
#include <stdio.h>

// #include "binstream.hh"

// - force problems with non IEEE 754 environments...
// - i guess it would be good to comile this file with GCCs -ffloat-store
//   option [MAH]
//#include <ieee754.h>

using namespace toad;

#ifdef TOAD_DEBUG
unsigned TBinStreamBase::debug_output = 0;
#endif

static const char* read_error = "InBinStream: read failed";

#define CHECK_IO if(!(*in)) {perror("read"); throw runtime_error(read_error);}

TBinStreamBase::~TBinStreamBase()
{
}

/**
 * This class might help you to read and write binary data from and to 
 * C++ iostreams.<BR>
 * After <CODE>SetEndian(TInBinStream::BIG)</CODE> it's even possible
 * to exchange data with Java. See <CODE>java.io.DataOutputStream</CODE> 
 * and <CODE>java.io.DataInputStream</CODE> in the Java reference for
 * more information.
 * <P>
 * <H3>Attention!</H3>
 * <UL>
 *   <LI>This class is very alpha and only tested on x86 CPUs but
 *       fixed-point methods should work on common little- and big-endian
 *       CPUs.
 *   <LI>Floating-point methods will fail on non IEEE 754 compliant CPUs
 * </UL>
 */
TInBinStream::TInBinStream(istream *stream)
{
  in  = stream;
#ifndef OLDLIBSTD
  in->exceptions(ios_base::badbit|
                 ios_base::eofbit|
                 ios_base::failbit);
#else
  in->exceptions(ios::badbit|
                 ios::eofbit|
                 ios::failbit);
#endif
  
//  stream->seekg(0);
}

TOutBinStream::TOutBinStream(ostream *stream)
{
  out = stream;
//  stream->seekp(0);
}

void TOutBinStream::write(const signed char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(const unsigned char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(signed char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(unsigned char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::writeString(const char* str)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, strlen(str));
  #endif
  out->write(str, strlen(str));
}

void TOutBinStream::writeString(const char* str, unsigned len)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, strlen(str));
  #endif
  out->write(str,len);
}

void TOutBinStream::writeString(const string &s)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: string of %i bytes\n", this, s.size());
  #endif
  out->write(s.c_str(), s.size());
}

// Byte
//---------------------------------------------------------------------------
void 
TOutBinStream::writeSByte(signed int d)
{
  if (d<-128 || d>127) {
    cerr << __PRETTY_FUNCTION__ << ": overflow" << endl;
  }
  unsigned int v;
  if (d<0) {
    v = 256+d;
  } else {
    v = d;
  }
  writeByte(v);
}

signed int 
TInBinStream::readSByte()
{
  unsigned v = readByte();
  if (v>127) {
    return -(256-v);
  }
  return v;
}

void 
TOutBinStream::writeByte(unsigned int d)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: byte %02x\n", this, d);
  #endif
  out->put(d);
}

unsigned int 
TInBinStream::readByte()
{
  unsigned char buffer;
  in->read((char*)&buffer,1);
  return buffer;
}

// Word
//---------------------------------------------------------------------------
void 
TOutBinStream::writeSWord(signed int d)
{
  if (d<-32768 || d>32767) {
    cerr << __PRETTY_FUNCTION__ << ": overflow" << endl;
  }
  unsigned v;
  if (d<0) {
    v = 65535U - (unsigned)(-d);
    v++;
  } else {
    v = d;
  }
  writeWord(v);
}

signed int 
TInBinStream::readSWord()
{
  unsigned d = readWord();
  if (d>32767U) {
    return -( (int)(65535U-d) ) - 1;
  }
  return d;
}

void 
TOutBinStream::writeWord(unsigned int v)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: word %04x\n", this, v);
  #endif
  char buffer[2];
  switch(_endian) {
    case LITTLE:
      buffer[0]=v&255;
      buffer[1]=(v>>8)&255;
      break;
    case BIG:
      buffer[1]=v&255;
      buffer[0]=(v>>8)&255;
      break;
  }
  out->write(buffer,2);
}

unsigned int 
TInBinStream::readWord()
{
  unsigned v;
  unsigned char buffer[2];
  in->read((char*)buffer,2);
  CHECK_IO
  switch(_endian) {
    case LITTLE:
      v = buffer[0] + 
          (buffer[1]<<8);
      break;
    case BIG:
      v = buffer[1] +
          (buffer[0]<<8);
      break;
  }
  return v;
}

// double word
//---------------------------------------------------------------------------
void 
TOutBinStream::writeSDWord(signed long d)
{
  if (d<-2147483647L || d>2147483647L) {
    cerr << __PRETTY_FUNCTION__ << ": overflow" << endl;
  }
  unsigned long v;
  if (d<0) {
    v = 4294967295UL - (unsigned long)(-d);
    v++;
  } else {
    v = d;
  }
  writeDWord(v);
}

signed long 
TInBinStream::readSDWord()
{
  unsigned long d = readDWord();
  if (d>4294967295UL) {
    return -( (unsigned)(4294967295UL-d) ) - 1;
  }
  return d;
}

void TOutBinStream::writeDWord(unsigned long v)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %08x: dword %08lx\n", this, v);
  #endif
  char buffer[4];
  switch(_endian) {
    case LITTLE:
      buffer[0]=v&255;
      buffer[1]=(v>>8)&255;
      buffer[2]=(v>>16)&255;
      buffer[3]=(v>>24)&255;
      break;
    case BIG:
      buffer[3]=v&255;
      buffer[2]=(v>>8)&255;
      buffer[1]=(v>>16)&255;
      buffer[0]=(v>>24)&255;
      break;
  }
  out->write(buffer, 4);
}

unsigned long 
TInBinStream::readDWord()
{
  unsigned long v;
  unsigned char buffer[4];
  in->read((char*)buffer, 4);
  CHECK_IO
  switch(_endian) {
    case LITTLE:
      v = buffer[0] + 
          (buffer[1]<<8) +
          (buffer[2]<<16) +
          (buffer[3]<<24);
          break;
    case BIG:
      v = buffer[3] + 
          (buffer[2]<<8) +
          (buffer[1]<<16) +
          (buffer[0]<<24);
          break;
  }
  return v;
}

//---------------------------------------------------------------------------

void 
TInBinStream::read(signed char* buffer, size_t count)
{
  in->read((char*)buffer, count);
  CHECK_IO
}

void 
TInBinStream::read(unsigned char* buffer, size_t count)
{
  in->read((char*)buffer, count);
  CHECK_IO
}

void 
TInBinStream::readString(string *str,int len)
{
  if (len==0) return;
  char buffer[len+1];
  in->read(buffer,len);
  CHECK_IO
  (*str).erase();
  (*str).append(buffer,len);
}

void 
TInBinStream::readString(char *str,int len)
{
  in->read(str, len);
  CHECK_IO
}

string 
TInBinStream::readString(int len)
{
  char buffer[len+1];
  in->read(buffer,len);
  CHECK_IO
  string s;
  s.append(buffer,len);
  return s;
}

bool 
TInBinStream::compareString(const char* s,int len)
{
  char buffer[len+1];
  in->read(buffer, len);
  CHECK_IO
  return strncmp(buffer,s,len)==0?true:false;
}

//---------------------------------------------------------------------------

#if 0

double TInBinStream::ReadDouble()
{
  union bug {
    ieee754_double ieee;
    byte buffer[8];
  } data;
  in->read(data.buffer, 8);
  CHECK_IO
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  if (_endian==BIG) {
  #else
  if (_endian==LITTLE) {
  #endif
    byte a;
    a = data.buffer[0]; data.buffer[0] = data.buffer[7]; data.buffer[7] = a;
    a = data.buffer[1]; data.buffer[1] = data.buffer[6]; data.buffer[6] = a;
    a = data.buffer[2]; data.buffer[2] = data.buffer[5]; data.buffer[5] = a;
    a = data.buffer[3]; data.buffer[3] = data.buffer[4]; data.buffer[4] = a;
  } 
  return data.ieee.d;
}

void TOutBinStream::WriteDouble(double v)
{
  union bug {
    ieee754_double ieee;
    byte buffer[8];
  } data;
  
  data.ieee.d = v;
  
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  if (_endian==BIG) {
  #else
  if (_endian==LITTLE) {
  #endif
    byte a;
    a = data.buffer[0]; data.buffer[0] = data.buffer[7]; data.buffer[7] = a;
    a = data.buffer[1]; data.buffer[1] = data.buffer[6]; data.buffer[6] = a;
    a = data.buffer[2]; data.buffer[2] = data.buffer[5]; data.buffer[5] = a;
    a = data.buffer[3]; data.buffer[3] = data.buffer[4]; data.buffer[4] = a;
  }
  out->write(data.buffer, 8);
}

//---------------------------------------------------------------------------

float TInBinStream::ReadFloat()
{
  union bug {
    ieee754_float ieee;
    byte buffer[4];
  } data;
  in->read(data.buffer, 4);
  CHECK_IO
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  if (_endian==BIG) {
  #else
  if (_endian==LITTLE) {
  #endif
    byte a;
    a = data.buffer[0]; data.buffer[0] = data.buffer[3]; data.buffer[3] = a;
    a = data.buffer[1]; data.buffer[1] = data.buffer[2]; data.buffer[2] = a;
  }
  return data.ieee.f;
}

void TOutBinStream::WriteFloat(float v)
{
  union bug {
    ieee754_float ieee;
    byte buffer[4];
  } data;
  
  data.ieee.f = v;
  
  #if __BYTE_ORDER == __LITTLE_ENDIAN
  if (_endian==BIG) {
  #else
  if (_endian==LITTLE) {
  #endif
    byte a;
    a = data.buffer[0]; data.buffer[0] = data.buffer[3]; data.buffer[3] = a;
    a = data.buffer[1]; data.buffer[1] = data.buffer[2]; data.buffer[2] = a;
  }
  
  out->write(data.buffer, 4);
}

#endif

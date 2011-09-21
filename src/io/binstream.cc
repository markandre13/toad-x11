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
 * Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, 
 * MA  02111-1307,  USA
 */

#include <stdexcept>

#include <toad/io/binstream.hh>
#include <toad/debug.hh>
#include <cstdio>
#include <cstring>

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
 * After <CODE>setEndian(TInBinStream::BIG)</CODE> it's also possible
 * to exchange data with Java. See <CODE>java.io.DataOutputStream</CODE> 
 * and <CODE>java.io.DataInputStream</CODE> in the Java reference for
 * more information.
 * <P>
 * <H3>Attention!</H3>
 * The implementation here isn't very fast. It's just here to get the job
 * done.
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
    printf("TOutBinStream %p: string of %zi bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(const unsigned char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(signed char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::write(unsigned char* buffer, size_t count)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, count);
  #endif
  out->write((char*)buffer, count);
}

void TOutBinStream::writeString(const char* str)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, strlen(str));
  #endif
  out->write(str, strlen(str));
}

void TOutBinStream::writeString(const char* str, unsigned len)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, strlen(str));
  #endif
  out->write(str,len);
}

void TOutBinStream::writeString(const string &s)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: string of %zi bytes\n", this, s.size());
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
    printf("TOutBinStream %p: byte %02x\n", this, d);
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
TOutBinStream::writeWord(uint16_t v)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: word %04x\n", this, v);
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

uint16_t 
TInBinStream::readWord()
{
  uint16_t v;
  unsigned char buffer[2];
  in->read((char*)buffer,2);
  CHECK_IO
  switch(_endian) {
    case LITTLE:
      v = buffer[0] + 
          (static_cast<uint16_t>(buffer[1])<<8);
      break;
    case BIG:
      v = buffer[1] +
          (static_cast<uint16_t>(buffer[0])<<8);
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

void
TOutBinStream::writeDWord(uint32_t v)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: dword %08x\n", this, (unsigned)v);
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

uint32_t
TInBinStream::readDWord()
{
  uint32_t long v;
  unsigned char buffer[4];
  in->read((char*)buffer, 4);
  CHECK_IO
  switch(_endian) {
    case LITTLE:
      v = buffer[0] + 
          (static_cast<uint32_t>(buffer[1])<<8) +
          (static_cast<uint32_t>(buffer[2])<<16) +
          (static_cast<uint32_t>(buffer[3])<<24);
          break;
    case BIG:
      v = buffer[3] + 
          (static_cast<uint32_t>(buffer[2])<<8) +
          (static_cast<uint32_t>(buffer[1])<<16) +
          (static_cast<uint32_t>(buffer[0])<<24);
          break;
  }
  return v;
}

void
TOutBinStream::writeQWord(uint64_t v)
{
  #ifdef TOAD_DEBUG
  if (debug_output)
    printf("TOutBinStream %p: dword %08lx\n", this, (long unsigned)v);
  #endif
  char buffer[8];
  switch(_endian) {
    case LITTLE:
      buffer[0]=v&255;
      buffer[1]=(v>>8)&255;
      buffer[2]=(v>>16)&255;
      buffer[3]=(v>>24)&255;
      buffer[4]=(v>>32)&255;
      buffer[5]=(v>>40)&255;
      buffer[6]=(v>>48)&255;
      buffer[7]=(v>>56)&255;
      break;
    case BIG:
      buffer[7]=v&255;
      buffer[6]=(v>>8)&255;
      buffer[5]=(v>>16)&255;
      buffer[4]=(v>>24)&255;
      buffer[3]=(v>>32)&255;
      buffer[2]=(v>>40)&255;
      buffer[1]=(v>>48)&255;
      buffer[0]=(v>>56)&255;
      break;
  }
  out->write(buffer, 8);
}

uint64_t
TInBinStream::readQWord()
{
  uint64_t v;
  unsigned char buffer[8];
  in->read((char*)buffer, 8);
  CHECK_IO
  switch(_endian) {
    case LITTLE:
      v = buffer[0] + 
          (static_cast<uint64_t>(buffer[1])<<8) +
          (static_cast<uint64_t>(buffer[2])<<16) +
          (static_cast<uint64_t>(buffer[3])<<24) +
          (static_cast<uint64_t>(buffer[4])<<32) +
          (static_cast<uint64_t>(buffer[5])<<40) +
          (static_cast<uint64_t>(buffer[6])<<48) +
          (static_cast<uint64_t>(buffer[7])<<56);
          break;
    case BIG:
      v = buffer[7] + 
          (static_cast<uint64_t>(buffer[6])<<8) +
          (static_cast<uint64_t>(buffer[5])<<16) +
          (static_cast<uint64_t>(buffer[4])<<24) +
          (static_cast<uint64_t>(buffer[3])<<32) +
          (static_cast<uint64_t>(buffer[2])<<40) +
          (static_cast<uint64_t>(buffer[1])<<48) +
          (static_cast<uint64_t>(buffer[0])<<56);
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
// public domain IEEE 754 unpack/unpack functions by Brian "Beej Jorgensen" Hall
// just in case the machine we're running on doesn't do IEEE 754
//---------------------------------------------------------------------------

#define pack754_32(f) (pack754((f), 32, 8))
#define pack754_64(f) (pack754((f), 64, 11))
#define unpack754_32(i) (unpack754((i), 32, 8))  
#define unpack754_64(i) (unpack754((i), 64, 11))
 
static uint64_t
pack754(long double f, unsigned bits, unsigned expbits)
{
  long double fnorm;
  int shift;
  long long sign, exp, significand;
  unsigned significandbits = bits - expbits - 1; // -1 for sign bit

  if (f == 0.0) return 0; // get this special case out of the way 
 
  // check sign and begin normalization
  if (f < 0) { sign = 1; fnorm = -f; }
  else { sign = 0; fnorm = f; }

  // get the normalized form of f and track the exponent
  shift = 0;
  while(fnorm >= 2.0) { fnorm /= 2.0; shift++; }
  while(fnorm < 1.0) { fnorm *= 2.0; shift--; }
  fnorm = fnorm - 1.0;      

  // calculate the binary form (non-float) of the significand data
  significand = fnorm * ((1LL<<significandbits) + 0.5f);

  // get the biased exponent
  exp = shift + ((1<<(expbits-1)) - 1); // shift + bias

  // return the final answer  
  return (sign<<(bits-1)) | (exp<<(bits-expbits-1)) | significand;
}

static long double
unpack754(uint64_t i, unsigned bits, unsigned expbits)
{
    long double result;
    long long shift;
    unsigned bias;
    unsigned significandbits = bits - expbits - 1; // -1 for sign bit

    if (i == 0) return 0.0;

    // pull the significand
    result = (i&((1LL<<significandbits)-1)); // mask
    result /= (1LL<<significandbits); // convert back to float
    result += 1.0f; // add the one back on

    // deal with the exponent
    bias = (1<<(expbits-1)) - 1;
    shift = ((i>>significandbits)&((1LL<<expbits)-1)) - bias;
    while(shift > 0) { result *= 2.0; shift--; }
    while(shift < 0) { result /= 2.0; shift++; }

    // sign it
    result *= (i>>(bits-1))&1? -1.0: 1.0;

    return result;
}

float
TInBinStream::readFloat()
{
  return unpack754_32(readDWord());
}

void
TOutBinStream::writeFloat(float v)
{
  writeDWord(pack754_32(v));
}

double
TInBinStream::readDouble()
{
  return unpack754_64(readDWord());
}

void
TOutBinStream::writeDouble(double v)
{
  writeDWord(pack754_64(v));
}

/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.org>
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

#include <toad/io/urlstream.hh>

#include <cstdio>
#include <cassert>

#ifndef OLDLIBSTD
#  include <streambuf>
#else
#  include <streambuf.h>
#  define ios_base ios
#endif
#include <sstream>
#include <stdexcept>

#include <map>

#include <sys/types.h>
#ifdef __X11__
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif

#include <unistd.h>
#include <fcntl.h>

#include <errno.h>

using namespace toad;

namespace {

/**
 * A streambuf class for UNIX file descriptors.
 *
 * This class can be used to access files and sockets.
 *
 * \todo
 *   \li
 *     memoryfiles aren't handled effective with sstream/stringstream
 *   \li
 *     Usage of STL might be not standard conform.
 */
class fdbuf:
  public std::streambuf
{
  #ifdef OLDLIBSTD
  typedef char        char_type;
  typedef int         int_type;
  typedef streampos   pos_type;
  typedef streamoff   off_type;
  #endif

  public:
    fdbuf(int fd, ios_base::openmode __mode);
    ~fdbuf() {
      fflush(cfile);
      fclose(cfile);
    }
    
    int fgetc() {
      return ::fgetc(cfile);
    }
    
  protected:
    FILE *cfile;
    
    // various
    int sync(void);

    pos_type seekoff(off_type off, ios_base::seekdir way,
       ios_base::openmode mode = ios_base::in | ios_base::out);
    pos_type seekpos(pos_type pos,
       ios_base::openmode mode = ios_base::in | ios_base::out);

    
    // input
    streamsize showmanyc();
    int_type uflow(void);
    int_type underflow(void);
    streamsize xsgetn(char_type* s, streamsize n);
    int_type pbackfail(int_type c=EOF);

    // output
    int_type overflow(int_type c);
    streamsize xsputn(const char_type* s, streamsize n);
};

} // namespace

fdbuf::fdbuf(int fd, ios_base::openmode om)
{
  if (om==ios::out) {
    cfile = fdopen(fd, "wb");
  } else
  if (om==ios::in) {
    cfile = fdopen(fd, "rb");
  } else {
    cerr << __FILE__ << ":" << __LINE__ << ": unsupported mode" << endl;
    exit(1);
  }
}

fdbuf::pos_type 
fdbuf::seekoff(off_type off, 
               ios_base::seekdir way, 
               ios_base::openmode /*mode*/)
{
  if (cfile) {
    int type;
    switch(way) {
      case ios_base::beg:
        type=SEEK_SET;
        break;
      case ios_base::cur:
        type=SEEK_CUR;
        break;
      case ios_base::end:
        type=SEEK_END;
        break;
    }
    fseek(cfile, off, type);
  }
  return pos_type(off_type(ftell(cfile)));
}

fdbuf::pos_type 
fdbuf::seekpos(pos_type pos, 
               ios_base::openmode /*mode*/)
{
  if (cfile)
    fseek(cfile, pos, SEEK_SET);
  return pos_type(off_type(ftell(cfile)));
}

int 
fdbuf::sync(void)
{
  if (cfile)
    return fflush(cfile);
  return 0;
}

// input
//----------------------------------------------------------------------------

streamsize
fdbuf::showmanyc(void)
{
//  cout << "[showmanyc]" << endl;
  if (!feof(cfile))
    return 4096;
//  cout << "no more chars" << endl;
  return EOF;
}

fdbuf::int_type
fdbuf::underflow(void)
{
//  cout << "[underflow]" << endl;
  return ::fgetc(cfile);
}

fdbuf::int_type
fdbuf::uflow(void)
{
//  cout << "[uflow]" << endl;
  return ::fgetc(cfile);
}

streamsize 
fdbuf::xsgetn(char_type* s, streamsize n)
{
//  cout << "[xsgetn n="<<n<<"]" << endl;
  return ::fread(s, 1, n, cfile);
}

int
fdbuf::pbackfail(int c)
{
  return ::ungetc(c, cfile);
}

// output
//----------------------------------------------------------------------------

/**
 * Called to output a single character.
 */
fdbuf::int_type
fdbuf::overflow(int_type c)
{
  return fputc(c, cfile);
}


/**
 * Called to output more than one character.
 */
streamsize 
fdbuf::xsputn(const char_type* s, streamsize n)
{
  return fwrite(s, 1, n, cfile);
}



void 
urlstreambase::set_buffer(int fd, ios_base::openmode om)
{
  close();
  if (om==ios::out) {
    fb = new fdbuf(fd, ios::out);
  }
  if (om==ios::in) {
    fb = new fdbuf(fd, ios::in);
  }
  if (fb)
    urlbase_init(fb);
}

void 
urlstreambase::set_buffer(streambuf* fb)
{
  close();
  this->fb = fb;
  urlbase_init(fb);
}

void
urlstreambase::close()
{
  if (fb)
    delete fb;
  fb = NULL;
}

void urlstreambase::parse(const string &url)
{
  protocol = P_NONE;
  port = 0;
  hostname.erase();
  filename.erase();
  this->url = url;

  struct TType {
    EProtocol protocol;
    const char *name;
    bool  with_hostname;
  } typetable[] = {
    { P_MEMORY, "memory", false },
    { P_FILE,   "file",   false },
#ifdef __X11__
    { P_HTTP,   "http",   true },
    { P_FTP,    "ftp",    true },
#endif
  };
  size_t p,l;

  // get protocol
  //--------------------
  p = url.find_first_not_of(" \t");
  if (p==string::npos)
    return;
  l = url.size() - p;
  unsigned type;
  for(type=0; type<sizeof(typetable)/sizeof(TType); type++) {
    size_t tl = strlen(typetable[type].name);
    if ( tl+3 <= l &&
         strncasecmp(typetable[type].name, url.c_str()+p, tl) == 0 &&
         strncmp("://", url.c_str()+p+tl, 3)==0 )
    {
      protocol = typetable[type].protocol;
      p += tl+3;
      break;
    }
  }
  
  // get hostname
  //--------------------
  if (protocol==P_HTTP)
    port=80;
  
  if (protocol!=P_NONE &&
      typetable[type].with_hostname) {
    l = url.substr(p).find_first_of(":/ \t");
    if (l==string::npos) {
      hostname = url.substr(p);
      return;
    }
    hostname = url.substr(p,l);
    p+=l;
    
    if (url[p]==':') {
      p++;
      l = url.substr(p).find_first_of("/ \t");
      sscanf(url.substr(p,l).c_str(), "%i", &port);
      if (l==string::npos)
        return;
      p+=l;
    }
  }

  if (protocol!=P_NONE)
    filename = url.substr(p);
  else
    filename = url;
}

bool
urlstreambase::iopen()
{
  switch(protocol) {
    case P_NONE:
    case P_FILE:
      return iopen_file();
      break;
#ifdef __X11__
    case P_HTTP:
      return iopen_http();
      break;
#endif
    case P_MEMORY:
      return iopen_memory();
      break;
    default:
      ;
      // throw runtime_error("input isn't supported for this protocol");
  }
  return false;
}

bool
urlstreambase::oopen()
{
  switch(protocol) {
    case P_NONE:
    case P_FILE:
      return oopen_file();
      break;
    default:
      ;
      // throw runtime_error("output isn't supported for this protocol");
  }
  return false;
}

#ifdef __X11__
bool
urlstreambase::iopen_http()
{
  string error;
  string cmd;
  char sport[256];
  snprintf(sport, 255, "%d", port);
  string fn = hostname+":"+sport;
  fdbuf *fd;
  unsigned r, n;

  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    error = "couldn't create socket";
    goto error1;
  }

  sockaddr_in name;
  in_addr ia;
  if (inet_aton(hostname.c_str(), &ia)!=0) {
    name.sin_addr.s_addr = ia.s_addr;
  } else {
    struct hostent *hostinfo;
    hostinfo = gethostbyname(hostname.c_str());
    if (hostinfo==0) {
      error = "couldn't resolve hostname of " + hostname;
      goto error3;
    }
    name.sin_addr = *(struct in_addr *) hostinfo->h_addr;
  }

  name.sin_family = AF_INET;
  name.sin_port   = htons(port);
  
  if (connect(sock, (sockaddr*) &name, sizeof(sockaddr_in)) < 0) {
    error = "couldn't connect to " + fn;
    goto error4;
  }

  cmd = "GET ";
  cmd+=filename;
  cmd+=" HTTP/1.1\r\n";
  cmd+="Host: ";
  cmd+=hostname;
  cmd+="\r\n\r\n";
  if (write(sock, cmd.c_str(), cmd.size())!=(int)cmd.size()) {
    error = "failed to send HTTP request to " + fn;
    goto error5;
  }
  set_buffer(sock, ios::in);

  // skip HTTP header
  fd = dynamic_cast<fdbuf*>(fb);
  assert(fd!=NULL);
  
  r=0;
  n=0;
  while(true) {
    if (n==2)
      break;
    int c = fd->fgetc();
    if (c==EOF)
      break;
    if (c=='\n') {
//cerr << "\\n";
      ++n;
      continue;
    }
    if (c=='\r') {
//cerr << "\\r";
      ++r;
      continue;
    }
//cerr << (char)c;
    r = n = 0;
  }

  return true;

error5:
error4:
error3:
  ::close(sock);
error1:
  // throw runtime_error(error.c_str());
  return false;
}
#endif

bool
urlstreambase::iopen_file()
{
  close();
  int fd = ::open(filename.c_str(), O_RDONLY);
  if (fd==-1) {
    // error = "failed to open `"+url+"': "+ strerror(errno);
    return false;
  }
  set_buffer(fd, ios::in);
  return true;
}

bool
urlstreambase::oopen_file()
{
  close();
  int fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644 );
  if (fd==-1) {
    string error = "failed to open `"+url+"': "+ strerror(errno);
    return false;
  }
  set_buffer(fd, ios::out);
  return true;
}

namespace toad {
typedef map<const string, string> TMemoryFileSystem;
static TMemoryFileSystem memory_file_system;
}

void urlstreambase::saveMemoryFile(const string &name, 
                                   const char* data, unsigned len)
{
  memory_file_system[name].assign(data, len);
}

bool
urlstreambase::iopen_memory()
{
  close();
  TMemoryFileSystem::iterator p = memory_file_system.find(filename);
  if (p==memory_file_system.end()) {
    return false;
  }
  set_buffer(new stringbuf(p->second));
  return true;
}

void
iurlstream::urlbase_init(streambuf *fb)
{
  init(fb);
}

void
ourlstream::urlbase_init(streambuf *fb)
{
  init(fb);
}

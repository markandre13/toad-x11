/*
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2004 by Mark-André Hopf <mhopf@mark13.de>
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

#ifndef urlstreambase
#define urlstreambase urlstreambase

#include <iostream>
#include <string>

#include <toad/os.hh>

namespace toad {

using namespace std;

class urlstreambase
{
  public:
    static void saveMemoryFile(const string &name, 
                               const char* data, unsigned len);
  protected:
    urlstreambase() {
      protocol = P_NONE;
      fb = 0;
    }
    virtual ~urlstreambase() {
      close();
    }
    string url;
    enum EProtocol {
      P_NONE,
      P_MEMORY,
      P_FILE,
      P_HTTP,
      P_FTP
    } protocol;
    string hostname;
    int port;
    string filename;

    std::streambuf *fb;

    void parse(const string&);
    void iopen();
#ifdef __X11__
    void iopen_http();
#endif
    void iopen_file();
    void iopen_memory();
    
    void oopen();
    void oopen_file();

#if 0
    void set_sb(streambuf *new_sb, int new_fd = -1);
#endif

    void set_buffer(int fd, ios::openmode om);
    void set_buffer(streambuf*);
    void close();
    
    virtual void urlbase_init(streambuf*)=0;
};

class iurlstream:
  public urlstreambase, public istream
{
  public:
    iurlstream(const string &url):
      istream(NULL) 
    {
      parse(url);
      iopen();
    }
    iurlstream():
      istream(NULL)
    {}
    void open(const string &url) {
      parse(url);
      iopen();
    }
  protected:
    void urlbase_init(streambuf*);
};

class ourlstream:
  public urlstreambase, public ostream
{
  public:
    ourlstream(const string &url):
      ostream(NULL)
    {
      parse(url);
      oopen();
    }
    ourlstream():
      ostream(NULL)
    {}
    void open(const string &url) {
      parse(url);
      oopen();
    }
  protected:
    void urlbase_init(streambuf*);
};

// there ain't no such thing as an url stream for input and output

} // namespace toad

#endif

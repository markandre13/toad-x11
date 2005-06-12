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

/*
 *
 * Asynchronous I/O multiplexing
 *
 */

/**
 * \file select.cc
 *
 *  \todo
 *    \li 
 *      synchronisation
 *    \li
 *      after StopTimer() it must be safe to call StartTimer which I suppose
 *      not being possible right now
 *    \li
 *      there's a midnight bug
 *    \li
 *      remove the X11 specific parts and let TOAD register an TIOObserver
 */

#include <toad/os.hh>

#include <toad/toadbase.hh>
#include <toad/ioobserver.hh>
#include <toad/simpletimer.hh>
#include <vector>
#include <set>

#include <fcntl.h>
#include <sys/time.h>

#include <unistd.h>

using namespace toad;

enum {
  VMAS_READ,
  VMAS_WRITE,
  VMAS_EXCEPTION,
  VMAS_MAX
};

typedef void (*pmf)(TIOObserver*);
#define SET_VMAS(const, func) \
  _vmas_table[const] = (pmf)(this->*(&TIOObserver::func))
#define CHK_VMAS(const, func) \
  _vmas_table[const] != (pmf)(this->*(&TIOObserver::func))
static pmf _vmas_table[VMAS_MAX] = { NULL, };

namespace toad {
  bool debug_select = false;
}

typedef vector<TIOObserver*> TList;
static TList fd_list;
static TList fd_new;

static fd_set fd_set_rd, fd_set_wr, fd_set_ex;
static int fd_x11;
static int fd_max;

static struct timeval crnt_time;

void TOADBase::initIO(int x11)
{
  fd_x11 = x11;
  fd_max = x11+1;
  FD_ZERO(&fd_set_rd);
  FD_ZERO(&fd_set_wr);
  FD_ZERO(&fd_set_ex);
  FD_SET(fd_x11, &fd_set_rd);
}

// TSimpleTimer stuff (Part I, see Part II below)
//----------------------------------------------------------------
struct TSimpleTimer::less {
  bool operator()(const TSimpleTimer *l1, const TSimpleTimer *l2) const {
    return l1->_next.tv_sec  < l2->_next.tv_sec ||
         ( l1->_next.tv_sec == l2->_next.tv_sec && l1->_next.tv_usec < l2->_next.tv_usec);
  }
};

typedef set<TSimpleTimer*, TSimpleTimer::less> TSortedList;
static TSortedList _sorted_list;

#define DBM(A)

// wrapper for the C library `select' call
// handles X11, TIOObserver & TSimpleTimer
//----------------------------------------------------------------
void 
TOADBase::select()
{
#ifdef __X11__
  // add new io observers to the list
  //----------------------------------
  TList::iterator p;
  if (fd_new.size()>0) {
    p = fd_new.begin();
    while(p!=fd_new.end()) {
      int fd = (*p)->fd();
      if (debug_select)
        cerr << "select: adding checks for fd " << fd << endl;
      if (fd>=fd_max)
        fd_max = fd+1;
      int flag = 0;
      (*p)->_check(flag);
      if (flag&1) {
        if (debug_select)
          cerr << "select: adding fd " << fd << " for read check\n";
        FD_SET(fd, &fd_set_rd);
      }
      if (flag&2) {
        if (debug_select)
          cerr << "select: adding fd " << fd << " for write check\n";
        FD_SET(fd, &fd_set_wr);
      }
      if (flag&4) {
        if (debug_select)
          cerr << "select: adding fd " << fd << " for exception check\n";
        FD_SET(fd, &fd_set_ex);
      }
      p++;
    }
    fd_new.erase(fd_new.begin(), fd_new.end());
  }
  
  // select loop
  //-------------
  fd_set rd, wr, ex;
  
  while(true) {
    rd = fd_set_rd;
    wr = fd_set_wr;
    ex = fd_set_ex;

    int n;

    if (_sorted_list.empty()) {
      if (debug_select)
        cerr << "select: waiting for fd to become ready (fd_max=" << fd_max << ")\n";
      if (!bAppIsRunning) {
        cerr << "unexpected end of message loop [2]" << endl;
        return false;
      }
      n = ::select(fd_max, &rd, &wr, &ex, NULL);
    } else {
      // dispatch timer events
      gettimeofday(&crnt_time, NULL);
      TSortedList::iterator p,e;
      p = _sorted_list.begin();
      e = _sorted_list.end();

      while(p!=e && 
             (*p)->_next.tv_sec  < crnt_time.tv_sec ||
            ((*p)->_next.tv_usec < crnt_time.tv_usec && (*p)->_next.tv_sec==crnt_time.tv_sec))
      {
        TSimpleTimer *t = *p;

        if (t->_running) {
          t->_executing = true;
          t->tick();
          t->_executing = false;
        
          // set time for the next event
          while( t->_next.tv_sec  < crnt_time.tv_sec ||
                (t->_next.tv_usec < crnt_time.tv_usec && t->_next.tv_sec == crnt_time.tv_sec))
          {
            t->_next.tv_sec  += t->_interval.tv_sec;
            t->_next.tv_usec += t->_interval.tv_usec;
            if (t->_next.tv_usec >= 1000000L) {
              t->_next.tv_sec++;
              t->_next.tv_usec -= 1000000L;
            }
          }
        }

        // replace timer in list
        TSortedList::iterator last = p;
        p++;
        _sorted_list.erase(last);
        if (t->_running)
          _sorted_list.insert(t);
        if (p==e)
          break;
      }
      
      // calculate time for the next event
      TSimpleTimer *t = *_sorted_list.begin();
      struct timeval wait_time;
      if (_sorted_list.empty() ||
          t->_next.tv_sec < crnt_time.tv_sec ||
          (t->_next.tv_usec < crnt_time.tv_usec && t->_next.tv_sec == crnt_time.tv_sec))
      {
        wait_time.tv_sec = 0;
        wait_time.tv_usec = 0;
      } else {
        wait_time.tv_sec = t->_next.tv_sec  - crnt_time.tv_sec;
        wait_time.tv_usec= t->_next.tv_usec - crnt_time.tv_usec;
        if (wait_time.tv_usec<0) {
          wait_time.tv_sec--;
          wait_time.tv_usec+=1000000;
        }
      }
      flush();
      if (debug_select)
        cerr << "select: waiting for fd to become ready (fd_max=" << fd_max << ")\n";
      if (!bAppIsRunning) {
        cerr << "unexpected end of message loop [3]" << endl;
        return false;
      }
      n = ::select(fd_max, &rd, &wr, &ex, &wait_time);
    }
    
    if (debug_select)
      cerr << "select: got " << n << " valid fd's\n";

    if (n>=0) {
      p = fd_list.begin();
      while(p!=fd_list.end()) {
        int fd = (*p)->fd();
        if (debug_select)
          cerr << "select: checking fd " << fd << endl;
        if (FD_ISSET(fd, &rd)) {
          if (debug_select)
            cerr << "  rd\n";
          (*p)->canRead();
        }
        if (FD_ISSET(fd, &wr)) {
          if (debug_select)
            cerr << "  wr\n";
          (*p)->canWrite();
        }
        if (FD_ISSET(fd, &ex)) {
          if (debug_select)
            cerr << "  ex\n";
          (*p)->gotException();
        }
        p++;
      }
    }
    if (peekMessage()) {
      if (debug_select)
        cerr << "select: got x11 message\n";
      return;
    }
    flush();
  }
#endif
}

/**
 * \class toad::TIOObserver
 * A class for synchronous I/O multiplexing.
 *
 * Since TOAD already uses the <CODE>select</CODE> function, you
 * have to use <I>TIOObserver</I>.
 */

TIOObserver::TIOObserver()
{
  _fd = -1;
  if (!_vmas_table[0]) {
    SET_VMAS(VMAS_READ, canRead);
    SET_VMAS(VMAS_WRITE, canWrite);
    SET_VMAS(VMAS_EXCEPTION, gotException);
  }
}

/**
 * Creates a new asynchronous observer for file descriptor <VAR>fd</VAR>.
 */
TIOObserver::TIOObserver(int fd)
{
#ifdef __X11__
  if (!_vmas_table[0]) {
    SET_VMAS(VMAS_READ, canRead);
    SET_VMAS(VMAS_WRITE, canWrite);
    SET_VMAS(VMAS_EXCEPTION, gotException);
  }
  setFD(fd);
#endif
}

#ifdef __X11__
/**
 * Assign a file descriptor to the io observer.
 *
 * \param fd
 *   A value >= 0 will add the observer for the specified file
 *   descriptor to the message loop. A value < 0 will remove the
 *   observer from the messsage loop.
 */
void
TIOObserver::setFD(int fd)
{
  if (debug_select)
    cerr << "select: new fd " << fd << endl;

  // remove old FD from message loop
  if (_fd >= 0) {
    FD_CLR(_fd, &fd_set_rd);
    FD_CLR(_fd, &fd_set_wr);
    FD_CLR(_fd, &fd_set_ex);
    TList::iterator p;
    p = fd_new.begin();
    while(p!=fd_new.end()) {
      if (*p==this) {
        fd_new.erase(p);
        break;
      }
    }
    p++;
    p = fd_list.begin();
    while(p!=fd_list.end()) {
      if (*p==this) {
        fd_list.erase(p);
        break;
      }
      p++;
    }
  }

  _fd = fd;

  // add new FD to message loop
  if (_fd >= 0) {
    fcntl(fd, F_SETFL, O_NONBLOCK);
    fd_list.push_back(this);
    fd_new.push_back(this);
  }
}
#endif

void TIOObserver::_check(int &r)
{
  if (CHK_VMAS(VMAS_READ, canRead))
    r |= 1;
  if (CHK_VMAS(VMAS_WRITE, canWrite))
    r |= 2;
  if (CHK_VMAS(VMAS_EXCEPTION, gotException))
    r |= 4;
}

TIOObserver::~TIOObserver()
{
  if (debug_select)
    cerr << "select: delete fd " << _fd << endl;
  setFD(-1);
}

/**
 * Called when data is available on the file descriptor.
 */
void TIOObserver::canRead()
{
}

/**
 * Called when data can be written to the file descriptior.
 */
void TIOObserver::canWrite()
{
}

/**
 * Called when an exception occured on the file descriptor.
 */
void TIOObserver::gotException()
{
}

// TSimpleTimer stuff (Part II, as promised above)
//----------------------------------------------------------------
TSimpleTimer::~TSimpleTimer()
{
  // i should do a check here wether the timer is active...
}

/**
 * sec, usec specifies the interval time between calls to tick().<BR>
 * If <VAR>skip_first</VAR> is <CODE>false</CODE> the first call to tick() 
 * will happen almost at once. Otherwise the first tick will happen after 
 * sec, usec.
 * <P>
 * When called for a timer already running, only the interval will be
 * changed at the next tick.
 */
//-----------------------------------------------------------------------
void TSimpleTimer::startTimer(ulong sec, 
                              ulong usec, 
                              bool skip_first)
{
#ifdef __X11__
  while(usec>=(1000000UL)) {
    sec++;
    usec-=1000000UL;
  }
  
  _interval.tv_sec = sec;
  _interval.tv_usec= usec;

  if (_running)
    return;
  
  struct timeval crnt_time;
  gettimeofday(&crnt_time, NULL);
  _next.tv_usec = crnt_time.tv_usec;
  _next.tv_sec = crnt_time.tv_sec;
  
  if (skip_first) {
    _next.tv_sec  += _interval.tv_sec;
    _next.tv_usec += _interval.tv_usec;
    if (_next.tv_usec >= 1000000L) {
      _next.tv_sec++;
      _next.tv_usec -= 1000000L;
    }
  }

  _sorted_list.insert(this);
  _running = true;
#endif
}

/**
 * Stop a running timer.
 *
 * This method has no effect when the timer isn't running.
 */
void TSimpleTimer::stopTimer()
{
  // we can't erase the timer here from the list because it might
  // be referenced in `TOADBase::Select' right now
  _running = false;
}

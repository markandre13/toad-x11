#!/usr/bin/perl -w
#
# I wrote this one to simplify something,
# but after I've looked at it I forgot what.
#
# The text printed by the license function below is for this Perl 
# script and the output it creates.
#--------------------------------------------------------------------

use strict;

sub license() {
print<<EOT;
/* DO NOT EDIT - THIS IS A GENERATED FILE (EDIT CONNECT.HH.PM!)
 *
 * TOAD -- A Simple and Powerful C++ GUI Toolkit for the X Window System
 * Copyright (C) 1996-2003 by Mark-André Hopf <mhopf\@mark13.de>
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

#include <cstdlib>
#include <toad/debug.hh>

EOT
}

# print a connection node
#----------------------------------------------------------------------------
sub node
{
  my($tp, $vp, $ap, $dp, $ep, $cp1, $cp2, $mp, $bp, $ap1);
  SWITCH: {
    $_[6]==0 && do {
      $tp = "";		# template argument
      $ap = "";
      $vp = "";         # variable declaration
      $dp = "void";     # definition
      $ep = "";
      $cp1= "";         # constructor parameter 1
      $ap1= "";
      $cp2= "";         # constructor parameter 2
      $mp = "";         # method call argument
      $bp = "";
      last SWITCH; };
    $_[6]==1 && do {
      $tp = ", class U1";
      $ap = ", class V1";
      $vp = "  U1 _p1;\n  ";
      $dp = "U1";
      $ep = ", U1";
      $cp1= ", U1 p1";
      $ap1= ", V1 p1";
      $cp2= ", _p1(p1)";
      $mp = "_p1";
      $bp = ", p1";
      last SWITCH; };
    $_[6]==2 && do {
      $tp = ", class U1, class U2";
      $ap = ", class V1, class V2";
      $vp = "  U1 _p1; U2 _p2;\n  ";
      $dp = "U1, U2";
      $ep = ", U1, U2";
      $cp1= ", U1 p1, U2 p2";
      $ap1= ", V1 p1, V2 p2";
      $cp2= ", _p1(p1), _p2(p2)";
      $mp = "_p1, _p2";
      $bp = ", p1, p2";
      last SWITCH; };
  }

  print <<EOT;
template <class A$_[1]$tp>
class $_[0]:
  public TSignalLink
{
  protected:
    A($_[2]*_m)($dp);
  $_[4]$vp public:
    $_[0]($_[12]A($_[2]*m)($dp)$cp1):_m(m)$_[13]$cp2 {}
    void execute() { ($_[3]*_m)($mp); }$_[5]
};

/**
 * \\ingroup callback
 */
template <class R$_[11]$tp$ap>
TSignalLink* connect(TSignal &s, $_[8]R($_[7]*m)($dp)$ap1) {
  return s.add(new $_[0]<R$_[10]$ep>($_[9]m$bp));
}

/**
 * \\ingroup callback
 */
template <class R$_[11]$tp>
void disconnect(TSignal &s, $_[8]R($_[7]*m)($dp)) {
	$_[14]
}

EOT
}

# print a connection node for functions
#----------------------------------------------------------------------------
sub fnode
{
  node($_[0], "", "", "", "", "", $_[1], "", "", "", "", "", "", "",
	"s.remove( (void(*)())m );");
}

# print a connection node for objects & methods
#----------------------------------------------------------------------------
sub onode
{
  node($_[0], 
       ", class T",
       "T::", 
       "_o->", 
       "  T *_o;\n  ", 
       "\n    virtual void* objref() {return (void*)_o;}\n    virtual TMethod metref() {return (TMethod)_m;}",
       $_[1],
       "T2::",
       "T1 *o, ",
       "o, ",
       ", T1",
       ", class T1, class T2",
       "T *o, ",
       ", _o(o)",
	"T2 *co=o; s.remove( (void*)o, (TSignalLink::TMethod)m);");
}


sub main()
{
  my $i;
  license();
  print "\n";
  print <<EOT;
#ifndef TSignal
#define TSignal TSignal

namespace toad {  // connect.hh

/**
 * \\ingroup callback
 */
#define CONNECT(S, O, M, ARGS...) { typedef typeof(*O) T; connect(S, O, &T::M , ## ARGS ); }

/**
 * \\ingroup callback
 */
#define DISCONNECT_ALL(S) S.remove();

/**
 * \\ingroup callback
 */
#define DISCONNECT_OBJ(S, O) S.remove((void*)O);

/**
 * \\ingroup callback
 */
#define DISCONNECT_FNC(S, O) S.remove((void*)O);

/**
 * \\ingroup callback
 */
#define DISCONNECT(S, O, M) { typedef typeof(*O) T; S.remove((void*)O, (TSignalLink::TMethod)&T::M); }
  
/**
 * \\ingroup callback
 *
 * Objects of this type are needed to remove single callbacks from a signal.
 *
 * Usually returned when a callback is connected to a signal.
 */
class TSignalLink
{
    class TClass;
  public:
    typedef void(TClass::*TMethod)(void);
    TSignalLink();
    virtual ~TSignalLink();
    virtual void execute() = 0;
    virtual void* objref();
    virtual TMethod metref();
    TSignalLink *next;
    bool lock:1;
    bool dirty:1;    
};

/**
 * \\ingroup callback
 *
 * This class is the base to store pointers to callback functions.
 */
class TSignal
{
    /* this class isn't itended to be copied */
    TSignal(const TSignal&) {};
    TSignal& operator=(const TSignal&) {return *this;}
  public:
    TSignal();
    ~TSignal();
    TSignalLink* add(TSignalLink*);
		bool isConnected() const { return _list!=NULL; }
    void remove();
    void remove(TSignalLink*);
    void remove(void*);
    void remove(void(*)(void));
    void remove(void*, TSignalLink::TMethod);
    bool trigger();
    bool delayedTrigger();
    bool operator()() { return trigger(); }
    
    void lock();
    void unlock();
    void print();
#ifdef TOAD_SECURE
    unsigned delayedtrigger;
#endif
  protected:
    TSignalLink *_list;
};

/**
 * \\ingroup callback
 *
 * Remove all callbacks from the signal.
 */
inline void disconnect(TSignal &s) {
	s.remove();
}

/**
 * \\ingroup callback
 *
 * Remove the given link from the signal.
 */
template <class T>
inline void disconnect(TSignal &s, T *n) {
	s.remove(n);
}

// help template for connect_value, connect_value_of, ...
//-------------------------------------------------------
template <class S, class D>
class GSignalCodeLink:
	public TSignalLink
{
	protected:
		typedef GSignalCodeLink<S,D> TThis;
		typedef S* TS;
		typedef D* TD;
		TS src;
		TD dst;
	public:
		GSignalCodeLink(TS s, TD d): src(s), dst(d) {}
};

/**
 * \\ingroup callback
 *
 * A simplified variant of 'connect_value_of' for the GNU C++ Compiler.
 *
 * Connect method with method/attribut:
 * \\pre
 * CONNECT_VALUE_OF(signal,
 *   destination object, destination method,
 *   source object, source method or attribut)
 * \\endpre
 */
#define CONNECT_VALUE_OF(SIG, D, DM, S, SM)\\
{\\
	typedef typeof(*S) TS;\\
	typedef typeof(*D) TD;\\
	class unnamed:\\
		public GSignalCodeLink<TS, TD>\\
	{\\
	    typedef GSignalCodeLink<TS, TD> TThis;\\
		public:\\
			unnamed(TS s, TD d): TThis(s,d) {}\\
			void execute() {\\
				this->dst->DM(this->src->SM);\\
			}\\
	};\\
	SIG.add(new unnamed(S, D));\\
}

/**
 * \\ingroup callback
 *
 * Add a callback that takes one parameter. The value for this parameter
 * is delivered from another callback.
 *
 * \\param sig Signal 
 * \\param d     Destination's object of the call.
 * \\param dm    Destination's method of the call which takes one parameter.
 * \\param s     Source's object with a delivering the parameter.
 * \\param sm    Source's method delivering the parameter when calling the
 *              destination method.
 */
template <class D, class S, class R, class V>
TSignalLink* connect_value_of(TSignal &sig, D *d, R(D::*dm)(V), S *s, V(S::*sm)())
{
	struct unnamed:
		public GSignalCodeLink<S, D>
	{
	    typedef GSignalCodeLink<S, D> TThis;
			typedef R(D::*DM)(V);
			typedef V(S::*SM)();
      typedef typename GSignalCodeLink<S, D>::TD TD;
      typedef typename GSignalCodeLink<S, D>::TS TS;
			DM _dm;
			SM _sm;
		public:
			unnamed(TD d, DM dm, TS s, SM sm): TThis(s,d), _dm(dm), _sm(sm) {}
			void execute() {
				(this->dst->*_dm)((this->src->*_sm)());
			}
	};
	return sig.add(new unnamed(d, dm, s, sm));
}

// connect with value
/**
 * \\ingroup callback
 *
 * Add a callback that takes one parameter. The value for this parameter
 * is delivered from a variable.
 *
 * \\param sig Signal
 * \\param d     Destination's object of the call.
 * \\param dm    Destination's method of the call which takes one parameter.
 * \\param v     Pointer to the variable for the parameter.
 */
template <class D, class R, class V>
TSignalLink* connect_value_of(TSignal &sig, D *d, R(D::*dm)(V), V *v)
{
	struct unnamed:
		public TSignalLink
	{
			typedef R(D::*DM)(V);
			D *d;
			DM dm;
			V *v;
		public:
			unnamed(D *d, DM dm, V *v): d(d), dm(dm), v(v) {}
			void execute() {
				(d->*dm)(*v);
			}
	};
	return sig.add(new unnamed(d, dm, v));
}


/**
 * \\ingroup callback
 *
 * A simplified variant of 'connect_value' for the GNU C++ Compiler.
 *
 * Connect method with `Value()' method:
 * \\pre
 * CONNECT_VALUE(signal,
 *   destination object, destination method,
 *   source object)
 * \\endpre
 */
#define CONNECT_VALUE(SIG, D, DM, S)\\
{\\
	typedef typeof(*D) TD;\\
	connect_value(SIG, D, &TD::DM, S);\\
}

/**
 * \\ingroup callback
 *
 * Add a callback that takes one parameter. The value for this parameter
 * is delivered from another objects 'Value()' method.
 *
 * \\param sig Signal 
 * \\param d     Destination's object of the call.
 * \\param dm    Destination's method of the call which takes one parameter.
 * \\param s     Source's object whose 'Value()' method will be called to
 *              deliver the parameter.
 */
template <class D, class S, class R, class V>
TSignalLink* connect_value(TSignal &sig, D *d, R(D::*dm)(V), S *s)
{
	struct unnamed:
		public GSignalCodeLink<S, D>
	{
	    typedef GSignalCodeLink<S, D> TThis;
			typedef R(D::*DM)(V);
			typedef typename GSignalCodeLink<S, D>::TD TD;
			typedef typename GSignalCodeLink<S, D>::TS TS;
			DM _dm;
		public:
			unnamed(TD d, DM dm, TS s): TThis(s,d), _dm(dm) {}
			void execute() {
				(this->dst->*_dm)(this->src->getValue());
			}
	};
	return sig.add(new unnamed(d, dm, s));
}

struct TNone {
};

/**
 * \\ingroup callback
 *
 * This variable can be used as NULL variant as the 2nd and 3rd parameter
 * of the BGN_CONNECT_CODE macro.
 */
extern const TNone *NONE;

/**
 * \\ingroup callback
 *
 * Code Connection
 * \\li all arguments are executed once, which avoids unexpected
 *   side effects
 * \\li arguments
 *    1st: TSignal
 *    2nd: pointer to destination object or NONE
 *    3rd: pointer to source object or NONE
 *    4th: TSignalCode** or NULL
 */
#define BGN_CONNECT_CODE(SIG,D,S,NR) \\
	{\\
		typedef typeof(*S) TS;\\
		typedef typeof(*D) TD;\\
		TS *_s = S;\\
		TD *_d = D;\\
		TSignal *_sig = &SIG;\\
		TSignalLink **_nr = NR;\\
		class A:\\
			public GSignalCodeLink<TS,TD>\\
		{\\
		    typedef GSignalCodeLink<TS,TD> TThis;\\
			public:\\
				A(TS s, TD d): TThis(s,d) {}\\
				void execute() {

/**
 * \\ingroup callback
 */
#define END_CONNECT_CODE() \\
				}\\
		};\\
		TSignalLink *n = _sig->add(new A(_s,_d));\\
		if (_nr) *_nr = n;\\
	}


/*
 * various signal nodes & connect's
 */
EOT
  for($i=0; $i<=2; $i++) {
    fnode("GSignalLinkF$i", $i);
    onode("GSignalLinkO$i", $i);
  }
	print<<EOT;
	
} // namespace toad  connect.hh

#endif
EOT
}

main();

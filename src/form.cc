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

#include <toad/toadbase.hh>
#include <toad/pen.hh>
#include <toad/window.hh>
#include <toad/form.hh>
#include <toad/stacktrace.hh>

#include <algorithm>

using namespace toad;

#ifdef DEBUG
#undef DEBUG
#endif

// #define DEBUG

/**
 * \class toad::TForm
 * TOAD's layout manager.
 *
 * \todo
 *   \li 
 *     'attach' doesn't work when called from the constructor, it seems
 *     only to work from 'create()'
 *   \li
 *     make TFormBase a layout manager
 */

#define HAS_T 1
#define HAS_B 2
#define HAS_L 4
#define HAS_R 8
#define HAS_ALL 15
#define DTOP 0
#define DBOTTOM 1
#define DLEFT 2
#define DRIGHT 3

/*****************************************************************************
 *                                                                           *
 * TFormNode                                                                 *
 *                                                                           *
 *****************************************************************************/
class TFormBase::TFormNode
{
  public:
    TFormNode();
    TWindow *it;                // the window to be placed
    unsigned how[4];            // how to attach
    TWindow *which[4];          // where to attach
    int dist[4];                // minimal distance to neighbours
    int coord[4];               // the left,right,top & bottom during calculation
    byte done;                  // flags for attached sides
    byte nflag;                 // flags for sides with undefined attachment
    TFormNode *next, *prev;     // should remove `prev'
};

inline TFormBase::TFormNode::TFormNode()
{
  for(int i=0; i<4; i++)
  {
    how[i]=NONE;
    which[i]=NULL;
    dist[i]=0;
  }   
}

/*****************************************************************************
 *                                                                           *
 * TFormBase                                                                 *
 *                                                                           *
 *****************************************************************************/

/*---------------------------------------------------------------------------*
 | Constructor                                                               |
 *---------------------------------------------------------------------------*/
TFormBase::TFormBase()
{
  flist = lastadd = NULL;
  nBorderOverlap = 1;
  bKeepOwnBorder = false;
running = false;
}

TFormBase::~TFormBase()
{
#warning "memory leak"
#if 0
  while(flist) {
    TFormNode *on = flist;
    flist = flist->next;
    delete on;
  }
#endif
}

void
TFormBase::addForm(TWindow *child)
{
  if (flist) {
    lastadd = new TFormNode;
    lastadd->next = flist;
    lastadd->prev = flist->prev;
    flist->prev->next = lastadd;
    flist->prev = lastadd;
  } else {
    lastadd = flist = new TFormNode;
    flist->next = flist;
    flist->prev = flist;
  } 
  lastadd->it = child;
}

void
TFormBase::removeForm(TWindow *child)
{
  if (flist) {
    if (flist == flist->next) {
      delete flist;
      flist=NULL;
    } else {
      TFormNode *ptr = _find(child);
      if (!ptr)
        return;
      ptr->next->prev = ptr->prev;
      ptr->prev->next = ptr->next;
      if (flist==ptr)
        flist = ptr->next;
      delete ptr;
    }
  }
}

void
TFormBase::attachLast(unsigned where, unsigned how, TWindow *which)
{
  #ifdef SECURE
  if (!lastadd) {
    fprintf(stderr, "TOAD: error at TFormBase::AttachLast; no children\n");
    return;
  }
  #endif
  for(int i=0; i<4; i++) {
    if( (i==0 && where&TOP   ) ||
        (i==1 && where&BOTTOM) ||
        (i==2 && where&LEFT  ) ||
        (i==3 && where&RIGHT ) )
    {
      lastadd->how[i]=how;
      lastadd->which[i]=which;
    }
  }
}

void 
TFormBase::attach(TWindow *wnd, unsigned where, unsigned how, TWindow *which)
{
  #ifdef SECURE
  if (how==WINDOW && !which) {
    fprintf(stderr, "TFormBase.Attach: WINDOW without window specified => ignoring\n");
    printStackTrace();
    return;
  }
  #endif

  TFormNode *node = _find(wnd);
  if (!node)
    return;
  
  for(int i=0; i<4; i++) {
    if( (i==0 && where&TOP   ) ||
        (i==1 && where&BOTTOM) ||
        (i==2 && where&LEFT  ) ||
        (i==3 && where&RIGHT ) )
    {
      node->how[i]=how;
      node->which[i]=which;
    }
  }
}

void
TFormBase::distance(TWindow *wnd, int distance, unsigned where)
{
  TFormNode *node = _find(wnd);
  if (!node)
    return;
  
  for(int i=0; i<4; i++)
  {
    if( (i==0 && where&TOP   ) ||
        (i==1 && where&BOTTOM) ||
        (i==2 && where&LEFT  ) ||
        (i==3 && where&RIGHT ) )
    {
      node->dist[i]=distance;
    }
  }
}

/*---------------------------------------------------------------------------*
 | Arrange                                                                   |
 | arrange all children as described in the 'flist'                          |
 *---------------------------------------------------------------------------*/
void
TFormBase::arrange(int fx,int fy,int fw,int fh)
{
if (running) {
//  cout << "Rekursion" << endl;
  return;
}
running = true;

//cout << "TForm: arranging" << endl;
  #ifdef DEBUG
  printf("--> Arrange\n");
  #endif
  if (!flist) {
running = false;
    return;
  }

  // initialize data structures
  //----------------------------
  TRectangle shape;
  unsigned nChildren=0;
  bool bError=false;
  TFormNode *ptr = flist;
  do {
    #ifdef DEBUG
    printf("! \"%s\" at (%i,%i) with size (%i,%i)\n",
      ptr->it->Title().c_str(),
      ptr->it->XPos(), ptr->it->YPos(),
      ptr->it->Width(), ptr->it->Height());
    #endif
    nChildren++;
    ptr->done  = 0;
    ptr->nflag = 0;
    ptr->it->getShape(&shape);
    ptr->coord[DTOP]    = shape.y;
    ptr->coord[DBOTTOM] = shape.y+shape.h;
    ptr->coord[DLEFT]   = shape.x;
    ptr->coord[DRIGHT]  = shape.x+shape.w;
    for(int i=0; i<4; i++) {
      if (ptr->how[i] == NONE ) {
        ptr->nflag|=(1<<i);
      }
    }
    if ((ptr->nflag&3)==3 || (ptr->nflag&12)==12) {
      if(!ptr->it->bShell && !ptr->it->bPopup ) {
        fprintf(stderr, "toad: '%s' within TForm has undefined attachment\n",
                ptr->it->getTitle().c_str());
        bError = true;
      }
    }

    ptr = ptr->next;
  }while(ptr!=flist);
  
  if (bError) {
    fprintf(stderr, "toad: can't arrange children\n");
//    exit(1);
  }

  // arrange children
  //+-----------------
  int form[4];
  // form[0]=0; form[1]=Height(); form[2]=0; form[3]=Width();
  form[DTOP]=fy;
  form[DBOTTOM]=fy+fh;
  form[DLEFT]=fx;
  form[DRIGHT]=fx+fw;

  TFormNode *ptr2;

  unsigned count=0;
  unsigned done=0;      // we're done when `done' equals `nChildren'

  while(true) {
    count++;
    if (ptr->done != HAS_ALL) {
      // window has non attached sides
      //-------------------------------
      // 1st strategy:
      // attach all sides where the opposite side of another object is known
      //---------------------------------------------------------------------
      for(int i=0; i<4; i++) {
        if (!(ptr->done & (1<<i))) {
          switch(ptr->how[i])
          {
            case FORM:
              ptr->done |= (1<<i);
              ptr->coord[i] = form[i];
              if (!bKeepOwnBorder) {
                if (i&1)
                  ptr->coord[i] += nBorderOverlap;
                else
                  ptr->coord[i] -= nBorderOverlap;
              }
              if (i&1) {
                ptr->coord[i] -= ptr->dist[i];
              } else {
                ptr->coord[i] += ptr->dist[i];
              }
              count = 0;
              break;
            case WINDOW:
              ptr2=_find(ptr->which[i]);          // opposite window
              if ((ptr2->done) & (1<<(i^1))) {    // opposite side is set
                ptr->done |=(1<<i);
                ptr->coord[i] = ptr2->coord[i^1];
                if (i&1) { // bottom & right
                  ptr->coord[i] += nBorderOverlap;
                  ptr->coord[i] -= max(ptr->dist[i], ptr2->dist[i^1]);
                } else { // top & left
                  ptr->coord[i] -= nBorderOverlap;
                  ptr->coord[i] += max(ptr->dist[i], ptr2->dist[i^1]);
                }
                count = 0;
              }
              break;
            case OPPOSITE_WINDOW: // CODE IS MISSING FOR DISTANCE !!!
              ptr2=_find(ptr->which[i]);
              if ((ptr2->done) & (1<<(i))) {
                ptr->done |=(1<<i);
                ptr->coord[i] = ptr2->coord[i];
                count = 0;
              }
              break;
          }
        }
      } // end of the 1st strategy
      
      if ( (ptr->done|ptr->nflag) == HAS_ALL) {
      
        // 2nd strategy
        // we're almost done with the window, the missing coordinates
        // can be calculated from the objects size
        //------------------------------------------------------------
        ptr->it->getShape(&shape);
        #ifdef DEBUG
        printf("Placing %s now:\n",ptr->it->Title().c_str());
        #endif
        // no top and/or left attachment
        #ifdef DEBUG
        if (ptr->nflag & TOP) {
          ptr->coord[DTOP] = ptr->coord[DBOTTOM] - shape.h;
          printf("  has no top attachment, calculating it from bottom(%i) & height(%i)\n",
           ptr->coord[DBOTTOM],shape.h);
        }
        if (ptr->nflag & BOTTOM) {
          ptr->coord[DBOTTOM] = ptr->coord[DTOP] + shape.h;
          printf("  has no bottom attachment, calculating it from top(%i) & height(%i)\n",
           ptr->coord[DTOP],shape.h);
        }
        if (ptr->nflag & LEFT) {
          ptr->coord[DLEFT] = ptr->coord[DRIGHT] - shape.w;
          printf("  has no left attachment, calculating it from right(%i) & width(%i)\n",
          ptr->coord[DRIGHT], shape.w);
        }
        if (ptr->nflag & RIGHT) {
          ptr->coord[DRIGHT] = ptr->coord[DLEFT] + shape.w;
          printf("  has no right attachment, calculating it from right(%i) & width(%i)\n",
          ptr->coord[DLEFT], shape.w);
        }
        printf("  want to (%i,%i)-(%i,%i)\n",ptr->coord[DLEFT]
                                            ,ptr->coord[DTOP]
                                            ,ptr->coord[DRIGHT]
                                            ,ptr->coord[DBOTTOM] );
        #else
        if (ptr->nflag & TOP)
          ptr->coord[DTOP] = ptr->coord[DBOTTOM] - shape.h;
        if (ptr->nflag & BOTTOM)
          ptr->coord[DBOTTOM] = ptr->coord[DTOP] + shape.h;
        if (ptr->nflag & LEFT)
          ptr->coord[DLEFT] = ptr->coord[DRIGHT] - shape.w;
        if (ptr->nflag & RIGHT)
          ptr->coord[DRIGHT] = ptr->coord[DLEFT] + shape.w;
        #endif
        unsigned w,h;
        w = ptr->coord[DRIGHT] - ptr->coord[DLEFT];
        h = ptr->coord[DBOTTOM] - ptr->coord[DTOP];

        //ptr->it->SetSize(w,h);
        ptr->it->setShape(TPOS_PREVIOUS, TPOS_PREVIOUS, w,h);

        // adjust top and/or left after SetSize
        ptr->it->getShape(&shape);
        if (ptr->nflag & TOP)
          ptr->coord[DTOP] = ptr->coord[DBOTTOM] - shape.h;
        if (ptr->nflag & BOTTOM)
          ptr->coord[DBOTTOM] = ptr->coord[DTOP] + shape.h;
        if (ptr->nflag & LEFT)
          ptr->coord[DLEFT] = ptr->coord[DRIGHT] - shape.w;
        if (ptr->nflag & RIGHT)
          ptr->coord[DRIGHT] = ptr->coord[DLEFT] + shape.w;
        #ifdef DEBUG
        printf("  want to (%i,%i)-(%i,%i)\n",ptr->coord[DLEFT]
                                            ,ptr->coord[DTOP]
                                            ,ptr->coord[DRIGHT]
                                            ,ptr->coord[DBOTTOM] );
        #endif
        
        ptr->it->setPosition(ptr->coord[DLEFT],ptr->coord[DTOP]);
        ptr->it->getShape(&shape);

        ptr->coord[DTOP]   = shape.y;
        ptr->coord[DBOTTOM] = shape.y+shape.h;
        ptr->coord[DLEFT]    = shape.x;
        ptr->coord[DRIGHT]  = shape.x+shape.w;
        ptr->done = HAS_ALL;
        done++;
        #ifdef DEBUG
        printf("  set to (%i,%i)-(%i,%i)\n" ,ptr->coord[DLEFT]
                                            ,ptr->coord[DTOP]
                                            ,ptr->coord[DRIGHT]
                                            ,ptr->coord[DBOTTOM] );
        #endif
      }
    }

    if (done>=nChildren) {
//      cout << "TForm: >>>done<<<" << endl;
      running = false;
      return;
    }
    
    if (count>nChildren) {
      bool bNoGuess = true;
      count=0;
      while(count<nChildren) {
        if (ptr->done != HAS_ALL) {
          ptr->it->getShape(&shape);
          if ( (ptr->nflag&LEFT) && !(ptr->done&LEFT) && (ptr->done&RIGHT) ) {
            #ifdef DEBUG
            printf("guessing left side of %s\n",ptr->it->Title().c_str());
            #endif
            ptr->coord[DLEFT] = ptr->coord[DRIGHT] - shape.w;
            ptr->done|=HAS_L;
            bNoGuess = false;
          }
          if ( (ptr->nflag&RIGHT) && !(ptr->done&RIGHT) && (ptr->done&LEFT) ) {
            #ifdef DEBUG
            printf("guessing right side of %s\n",ptr->it->Title().c_str());
            #endif
            ptr->coord[DRIGHT] = ptr->coord[DLEFT] + shape.w;
            ptr->done|=HAS_R;
            bNoGuess = false;
          }
          if ( !(ptr->nflag&TOP) && !(ptr->done&TOP) && (ptr->done&BOTTOM) ) {
            #ifdef DEBUG
            printf("guessing top side of %s\n",ptr->it->Title().c_str());
            #endif
            ptr->coord[DTOP] = ptr->coord[DBOTTOM] - shape.h;
            ptr->done|=HAS_T;
            bNoGuess = false;
          }
          if ( (ptr->nflag&BOTTOM) && !(ptr->done&BOTTOM) && (ptr->done&TOP) ) {
            #ifdef DEBUG
            printf("guessing bottom side of %s\n",ptr->it->Title().c_str());
            #endif
            ptr->coord[DBOTTOM] = ptr->coord[DTOP] + shape.h;
            ptr->done|=HAS_B;
            bNoGuess = false;
          }
          if (ptr->done == HAS_ALL) {
            cout << "TForm: looks like recursive attachment" << endl;
          }
        }
        count++;
        ptr = ptr->next;
      }

      if(bNoGuess) {
        printf("*TForm: Can't handle recursive attachment. Stopped.\n");
        #ifdef DEBUG
        count=0;
        while(count<nChildren) {
          printf("%25s : ",ptr->it->Title().c_str());
          printf( ptr->done&HAS_T ? "t" : "-");
          printf( ptr->done&HAS_B ? "b" : "-");
          printf( ptr->done&HAS_L ? "l" : "-");
          printf( ptr->done&HAS_R ? "r" : "-");
          printf("\n");
          count++;
          ptr = ptr->next;
        } 
        #endif
        running = false;
        return;
      }
      #ifdef DEBUG
      printf("trying again\n");
      #endif
      count = 0;
    }
    ptr = ptr->next;
  }
running=false;
return;
}

TFormBase::TFormNode* 
TFormBase::_find(TWindow* which)
{
  TFormNode *ptr2=flist;
  while(ptr2->it != which) {
    ptr2 = ptr2->next;
    if (ptr2==flist) {
      fprintf(stderr, "toad: (TForm) '%s' is not a child of TForm.\n",
                      which->getTitle().c_str()/*, Title()*/);
      printStackTrace();
      return NULL;
    }
  }
  return ptr2;
}

void
TForm::childNotify(TWindow *c, EChildNotify t)
{
  switch(t){
    case TCHILD_ADD: 
      addForm(c);
      break;
    case TCHILD_REMOVE: 
      removeForm(c); 
      break;
    case TCHILD_RESIZE:
//cout << "TForm: child \"" << c->Title() << "\" has been resized" << endl;
      arrange(0,0,getWidth(),getHeight()); 
      break;
    default:;
  }
}

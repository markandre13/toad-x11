/****************************************************************************
 *                                                                          *
 * Boobaloo                                                                 *
 * Copyright (C) 1995-2003 by Mark-Andr√© Hopf <mhopf@mark13.org>            *
 *                                                                          *
 * This program is free software; you can redistribute it and/or modify     *
 * it under the terms of the GNU General Public License as published by     *
 * the Free Software Foundation; either version 2 of the License, or        *
 * (at your option) any later version.                                      *
 *                                                                          *
 * This program is distributed in the hope that it will be useful,          *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 * GNU General Public License for more details.                             *
 *                                                                          *
 * You should have received a copy of the GNU General Public License        *
 * along with this program; if not, write to the Free Software              *
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA  *
 *                                                                          *
 * 09.08.1995: started                                                      *
 * 11.08.1995: player sprite finished, walking on platforms                 *
 * 14.09.1995: invented a C++ class for the player to simplify programming  *
 *             and added comments                                           *
 * 28.09.1995: done a little bit in the evening                             *
 * 31.10.1995: done a little bit in the morning                             *
 * 07.01.1996: checking the connection to the X server on my own            *
 * 03.06.1996: added JUMP_UP                                                *
 * 28.10.1998: started using the TOAD toolkit and got the keyboard stuff    *
 *             right for the first time                                     *
 * 03.10.1999: fixed a screen update bug                                    *
 *                                                                          *
 ****************************************************************************/

#include <toad/toad.hh>
#include <toad/region.hh>
#include <toad/simpletimer.hh>

using namespace toad;

TBitmap *bitmap;
TBitmap *brick;

int nZoom = 2;
int lives = 3;

void create_sprites();
void create_bricks();

TWindow *game_window;

class TSprite
{
  public:
    TSprite() {
      x = y = 0;
    }
    virtual void step(TWindow&) = 0;
    virtual void paint(TPen&) = 0;
    int x,y;
};

class TPlayer:
  public TSprite
{
    enum EState { STANDING, JUMP_LEFT, JUMP_RIGHT, JUMP_UP };
    EState state;
    unsigned jumpstep;
    int ghost_y;
  public:
    TPlayer();
  
    bool move_left:1;
    bool move_right:1;
    bool move_up:1;
    bool move_down:1;
    bool dead:1;
    int sprite;

    void MoveLeft();
    void MoveRight();
    bool OnGround();
    bool OnLeftBelt();
    bool Collision();
    void CollectGold();
    void CheckDeath();
    void Kill();
    
    void step(TWindow&);
    void paint(TPen&);
};

enum {
 ROOM_EMPTY, ROOM_EARTH, ROOM_STEEL, 
 ROOM_MOVE8, ROOM_MOVE7, ROOM_MOVE6, ROOM_MOVE5, ROOM_MOVE4, ROOM_MOVE3, 
 ROOM_MOVE2, ROOM_MOVE1, ROOM_PLANT, 
 ROOM_ROCK, 
 WATER, WATER1, WATER2, WATER3, WATER4, WATER5,
 GOLD1, GOLD2, GOLD3, GOLD4, GOLD5, GOLD6, GOLD7,
 LBELT1, LBELT2, LBELT3, LBELT4,
 LLBELT1, LLBELT2, LLBELT3, LLBELT4,
 RLBELT1, RLBELT2, RLBELT3, RLBELT4,
 NICONS
};

const unsigned uRoomCount = 1;

class Room
{
  private:
    char cRoom[80][25];
    
    unsigned gold_total;
    unsigned gold_remaining;
    int gx,gy;                  // position of current gold animation
    
  public:
    bool Load(unsigned nr);     // load room description
    void Paint(TPen&);          // display whole current room
    char Peek(int x,int y);     // peek into the room at position (x,y)
    void Poke(int x,int y, char item);
    void StandOn(int x,int y, TWindow&);
    
    void step(TWindow&);
};

Room room;

TPlayer::TPlayer()
{
  dead = false;
  move_left = move_right = move_up = move_down = false;
  sprite=0;
  state = STANDING;
}

void TPlayer::step(TWindow &wnd)
{
  if (!dead) {
    struct jp {int x,y;};
    static struct jp way[12] = {
      { 1,-1},{ 1,-1},{ 1, 0},{ 1,-1},{ 1, 0},{ 1, 0},
      { 1, 0},{ 1, 0},{ 1, 1},{ 1, 0},{ 1, 1},{ 1, 1}
    };
  
    if (state==STANDING && move_up && OnGround()) {
      jumpstep=0;
      if (move_right==move_left) {
        state = JUMP_UP;
      } else if (move_right) {
        state = JUMP_RIGHT;
      } else if (move_left) {
        state = JUMP_LEFT;
      }
    }
  
    int ox=x, oy=y, os=sprite;
    if (state!=STANDING) {
      if (way[jumpstep].x) {
        switch(state) {
          case JUMP_LEFT:
            MoveLeft();
            break;
          case JUMP_RIGHT:
            MoveRight();
            break;
          default:
            break;
        }
      }
    
      y+=way[jumpstep].y;
      jumpstep++;
      if (jumpstep>6) {
        if (jumpstep<12) {
          if (OnGround()) {
            state = STANDING;
          }
        } else {
          state = STANDING;
        }
      }
#ifdef SOUND
      if (way[jumpstep-1].y>0)
        TOADBase::Bell(0, (25-y)*10);
#endif
    } else if (!OnGround()) {
#ifdef SOUND
      TOADBase::Bell(0, (25-y)*10);
#endif
      y++;
    } else if (move_left && !move_right) {
      MoveLeft();
    } else if (move_right && !move_left) {
      MoveRight();
    }
    
    if (OnLeftBelt()) {
      MoveLeft();
    }
  
    if (Collision()) {
      x = ox; y = oy; sprite=os; state = STANDING;
    } else if (x<0 || x>=40) {
      x=ox; sprite=os;
    }

    if (ox!=x || oy!=y || os!=sprite) {
      CollectGold();
      wnd.invalidateWindow(ox<<4,oy<<4,nZoom<<4,nZoom<<4);
      wnd.invalidateWindow(x<<4,y<<4,nZoom<<4,nZoom<<4);
    }
    
    room.StandOn(x,y+2, wnd);
    room.StandOn(x+1,y+2, wnd);
    
    CheckDeath();
    
  } else {
    // dead
    ghost_y-=4;
    wnd.invalidateWindow(x<<4,ghost_y,nZoom<<4,nZoom<<4+1);
  }
}

void TPlayer::CheckDeath()
{
  return;
  // the deadly plant is missing...
  if ( (room.Peek(x,y+2)>=WATER1 && room.Peek(x,y+2)<=WATER5)
     &&(room.Peek(x+1,y+2)>=WATER1 && room.Peek(x+1,y+2)<=WATER5) )
  {
    Kill();
  }
}

bool TPlayer::OnGround()
{
  return !(   (room.Peek(x  ,y+2)==ROOM_EMPTY 
           || (room.Peek(x  ,y+2)>=GOLD1 && room.Peek(x  ,y+2)<=GOLD7)
           || (room.Peek(x  ,y+2)>=WATER && room.Peek(x  ,y+2)<=WATER5) )
           && (room.Peek(x+1,y+2)==ROOM_EMPTY
           || (room.Peek(x+1,y+2)>=GOLD1 && room.Peek(x+1,y+2)<=GOLD7)
           || (room.Peek(x+1,y+2)>=WATER && room.Peek(x+1,y+2)<=WATER5) )
           );
}

bool TPlayer::OnLeftBelt()
{
  return (room.Peek(x,y+2)>=LBELT1 && room.Peek(x,y+2)<=RLBELT4) ||
         (room.Peek(x+1,y+2)>=LBELT1 && room.Peek(x+1,y+2)<=RLBELT4);
}

bool TPlayer::Collision()
{
  return (room.Peek(x,y)==ROOM_ROCK ||
          room.Peek(x+1,y)==ROOM_ROCK ||
          room.Peek(x,y+1)==ROOM_ROCK ||
          room.Peek(x+1,y+1)==ROOM_ROCK);
}

void TPlayer::CollectGold()
{
  for(int lx=0; lx<2; lx++) {
    for(int ly=0; ly<2; ly++) {
      int item = room.Peek(lx+x,ly+y);
      if (item>=GOLD1 && item<=GOLD7) {
        room.Poke(lx+x,ly+y, ROOM_EMPTY);
#ifdef SOUND
        TOADBase::Bell(0, 500);
#endif
      }
    }
  }
}

void TPlayer::MoveLeft()
{
  if (sprite<4) {
    sprite=7-sprite;
  } else {
    sprite++;
    if (sprite>7) {
      sprite=4;
      x--;
    }
  }
}

void TPlayer::MoveRight()
{
  if (sprite>=4) {
    sprite-=4;
    sprite=3-sprite;
  } else {
    sprite++;
    if (sprite>3) {
      sprite=0;
      x++;
    }
  }
}

void TPlayer::Kill()
{
  if (!dead) {
    ghost_y = y<<4;
    dead = true;
  }
}

void TPlayer::paint(TPen &pen)
{
  if (!dead) {
    int width=nZoom<<4;
    pen.drawBitmap(x<<4,y<<4, bitmap, width*sprite, 0,width,width);
  } else {
    int width=nZoom<<4;
    pen.drawBitmap(x<<4,y<<4, bitmap, width*9, 0,width,width);
    pen.drawBitmap(x<<4,ghost_y, bitmap, width*8, 0,width,width);
  }
}

class TMainWindow: 
  public TWindow, public TSimpleTimer
{
  public:
    TMainWindow();
  protected:
    void keyDown(TKey key, char *string, unsigned state);
    void keyUp(TKey key, char *string, unsigned state);
    void tick();
    void paint();

    TPlayer *player;    
};

int main(int argc, char** argv, char** envv)
{
  toad::initialize(argc, argv, envv);

  create_sprites();
  create_bricks();
  room.Load(0);

  TMainWindow *main_window = new TMainWindow();

  toad::mainLoop();

        delete main_window;
        toad::terminate();

  return 0;
}

TMainWindow::TMainWindow()
  :TWindow(NULL, "Jump'n Run")
{
  bDoubleBuffer = true;
  ::game_window = this;
  startTimer(0, 1000000/12); // 12 frames per second
  setBackground(0,0,0);
  setSize(40<<(nZoom+2), 25<<(nZoom+2));
  player = new TPlayer();
  player->x = 2;
  player->y = 17;
}

int lsc=0;
int livesprite = 0;

void TMainWindow::tick()
{
  player->step(*this);
  room.step(*this);

  ++lsc;
  if (lsc>=3) {
    ++livesprite;
      if (livesprite>=8)
        livesprite=0;
    lsc=0;
    int width=nZoom<<4;
    invalidateWindow(0,23<<4, width*3, width);
  }
}

void TMainWindow::paint()
{
  TPen pen(this);
  room.Paint(pen);
  player->paint(pen);
  
  for(int i=0; i<lives; ++i) {
    int x = i * 2;
    int y = 23;
    int width = 32;
    pen.drawBitmap(x<<4,y<<4, bitmap, width*livesprite, 0,width,width);
  }
}

void TMainWindow::keyDown(TKey key, char *string, unsigned state)
{
  switch(key) {
    case TK_LEFT:
      player->move_left=true;
      break;
    case TK_RIGHT:
      player->move_right=true;
      break;
    case TK_UP:
      player->move_up=true;
      break;
    case TK_DOWN:
      player->move_down=true;
      break;
  }
}

void TMainWindow::keyUp(TKey key, char *string, unsigned state)
{
  switch(key) {
    case TK_LEFT:
      player->move_left=false;
      break;
    case TK_RIGHT:
      player->move_right=false;
      break;
    case TK_UP:
      player->move_up=false;
      break;
    case TK_DOWN:
      player->move_down=false;
      break;
  }
}

int nzoom = 2;

const TRGB& decode_color(char a)
{
  static TRGB cmap[16] =
  {
    TRGB(  0,  0,  0),  // schwarz    0 
    TRGB(  0,  0,128),  // dunkelblau 1
    TRGB(  0,  0,255),  // blau       2
    TRGB(  0,128,255),  // hellblau   3
    TRGB(  0,255,255),  // hellcyan   4
    TRGB(128,  0,  0),  // dunkelrot  5
    TRGB(255,  0,  0),  // rot        6
    TRGB(255,128,  0),  // orange     7
    TRGB(255,255,  0),  // gelb       8
    TRGB(  0,128,  0),  // dunkelgr¸n 9
    TRGB(  0,255,  0),  // hellgr¸n   a
    TRGB(128,  0,128),  // lila       b
    TRGB(255,  0,255),  // pink       c
    TRGB(255,128,128),  // hautfarben d
    TRGB(128,128,128),  // grau       e
    TRGB(255,255,255)   // weiﬂ       f
  };

  if (a==' ') return cmap[0];
  if (a>='0' && a<='9') return cmap[a-'0'];
  if (a>='a' && a<='f') return cmap[a-'a'+10];
  if (a>='A' && a<='F') return cmap[a-'F'+10];
  fprintf(stderr,"Wrong entry inside icon or sprite definition: `%c'.\n",(char)a);
  return cmap[15];
}

void create_sprites()
{
  cout << "creating sprites" << endl;

  static char spr16x16[6][16][17] =
  {
    {
      " 08ff880        ",
      "0888880         ",
      "0885ddd0        ",
      " 055dd0         ",
      "  0dd0          ",
      "  09a0          ",
      " 0eeff0         ",
      " 09aaa0         ",
      " 0efff0         ",
      " 09aaa0         ",
      " 01dd20         ",
      " 01dd210        ",
      "05112210        ",
      " 012220         ",
      " 012220         ",
      " 0555550        "
    },{
      "    00000       ",
      "   08ff880      ",
      "  0888880       ",
      "  08885dd0      ",
      "   0555d0       ",
      "    0dd0        ",
      "    09a0        ",
      "   0eeff0       ",
      "   09aaa0       ",
      "  0eefff0       ",
      "  0ddaaad0      ",
      "  0dd222d0      ",
      "   0122220 0    ",
      "  01122122050   ",
      " 05222011150    ",
      "  055550550     "
    },{
      "     08ff880    ",
      "    0888880     ",
      "    0885ddd0    ",
      "     055dd0     ",
      "      0dd0      ",
      "      09a0      ",
      "     0eeff0     ",
      "     09aaa0     ",
      "     0efff0     ",
      "     09aaa0     ",
      "     01dd20     ",
      "     01dd220    ",
      "    05112220    ",
      "     051210     ",
      "     015520     ",
      "     0555550    "
    },{
      "        00000   ",
      "       08ff880  ",
      "      08888d0   ",
      "      085dddd0  ",
      "       05ddd0   ",
      "        0dd0    ",
      "        09a0    ",
      "       0eeff0   ",
      "       09aaa0   ",
      "      0deefdd0  ",
      "      0d999dd0  ",
      "       011110   ",
      "        011220 0",
      "      0110112205",
      "     05222011150",
      "      055550550 "
    },{
      "     fffff      ",
      "    f  f  f     ",
      "    f  f  f     ",
      "    fffeffe     ",
      "     fe fe      ",
      "      f e       ",
      "       f        ",
      "      ffe       ",
      "     fffffe     ",
      "    fefffefe    ",
      "   feffffe fe   ",
      "  fe ffeee ee   ",
      "     fe fe      ",
      "     fe fe      ",
      "     fe fe      ",
      "     fe fe      "
    },{
      "                ",
      "                ",
      "                ",
      "                ",
      "                ",
      "                ",
      "                ",
      "        0000    ",
      "       0feff0   ",
      "   0  0fe0fef0  ",
      "  0e00fe0fe0ff0 ",
      " 0fe00fffe0ffe0 ",
      "000e00ff0fffe0f0",
      "ff00e0ffffee0fff",
      "0eefeeffee00fe0 ",
      "fffeffeeeeeee0e0"
    }
  };
  
  int width=nZoom<<4;

#define NSPRITES 10

  bitmap = new TBitmap(width*10,width, TBITMAP_SERVER);
  TBitmapMask mask(width*10, width);

  TPen pen(bitmap);
  for(int i=0; i<4; i++) {
    for(int y=0; y<16; y++) {
      for(int x=0; x<16; x++) {
        if (spr16x16[i][y][x]==' ') {
          for(int y2=0; y2<nZoom; y2++) {
            for(int x2=0; x2<nZoom; x2++) {
              mask.setPixel(x*nZoom+i*width+x2,          y*nZoom+y2, false);
              mask.setPixel((15-x)*nZoom+(i+4)*width+x2, y*nZoom+y2, false);
            }
          }
        } else {
          pen.setColor(decode_color(spr16x16[i][y][x]));
          pen.fillRectangle(
            x*nZoom+i*width, y*nZoom,
            nZoom, nZoom
          );
          pen.fillRectangle(
            (15-x)*nZoom+(i+4)*width, y*nZoom,
            nZoom, nZoom
          );
        }
      }
    }
  }

  for(int i=4; i<6; i++) {
    for(int y=0; y<16; y++) {
      for(int x=0; x<16; x++) {
        if (spr16x16[i][y][x]==' ') {
          for(int y2=0; y2<nZoom; y2++) {
            for(int x2=0; x2<nZoom; x2++) {
              mask.setPixel(x*nZoom+(i+4)*width+x2, y*nZoom+y2, false);
            }
          }
        } else {
          pen.setColor(decode_color(spr16x16[i][y][x]));
          pen.fillRectangle(
            x*nZoom+(i+4)*width, y*nZoom,
            nZoom, nZoom
          );
        }
      }
    }
  }

  bitmap->setMask(mask);
}

void create_bricks()
{
  static char spr8x8[NICONS][8][9] =
  {
    {
      "        ", // 0: EMPTY
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
    },{
      " 557775 ", // 1: EARTH 
      "55775577",
      "77557577",
      "75577757",
      "77775577",
      "77775575",
      "57757777",
      "55577755",
    },{
      "44444444", // 2: STEEL
      "11111111",
      "22211222",
      "224  122",
      "22f  122",
      "222f4222",
      "33333333",
      "11111111",
    },{
      "66666666", // 3: MOVE8
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ",
      "66666666", // 4: MOVE7
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ", // 5: MOVE6
      "        ",
      "66666666",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ", // 6: MOVE5
      "        ",
      "        ",
      "66666666",
      "6 6 6 6 ",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ", // 7: MOVE4
      "        ",
      "        ",
      "        ",
      "66666666",
      " 6 6 6 6",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ", // 8: MOVE3
      "        ",
      "        ",
      "        ",
      "        ",
      "66666666",
      "6 6 6 6 ",
      " 6 6 6 6",
    },{
      "        ", // 9: MOVE2
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
      "66666666",
      " 6 6 6 6",
    },{
      "        ", // 10:MOVE1
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
      "        ",
      "66666666",
    },{
      "    8a  ", // 11: PLANT
      "   899a9",
      "  8a a9 ",
      "   9a9  ",
      " 8a a9 a",
      "89a9a9a ",
      "  aaa9  ",
      "  aa999 ",
    },{
      " fee fee", // 12: ROCK
      "eee feee",
      "efe   ee",
      "ee eee f",
      "eee  ee ",
      " feee ee",
      "feeee fe",
      "eeee fee",
    },{
      "11111111", // 13: WATER
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "3       ", // 13: WATER1
      "111   23",
      "11112111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "        ", // 14: WATER2
      "21     2",
      "11112121",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "        ", // 15: WATER3
      "        ",
      "21112111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "        ", // 16: WATER4
      "   221  ",
      "12111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "    3   ", // 17: WATER5
      "  23111 ",
      "21111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
      "11111111",
    },{
      "        ", // 18: GOLD1
      "        ",
      "        ",
      "   8877 ",
      "  877755",
      "  877655",
      "  776555",
      "   5555 ",
    },{
      "        ", // 19: GOLD2
      "        ",
      "   f    ",
      "  fef77 ",
      "  8f7755",
      "  877655",
      "  776555",
      "   5555 ",
    },{
      "        ", // 20: GOLD3
      "   f    ",
      "   f    ",
      " ffeff7 ",
      "  8f7755",
      "  8f7655",
      "  776555",
      "   5555 ",
    },{
      "   f    ", // 21: GOLD4
      "   f    ",
      "   f    ",
      "fffffff ",
      "  8f7755",
      "  8f7655",
      "  7f6555",
      "   5555 ",
    },{
      "  f     ", // 22: GOLD5
      "  f     ",
      "   f ff ",
      "  fef   ",
      "ff8f7755",
      "  87f655",
      "  77f555",
      "   5555 ",
    },{
      "        ", // 23: GOLD6
      " f   f  ",
      "  f f   ",
      "   e888 ",
      "  f7f755",
      " f876f55",
      "  775555",
      "   5555 ",
    },{
      "        ", // 24: GOLD7
      "        ",
      "   f    ",
      "  fef88 ",
      "  8f7755",
      "  876f55",
      "  775555",
      "   5555 ",
    },{
      "  eeeeee", // 25:  LBELT1 (conveyor-belt left)
      "        ",
      "55665555",
      "66666666",
      "66666666",
      "55555555",
      "        ",
      "  eeeeee",
    },{
      "eeeeee  ", // 26:  LBELT2 (conveyor-belt)
      "        ",
      "66555555",
      "66666666",
      "66666666",
      "55555555",
      "        ",
      "ee  eeee",
    },{
      "eeee  ee", // 27:  LBELT3 (conveyor-belt)
      "        ",
      "55555566",
      "66666666",
      "66666666",
      "55555555",
      "        ",
      "eeee  ee",
    },{
      "ee  eeee", // 28:  LBELT4 (conveyor-belt)
      "        ",
      "55556655",
      "66666666",
      "66666666",
      "55555555",
      "        ",
      "eeeeee  ",
    },{
      "     eee", // 25:  LLBELT1 (conveyor-belt left)
      "  eee   ",
      "    5555",
      "   66666",
      "   66666",
      "    5555",
      "  eee   ",
      "     eee",
    },{
      "    e   ", // 25:  LLBELT2 (conveyor-belt left)
      "   e    ",
      "  e 5556",
      " e 56666",
      "e  56666",
      "    5555",
      "    eee ",
      "       e",
    },{
      "      ee", // 25:  LLBELT3 (conveyor-belt left)
      "  e     ",
      "  e 6666",
      "  e56666",
      "  e56666",
      "  e 5555",
      "  e     ",
      "      ee",
    },{
      "       e", // 25:  LLBELT4 (conveyor-belt left)
      "    eee ",
      "e   6555",
      " e 66666",
      "  e56666",
      "   e5555",
      "    e   ",
      "     e  ",
    },{
    
    
      "  e     ", // 25:  RLBELT1 (conveyor-belt left)
      "   e    ",
      "5566e   ",
      "66665e  ",
      "66665 e ",
      "5555   e",
      " eee    ",
      "e       ",
    },{
      "eee     ", // 25:  RLBELT2 (conveyor-belt left)
      "   eee  ",
      "5555    ",
      "66665   ",
      "66665   ",
      "5555    ",
      "   eee  ",
      "eee     ",
    },{
      "e       ", // 25:  RLBELT3 (conveyor-belt left)
      " eee    ",
      "5555   e",
      "66665 e ",
      "66665e  ",
      "5555e   ",
      "   e    ",
      "  e     ",
    },{
      "ee      ", // 25:  RLBELT4 (conveyor-belt left)
      "     e  ",
      "5556 e  ",
      "66665e  ",
      "66665e  ",
      "5555 e  ",
      "     e  ",
      "ee      ",
    }
  };

  int width=nZoom<<3;

  brick = new TBitmap(width * NICONS, width, TBITMAP_SERVER);
  TPen pen(brick);

  for(int i=0; i<NICONS; i++) {
    for(int y=0; y<8; y++) {
      for(int x=0; x<8; x++) {
        pen.setColor(decode_color(spr8x8[i][y][x]));
        pen.fillRectangle(
          x*nZoom+i*width,y*nZoom,
          nZoom, nZoom
        );
      }
    }
  }
}

char Room::Peek(int x,int y)
{
  if (x<0 || x>79 || y<0 || y>24)
    return ROOM_EMPTY;
  return cRoom[x][y];
};

void Room::Poke(int x,int y, char item)
{
  if (x<0 || x>79 || y<0 || y>24)
    return;
  if (x==gx && y==gy) {
    gx=gy=-1;
  }
  cRoom[x][y]=item;
  
  int s=nZoom+2;
  int width=nZoom<<3;
  game_window->invalidateWindow(x<<s, y<<s, width,width);
};

void Room::StandOn(int x,int y, TWindow &wnd)
{
  if (x<0 || x>79 || y<0 || y>24)
    return;
  
  int s=nZoom+2;

  if (cRoom[x][y]>=ROOM_MOVE8 && cRoom[x][y]<=ROOM_MOVE1)
  {
    cRoom[x][y]++;
    if (cRoom[x][y]>ROOM_MOVE1)
      cRoom[x][y] = ROOM_EMPTY;
    wnd.invalidateWindow(x<<s, y<<s, nZoom<<3,nZoom<<3);
  }
};

bool Room::Load(unsigned nr)
{
  char room[uRoomCount][23][41] =
  {
    {
      "rr                                   rrr",
      "                                       r",
      "e                                       ",
      "eg                         grrrrrr      ",
      "eessssssss   ssssss       rrrr        rr",
      "eeeeee           eeee      rrr       rrr",
      "eeee                       rrr     rrrrr",
      "ee                      g    rr       rr",
      "                   rrrrrrr   rr        r",
      "    66366366366366           r  g      r",
      "                             rrrrr     r",
      "r                            rrr        ",
      "rrr         p         g   124rr         ",
      "r    rrr[<<<<<<<<]rrrrrrrrr88r          ",
      "r      rrrrrrrrrrrr rrrrrrr88rr         ",
      "p       g    rrrr    grrrrr88rr         ",
      "e             rr       rrrr88rrr        ",
      "ee            rr          588rr         ",
      "ee            rrr       25888rrr       r",
      "eeeeWWWWWWWWWWrrrrrrWWWrrrrrrrrrrWWWWWrr",
      "eeeeeewwwwwwwwwwwwwwwwrrrrrrrrrrrrwwwrrr",
      "eeeeeeeewwwwwwwwwwwwwwrrrrrrrrrrrrwwwrrr",
      "eeeeeeeeeeeeerrrrrrrrrrrrrrrrrrrrrwwwrrr",
    }
  };
  
  bool bRoomValid=nr<uRoomCount;
  
  gold_total = 0;

  for(int y=0; y<23; y++) {
    for(int x=0; x<40; x++) {
      int c;
      if (bRoomValid) {
        switch(room[nr][y][x])
        {
          case ' ':
            c=ROOM_EMPTY;
            break;
          case 'e':
            c=ROOM_EARTH;
            break;
          case 's':
            c=ROOM_STEEL;
            break;
          case '8':
            c=ROOM_MOVE8;
            break;
          case '7':
            c=ROOM_MOVE7;
            break;
          case '6':
            c=ROOM_MOVE6;
            break;
          case '5':
            c=ROOM_MOVE5;
            break;
          case '4':
            c=ROOM_MOVE4;
            break;
          case '3':
            c=ROOM_MOVE3;
            break;
          case '2':
            c=ROOM_MOVE2;
            break;
          case '1':
            c=ROOM_MOVE1;
            break;
          case 'p':
            c=ROOM_PLANT;
            break;
          case 'r':
            c=ROOM_ROCK;
            break;
          case 'W':
            c=WATER1;
            break;
          case 'w':
            c=WATER;
            break;
          case 'g':
            c=GOLD1;
            gold_total++;
            break;
          case '<':
            c=LBELT1;
            break;
          case '[':
            c=LLBELT1;
            break;
          case ']':
            c=RLBELT1;
            break;
        }
      } else {
        c=ROOM_EMPTY;
      }
      cRoom[x][y] = c;
    } 
  }
  gold_remaining = gold_total;
  gx=gy=-1;
  return bRoomValid;
}

void Room::Paint(TPen &pen)
{
  int s=nZoom+2;
  long n = pen.region->getNumRects();
  for(long i=0; i<n; i++) {
    TRectangle rect;
    pen.region->getRect(i,&rect);
    rect.w+=rect.x+(1<<s)-1;
    rect.h+=rect.y+(1<<s)-1;
    rect.x>>=s;
    rect.y>>=s;
    rect.w>>=s;
    rect.h>>=s;
    if (rect.w>40)
      rect.w = 40;
    if (rect.h>23)
      rect.h = 23;

    int width=nZoom<<3;
    for(int y=rect.y; y<rect.h; y++)  {
      for(int x=rect.x; x<rect.w; x++)  {
        pen.drawBitmap(x<<s, y<<s, brick, cRoom[x][y]*width,0, width,width);
      }
    }
  }
}

static int water_state = 0;
static int timer1 = 0;

void Room::step(TWindow &wnd)
{
  int s=nZoom+2;
  // int width=nZoom<<3;

  // animate gold nuggets
  //--------------------------------------------------------------
  int item, oitem;
  if (gx<0 || gy<0) {
    gx=gy=-1;
    // find a new gold nugget for animation
    int n = (int) ( ((double)gold_total)*rand()/(RAND_MAX+1.0) );
    for(int y=0; y<23; y++) {
      for(int x=0; x<40; x++) {
        if (cRoom[x][y]==GOLD1) {
          n--;
          if (n<0) {
            gx=x; gy=y;
            goto stop;
          }
        }
      }
    }
    stop:;
  } else {
    item = cRoom[gx][gy];
    item++;
    if (item>GOLD7)
      item=GOLD1;
    cRoom[gx][gy] = item;
    wnd.invalidateWindow(gx<<s, gy<<s, nZoom<<4,nZoom<<4);
    if (item==GOLD1) {
      gx=gy=-1;
    }
  }
  
  // animate water
  //----------------------------------------------------------------- 
  timer1++;
  if (timer1&1) {
  water_state++;
    if (water_state>=10)
      water_state=0;
  }
  
  for(int y=0; y<23; y++) {
    for(int x=0; x<40; x++) {
      item = oitem = cRoom[x][y];
      if (timer1&1 && item>=WATER1 && item<=WATER5) {
        if (water_state<5) {
          item=water_state+WATER1;
        } else {
          item=9-water_state+WATER1;
        }
      }
      if (item>=LBELT1 && item<=LBELT4) {
        item++;
        if (item>LBELT4)
          item=LBELT1;
      } else if (item>=LLBELT1 && item<=LLBELT4) {
        item++;
        if (item>LLBELT4)
          item=LLBELT1;
      } else if (item>=RLBELT1 && item<=RLBELT4) {
        item++;
        if (item>RLBELT4)
          item=RLBELT1;
      } 
      if (item!=oitem) {
        cRoom[x][y] = item;
        wnd.invalidateWindow(x<<s, y<<s, nZoom<<4,nZoom<<4);
      }
    }
  }
}

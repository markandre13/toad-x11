/*
 * TCDP - An Audio CD Player for Linux 2.2 and the TOAD GUI Toolkit
 *
 * Copyright (C) 1998,99 Mark-André Hopf <mhopf@mark13.de>
 *
 * Changes:
 *   Tue, 17 Sep 2002 Amine Chadly
 *     adjustments to new TOAD API
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/*
    version 1.0 [Tue Nov 10 1998]
    version 1.1 [Wed Jan 20 1999]
      changes for `TSimpleTimer' in toad-0.42.11
    version 1.2 [Fri May 28 1999]
      moved the images inside the executable
    version 1.3pre1 [Wed Jun 23 1999]
      started adding cdindex support
      new background image
    version 1.3pre [
*/


#include <toad/toad.hh>
#include <toad/pushbutton.hh>
#include <toad/simpletimer.hh>


#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <vector>
#include <fstream>

// Thanx to Erik Andersen for the generic Linux CD-ROM interface:
#include <linux/cdrom.h>

#include "base64.h"
#include "sha.h"

using namespace toad;

struct TTrackInfo:
  public TSmartObject
{
  TTrackInfo(const string &t):title(t){};
  string title;
};

typedef GSmartPointer<TTrackInfo> PTrackInfo;

struct TCDInfo
{
  string artist;
  string title;
  string id;                      // ID for the current CD as in cdindex-1.1.0
  vector<PTrackInfo> tracks;
};

class TCDPlayer
{
    static const ulong CMD_PLAY_AUDIO_TRACK_INDEX=0x48;
    string _device;
    static const string unknown;
    int _fd;
    
    int first_track, last_track;  // first & last track of current CD
    int current_track;            // playing track or 0
    int current_index;            // playing index when track!=0
    TCDInfo *info;

    enum EState {
      STATE_NO_DISC,
      STATE_STOPED,   // disc
      STATE_PLAYING,  // disc & playing
      STATE_PAUSE     // disc & playing & paused
    };
    
    EState _state;

  public:
    const string& Artist() const { return _state==STATE_NO_DISC ? unknown : info->artist; }
    const string& Title() const { return _state==STATE_NO_DISC ? unknown : info->title; }
    const string& TrackTitle() const {
      if (_state!=STATE_NO_DISC &&
          _state!=STATE_STOPED && 
          current_track>0 && 
          current_track<=info->tracks.size())
        return info->tracks[current_track-1]->title;
      return unknown;
    }
  
    TCDPlayer(const string&);
    void StartMotor();
    void StopMotor();
    
    void Play() { Play(first_track, last_track); }
    void Play(int start_track) { Play(start_track, last_track); }
    void Play(int start_track, int end_track);
    void CheckDisc();
    
    void NextTrack();
    void PreviousTrack();
    
    void Pause();
    void Resume();
    void Eject();
    
    struct cdrom_subchnl _position;
    void FetchPosition();
    string PositionString();
};

const string TCDPlayer::unknown("");

class TMainWindow:
  public TWindow,
  public TSimpleTimer
{
    typedef TWindow super;
  public:
    TMainWindow(TWindow*,const string&);

    void cmdEject();
    void cmdPreviousTrack();
    void cmdNextTrack();
    void cmdStop();
    void cmdPause();
    void cmdPlay();

    void paint();
    void tick();
    int _ox;      // x-position for text output
    TBitmap *bmpBackground;
    TFont *_font;
    TCDPlayer *cdplayer;
};

void createMemoryFiles();

int main(int argc, char** argv, char** envv)
{
  // initialize toad
  toad::initialize(argc, argv, envv);
  
  createMemoryFiles();
  
  // create tcdp window without parent window
  TMainWindow *tcdp = new TMainWindow(NULL, "TOAD Compact Disc Player");
  
  // toad main loop, it will display every created element
  toad::mainLoop();
  
  // the window has been close so we dispose of the object.
  delete tcdp;
  
  // final toad clean up
  toad::terminate();
  
  // everything went just fine.
  return 0;
}

TMainWindow::TMainWindow(TWindow *p,const string &t)
  :super(p,t)
{
  cdplayer = new TCDPlayer("/dev/cdrom");
  TPushButton *pb;
  bStaticFrame = true;
  
  int h = 22+6, w;
  int y, x = y = 5;

  y+=60;
  
  w = 42+6;
  pb = new TPushButton(this, "^");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_open_close.png");
  pb->setToolTip("open/close tray");
  CONNECT(pb->sigClicked, this, cmdEject);
  x+=w+3;

  w=23+6;
  pb = new TPushButton(this, "«");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_prev_track.png");
  pb->setToolTip("previous track");
  CONNECT(pb->sigClicked, this, cmdPreviousTrack);
  x+=w-1;

  pb = new TPushButton(this, "»");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_next_track.png");
  pb->setToolTip("next track");
  CONNECT(pb->sigClicked, this, cmdNextTrack);
  x+=w+3;
  
  pb = new TPushButton(this, "[]");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_stop.png");
  pb->setToolTip("stop playing");
  CONNECT(pb->sigClicked, this, cmdStop);
  x+=w-1;

  pb = new TPushButton(this, "\"");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_pause.png");
  pb->setToolTip("pause playing");
  CONNECT(pb->sigClicked, this, cmdPause);
  x+=w-1;

  w=29+6;
  pb = new TPushButton(this, ">");
  pb->setShape(x,y,w,h);
  pb->loadBitmap("memory://resource/icon_play.png");
  pb->setToolTip("start/continue playing");
  CONNECT(pb->sigClicked, this, cmdPlay);
  x+=w+3;

  _ox = x+15;

  bmpBackground = new TBitmap();
  bmpBackground->load("memory://resource/background.png");
  setBackground(bmpBackground);
  
  _font = new TFont(TFont::SANS, TFont::BOLD, 20);
  setSize(332,98);
  startTimer(1, 0); // once per second
}

void TMainWindow::tick()
{
  cdplayer->FetchPosition();
  invalidateWindow();
}

void TMainWindow::paint()
{
  TPen pen(this);
  pen.setColor(191,191,255);

  int y=10,x=10;
  int h=pen.getHeight()+2;

  pen.drawString(x+35,y, cdplayer->Artist());
  y+=h;
  pen.drawString(x+35,y, cdplayer->Title());
  y+=h;
  pen.drawString(x+35,y, cdplayer->TrackTitle());

  pen.setFont(_font);
  pen.drawString(_ox, 65, cdplayer->PositionString());
}

void TMainWindow::cmdPlay()
{
  cdplayer->Play();
}

void TMainWindow::cmdStop()
{
  cdplayer->StopMotor();
}

void TMainWindow::cmdEject()
{
  cdplayer->Eject();
}

void TMainWindow::cmdNextTrack()
{
  cdplayer->NextTrack();
  invalidateWindow();
}

void TMainWindow::cmdPreviousTrack()
{
  cdplayer->PreviousTrack();
  invalidateWindow();
}

void TMainWindow::cmdPause()
{
  cdplayer->Pause();
}

TCDPlayer::TCDPlayer(const string &device)
{
  _device = device;
  _state  = STATE_NO_DISC;
  info = NULL;

  _fd = open(_device.c_str(), O_RDONLY | O_NONBLOCK);
  if (_fd < 0) {
    cerr << "couldn't open " << _device << endl;
    perror("reason");
    exit(1);
  }
  
  CheckDisc();
}

void TCDPlayer::NextTrack()
{
  if (_state==STATE_NO_DISC)
    return;
  if (_state!=STATE_STOPED) {
    FetchPosition();
    current_track++;
    if (current_track>last_track)
      current_track = first_track;
    _state = STATE_STOPED;
    Play(current_track);
  }
}

void TCDPlayer::PreviousTrack()
{
  if (_state==STATE_NO_DISC)
    return;
  if (_state!=STATE_STOPED) {
    FetchPosition();
    current_track--;
    if (current_track<first_track)
      current_track=last_track;
    _state = STATE_STOPED;
    Play(current_track);
  }
}

void TCDPlayer::StartMotor()
{
  if (ioctl(_fd, CDROMSTART, NULL)) {
    cerr << "CDROMSTART failed" << endl;
    perror("reason");
  }
}

void TCDPlayer::StopMotor()
{
  if (ioctl(_fd, CDROMSTOP, NULL)) {
    cerr << "CDROMSTOP failed" << endl;
    perror("reason");
  }
  if (_state!=STATE_NO_DISC)
    _state = STATE_STOPED;
}

void TCDPlayer::Pause()
{
  if (_state==STATE_PLAYING) {
    if (ioctl(_fd, CDROMPAUSE, NULL)) {
      cerr << "CDROMPAUSE failed" << endl;
      perror("reason");
    }
    _state = STATE_PAUSE;
  }
}

void TCDPlayer::Resume()
{
  if (_state==STATE_PAUSE) {
    if (ioctl(_fd, CDROMRESUME, NULL)) {
      cerr << "CDROMRESUME failed" << endl;
      perror("reason");
    }
    _state = STATE_PLAYING;
  }
}

void TCDPlayer::Eject()
{
  if (_state!=STATE_NO_DISC) {
    if (ioctl(_fd, CDROMEJECT, NULL)) {
      cerr << "CDROMEJECT failed" << endl;
      perror("reason");
      return;
    }
    _state = STATE_NO_DISC;
  }
}

void TCDPlayer::Play(int s, int e)
{ 
  struct cdrom_ti cmd;
  if (_state==STATE_NO_DISC)
    CheckDisc();
  switch(_state) {
    case STATE_STOPED:
      cmd.cdti_trk0 = s;
      cmd.cdti_ind0 = 1;
      cmd.cdti_trk1 = e;
      cmd.cdti_ind1 = 10;
      if(ioctl(_fd, CDROMPLAYTRKIND, &cmd) < 0) {
        cerr << "PLAY AUDIO TRACK INDEX failed\n";
        perror("reason");
      } else
        _state = STATE_PLAYING;
      break;
    case STATE_PAUSE:
      Resume();
      break;
    default:
      break;
  }
}

void TCDPlayer::FetchPosition()
{
  if (_state!=STATE_NO_DISC) {
    _position.cdsc_format = CDROM_MSF;
    if (ioctl(_fd, CDROMSUBCHNL, &_position)<0) {
      if (errno==ENOMEDIUM) {
        _state = STATE_NO_DISC;
        return;
      }

      cerr << "READ CDROM SUB CHANNEL failed\n";
      cerr << "errno:" << errno << endl;
      perror("reason");
      return;
    }
    switch(_position.cdsc_audiostatus) {
      case CDROM_AUDIO_COMPLETED:
      case CDROM_AUDIO_ERROR:
        _state = STATE_STOPED;
        break;
      case CDROM_AUDIO_PLAY:
        _state = STATE_PLAYING;
        break;
      case CDROM_AUDIO_PAUSED:
        _state = STATE_PAUSE;
        break;
    }
    current_track = _position.cdsc_trk;
  }
}

string TCDPlayer::PositionString()
{
  char buffer[20];
  string v="#";
  switch(_state) {
    case STATE_NO_DISC:
      v+="-- --:--";
      break;
    case STATE_STOPED:
      sprintf(buffer, "%02d", last_track-first_track+1); v+=buffer;
      v+=" --:--";
      break;
    default:
      sprintf(buffer, "%02d", current_track); v+=buffer;
      v+=" ";
      sprintf(buffer, "%02d", (int)_position.cdsc_reladdr.msf.minute); v+=buffer;
      v+=":";
      sprintf(buffer, "%02d", (int)_position.cdsc_reladdr.msf.second); v+=buffer;
      break;
  }
  return v;
}

void TCDPlayer::CheckDisc()
{
  int ret;

  ret = ioctl(_fd, CDROM_DRIVE_STATUS, CDSL_CURRENT);
  if (ret<0) {
    cerr << "Couldn't read CD-ROM drive status\n";
  } else {
    switch(ret) {
      case CDS_NO_INFO:
        cerr << "drive status not implemented for cdrom driver" << endl;
        break;
      case CDS_NO_DISC:
        cout << "tray close but no disc" << endl;
        break;
      case CDS_TRAY_OPEN:
        cout << "tray open" << endl;
        break;
      case CDS_DRIVE_NOT_READY:
        cout << "drive not ready" << endl;
        break;
      case CDS_DISC_OK:
//        cout << "tray is closed and disc is ready" << endl;
        break;
      default:
        cout << "unknown drive status" << endl;
    }
  }

  
  _state = STATE_NO_DISC;
  
  if (ret<0 || (ret!=CDS_DISC_OK && ret!=CDS_NO_INFO))
    return;

#if 0 
  ret = ioctl(_fd, CDROM_DISC_STATUS, NULL);
  if (ret<0) {
    cerr << "Couldn't read CD-ROM disc status\n";
  } else {
    cout << "disc type:";
    switch(ret) {
      case CDS_AUDIO:
        cout << "Audio";
        break;
      case CDS_MIXED:
        cout << "Mixed Mode";
        break;
      case CDS_NO_INFO:
        cout << "Unknown";
        break;
      default:
        cout << "Data" << endl;
    }
    cout << endl;
  }
#endif
  
  struct cdrom_tochdr header;
  ret = ioctl(_fd, CDROMREADTOCHDR, &header);
  if (ret<0) {
    cerr << "Couldn't read CDs table of contents" << endl;
    return;
  }

  if (_state==STATE_NO_DISC) {
    _state = STATE_STOPED;
    current_track = current_index = 1;
  }

  first_track=header.cdth_trk0;
  last_track =header.cdth_trk1;

  // try to build an unique id for the CD
  //--------------------------------------
  SHA_INFO sha;
  char buffer[255];
  sha_init(&sha);
  sprintf(buffer, "%02X", first_track);
  sha_update(&sha, buffer, strlen(buffer));
  sprintf(buffer, "%02X", last_track);
  sha_update(&sha, buffer, strlen(buffer));
  struct cdrom_tocentry entry;
  for(int i=0; i<100; i++) {
    if (i==0)
      entry.cdte_track = CDROM_LEADOUT; // pos 0
    else
      entry.cdte_track = i;             // pos i [0-100]
    entry.cdte_format = CDROM_LBA;
    ret = ioctl(_fd, CDROMREADTOCENTRY, &entry);
    if(ret==0) {
      assert(entry.cdte_format==CDROM_LBA);
      sprintf(buffer, "%08X", entry.cdte_addr.lba+150);
    } else {
      sprintf(buffer, "%08X", 0);
    }
    sha_update(&sha, buffer, strlen(buffer));
  }
    
  unsigned char digest[20];
  char *base64;
  sha_final(digest, &sha);
  unsigned long size;
  base64 = (char*)rfc822_binary(digest, 20, &size);
  base64[size]=0;
  string id(base64);
  free(base64);

  // now here's the place to query the database:
  // - local (hmm, should be compatible with freeamp)
  // - remote (fork and get the stuff with urlstream in xml)
  // but hey, i should always try to do a remote query and the first host
  // to query is a local server!
  if (info)
    delete info;
  info = new TCDInfo;

  ifstream in("cd.dat");
  while(in) {
    char buffer[1024];
    in.getline(buffer,1023);
    if(strncmp(buffer,"cd:", 3)==0) {
      if(info->title.empty()) {
        if (id==buffer+3)
          info->id = id;
      } else {
        return;
      }
    }
    if (!info->id.empty()) {
      if (strncmp(buffer, "title:", 6)==0) {
        info->title = buffer+6;
      } else if (strncmp(buffer, "artist:", 7)==0) {
        info->artist = buffer+7;
      } else if (strncmp(buffer, "track:", 6)==0) {
        info->tracks.push_back(new TTrackInfo(buffer+6));
      }
    }
  }
  
  cout << "unknown CD id: `" << id << "'" << endl;
}

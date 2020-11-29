/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2021 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef GMWINDOW_H
#define GMWINDOW_H

class GMRemote;
class GMTrackView;
class GMSourceView;
class GMIconTheme;
class GMPreferencesDialog;
class GMImageView;
class GMCover;
class GMAnimImage;
class GMPresenter;

enum {
  SHOW_NORMAL,
  SHOW_WIZARD,
  SHOW_TRAY
  };

class GMWindow : public FXMainWindow {
  FXDECLARE(GMWindow)
friend class GMRemote;
friend class GMPlayerManager;
friend class GMTrackView;
friend class GMPreferencesDialog;
private:
  FXHorizontalFrame * toolbar = nullptr;
  FXStatusBar       * statusbar = nullptr;
private:
  FXMenuPtr         filemenu;
  FXMenuPtr         editmenu;
  FXMenuPtr         viewmenu;
  FXMenuPtr         playmenu;
  FXMenuPtr         helpmenu;
  FXMenuButton     *volumebutton = nullptr;
  FXPopup          *volumecontrol = nullptr;
  FXSlider         *volumeslider = nullptr;
private:
  FXMenuPtr         menu_library;
  FXMenuPtr         menu_media;
  FXMenuPtr         menu_view;
  FXMenuPtr         menu_gmm;
  GMMenuButton    * menubutton_library = nullptr;
  GMMenuButton    * menubutton_media = nullptr;
  GMMenuButton    * menubutton_gmm = nullptr;

  GMIconTheme       * icontheme = nullptr;
  GMTrackView       * trackview = nullptr;
  GMSourceView      * sourceview = nullptr;
  GMCoverFrame      * coverframe = nullptr;
#ifdef HAVE_OPENGL
  GMImageView       * coverview_gl = nullptr;
#endif
  FXImageFrame      * coverview_x11 = nullptr;
  FXToggleButton    * playpausebutton = nullptr;
  FXButton          * stopbutton = nullptr;
  FXButton          * nextbutton = nullptr;
  FXButton          * prevbutton = nullptr;
  FXLabel           * label_nowplaying = nullptr;
  FXMenuCheck       * fullscreencheck = nullptr;
  FXDragCorner 			* controldragcorner = nullptr;
  FXTextField       * time_progress = nullptr;
  FXTextField      	* time_remaining = nullptr;
  GMTrackProgressBar* trackslider = nullptr;
  FX4Splitter       * mainsplitter = nullptr;
  FXHorizontalFrame * progressbar = nullptr;
  FXLabel           * progressbar_label = nullptr;
  GMAnimImage       * progressbar_animation = nullptr;
  GMRemote          * remote = nullptr;
  GMPresenter       * presenter = nullptr;
  FXLabel           * lyricsview = nullptr;
  FXSwitcher        * metaview = nullptr;
private:
  void configureToolbar(FXbool docktop,FXbool init=false);
  void configureStatusbar(FXbool show);
  FXbool showSources() const;
  void updateCover();
  void clearCover();
private:
  GMWindow(){}
  GMWindow(const GMWindow&);
  GMWindow& operator=(const GMWindow&);
public: /// Message Handlers
  long onCmdAbout(FXObject*,FXSelector,void*);
  long onCmdQuit(FXObject*,FXSelector,void*);
  long onCmdPreferences(FXObject*,FXSelector,void*);

  long onCmdTimeSlider(FXObject*,FXSelector,void*);
  long onCmdVolume(FXObject*,FXSelector,void*);
  long onCmdVolumeButton(FXObject*,FXSelector,void*);

  long onCmdOpen(FXObject*,FXSelector,void*);

  long onCmdImport(FXObject*,FXSelector,void*);
  long onCmdImportFiles(FXObject*,FXSelector,void*);
  long onCmdShowFullScreen(FXObject*,FXSelector,void*);
  long onCmdShowSources(FXObject*,FXSelector,void*);
  long onUpdShowSources(FXObject*,FXSelector,void*);
  long onCmdShowBrowser(FXObject*,FXSelector,void*);
  long onCmdShowMiniPlayer(FXObject*,FXSelector,void*);
  long onUpdShowMiniPlayer(FXObject*,FXSelector,void*);
  long onCmdShowPresenter(FXObject*,FXSelector,void*);

  long onCmdPlayPause(FXObject*,FXSelector,void*);
  long onUpdPlayPause(FXObject*,FXSelector,void*);
  long onUpdPlayPauseMenu(FXObject*,FXSelector,void*);
  long onCmdPlay(FXObject*,FXSelector,void*);
  long onUpdPlay(FXObject*,FXSelector,void*);
  long onCmdPause(FXObject*,FXSelector,void*);
  long onUpdPause(FXObject*,FXSelector,void*);
  long onUpdScheduleStop(FXObject*,FXSelector,void*);
  long onCmdScheduleStop(FXObject*,FXSelector,void*);
  long onCmdStop(FXObject*,FXSelector,void*);
  long onUpdStop(FXObject*,FXSelector,void*);
  long onCmdNext(FXObject*,FXSelector,void*);
  long onUpdNext(FXObject*,FXSelector,void*);
  long onCmdPrev(FXObject*,FXSelector,void*);
  long onUpdPrev(FXObject*,FXSelector,void*);
  long onCmdRepeatAll(FXObject*,FXSelector,void*);
  long onUpdRepeatAll(FXObject*,FXSelector,void*);
  long onCmdRepeatAB(FXObject*,FXSelector,void*);
  long onUpdRepeatAB(FXObject*,FXSelector,void*);
  long onCmdRepeatOff(FXObject*,FXSelector,void*);
  long onUpdRepeatOff(FXObject*,FXSelector,void*);
  long onCmdRepeat(FXObject*,FXSelector,void*);
  long onUpdRepeat(FXObject*,FXSelector,void*);
  long onCmdSleepTimer(FXObject*,FXSelector,void*);
  long onUpdSleepTimer(FXObject*,FXSelector,void*);
  long onCmdShuffle(FXObject*,FXSelector,void*);
  long onUpdShuffle(FXObject*,FXSelector,void*);
  long onCmdJoinLastFM(FXObject*,FXSelector,void*);
  long onCmdJoinGMMLastFM(FXObject*,FXSelector,void*);

  long onCmdResetColors(FXObject*,FXSelector,void*);
  long onCmdPlayQueue(FXObject*,FXSelector,void*);
  long onUpdPlayQueue(FXObject*,FXSelector,void*);

  long onConfigureCoverView(FXObject*,FXSelector,void*);

  long onCmdNextFocus(FXObject*,FXSelector,void*);


  long onCmdLyricView(FXObject*,FXSelector,void*);
  long onCmdCoverView(FXObject*,FXSelector,void*);

  long onMetaContextMenu(FXObject*,FXSelector,void*);
public:
  enum{
    ID_ABOUT=FXMainWindow::ID_LAST,
    ID_QUIT,

    ID_OPEN,
    ID_JOIN_LASTFM,
    ID_JOIN_GMM_LASTFM,

    ID_PAUSE,
    ID_PLAYPAUSE,
    ID_PLAYPAUSEMENU,
    ID_STOP,
    ID_SCHEDULE_STOP,
    ID_NEXT,
    ID_PREV,

    ID_SEEK_FORWARD_10SEC,
    ID_SEEK_FORWARD_1MIN,
    ID_SEEK_BACKWARD_10SEC,
    ID_SEEK_BACKWARD_1MIN,

    ID_REPEAT,
    ID_REPEAT_ALL,
    ID_REPEAT_AB,
    ID_REPEAT_OFF,
    ID_SHUFFLE,

    ID_TIMESLIDER,
    ID_VOLUME_BUTTON,
    ID_VOLUME_SLIDER,

    ID_DISPLAYMODE,

    ID_DATABASE_CLEAR,

    ID_IMPORT_DIRS,
    ID_IMPORT_FILES,
    ID_SYNC_DIRS,
    ID_REMOVE_FOLDER,


    ID_PREFERENCES,
    ID_SHOW_TRACK,
    ID_SHOW_FULLSCREEN,
    ID_SHOW_SOURCES,
    ID_SHOW_MINIPLAYER,
    ID_SHOW_BROWSER,
    ID_SHOW_PRESENTER,

    ID_OPEN_DIR,


    ID_RESET_COLORS,

    ID_DDE_MESSAGE,
    ID_SLEEP,

    ID_LYRICVIEW,
    ID_COVERVIEW,
    ID_REFRESH_COVERVIEW,
    ID_CHANGE_COVERVIEW,
    ID_COVERSIZE_SMALL,
    ID_COVERSIZE_MEDIUM,
    ID_COVERSIZE_LARGE,
    ID_COVERSIZE_EXTRALARGE,
    ID_PLAYQUEUE,


    ID_NEXT_FOCUS,

    ID_LAST
    };
public:
  GMWindow(FXApp* a,FXObject*tgt,FXSelector sel);


  GMRemote * getRemote() const { return remote; }


  void updateCoverView();

  void create_dialog_header(FXDialogBox * dialog,const FXString & title,const FXString & label,FXIcon * icon=nullptr);

  FXbool question(const FXString & title,const FXString & label,const FXString & accept,const FXString & cancel);


  void reset();
  void display(const GMTrack&);

  void showRemote();
  void hideRemote();
  void showPresenter();
  void hidePresenter();

  void init(FXuint);
  void toggleShown();

  /// Create window
  virtual void create();
  virtual void show();
  virtual void hide();

  GMTrackView * getTrackView() const { return trackview; }
  GMSourceView  * getSourceView() const { return sourceview; }


  void update_time(const TrackTime & current,const TrackTime & remaining,FXint position,FXbool playing,FXbool seekable);
  void update_volume_display(FXint level);
  void update_cover_display();

  void update_meta_display();

  void setStatus(const FXString& status);

  void raiseWindow();

  void setFullScreen(FXbool show);

  /// Destructor
  virtual ~GMWindow();
  };

#endif

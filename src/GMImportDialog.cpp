/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2009-2021 by Sander Jansen. All Rights Reserved      *
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
#include <tag.h>
#include "gmdefs.h"
#include <FXArray.h>
#include <fxkeys.h>
#include "icons.h"
#include "GMTrack.h"
#include "GMApp.h"
#include "GMIconTheme.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMFilename.h"
#include "GMImportDialog.h"


static const FXchar * const imagetypes[]={
  "png",";Portable Network Graphics",
  "jpg",";JPEG",
  "jpeg",";JPEG",
  "bmp",";BMP",
  "gif",";GIF"
  };

static const FXchar * const filetypes[]={
  "mp3",";MPEG-1 Audio Layer 3",
  "ogg",";Ogg Vorbis",
  "oga",";Ogg Audio",
  "opus",";Opus Audio",
  "flac",";Free Lossless Audio Codec",
#if 0
  "mpc",";Musepack",
  "wma",";Windows Media",
  "asf",";Windows Media",
#endif
  "m4a",";MPEG-4 Part 14",
  "m4b",";MPEG-4 Part 14",
  "m4p",";MPEG-4 Part 14",
  "aac",";MPEG-4 Part 14",
  "m3u",";M3U Playlist",
  "pls",";PLS Playlist",
  "xspf",";XML Shareable Playlist"
  };


extern const FXchar gmfilepatterns[]="All Music (*.mp3,*.ogg,*.oga,*.opus,*.flac"
",*.aac,*.m4a,*.m4p,*.mp4,*.m4b"
#if 0
",*.mpc,*.wma,*.asf"
#endif
")\nFree Lossless Audio Codec (*.flac)\n"
"MPEG-1 Audio Layer 3 (*.mp3)\n"
"MPEG-4 Part 14 (*.mp4,*.m4a,*.m4p,*.aac,*.m4b)\n"
#if 0
"Musepack (*.mpc)\n"
#endif
"Ogg Vorbis (*.ogg)\n"
"Ogg Audio (*.oga)\n"
"Opus Audio (*.opus)\n"
#if 0
"Window Media (*.wma,*,asf)\n"
#endif
"All Files (*.*)";


const FXchar playlist_patterns[]="All Playlists (*.xspf,*.pls,*.m3u)\nXML Shareable Playlist (*.xspf)\nPLS (*.pls)\nM3U (*.m3u)";




class GMFileAssociations : public FXFileAssociations {

public:
  GMFileAssociations(FXApp*app) : FXFileAssociations(app) {
    FXFileAssoc* assoc = new FXFileAssoc;
    assoc->bigicon  = GMIconTheme::instance()->icon_file_big;
    assoc->miniicon = GMIconTheme::instance()->icon_file_small;
    bindings[FXFileAssociations::defaultFileBinding]=assoc;

    assoc = new FXFileAssoc;
    assoc->bigicon      = GMIconTheme::instance()->icon_folder_big;
    assoc->miniicon     = GMIconTheme::instance()->icon_folder_small;
    assoc->miniiconopen = GMIconTheme::instance()->icon_folder_open_small;
    bindings[FXFileAssociations::defaultDirBinding]=assoc;
    }

  void initFileBindings() {
    FXFileAssoc * assoc=nullptr;

    for (FXuint i=0;i<ARRAYNUMBER(filetypes);i+=2) {
      assoc = new FXFileAssoc;
      assoc->extension = filetypes[i+1];
      assoc->bigicon  = GMIconTheme::instance()->icon_audio_big;
      assoc->miniicon = GMIconTheme::instance()->icon_audio_small;
      bindings[filetypes[i]]=assoc;
      }

    for (FXuint i=0;i<ARRAYNUMBER(imagetypes);i+=2){
      assoc = new FXFileAssoc;
      assoc->extension = imagetypes[i+1];
      assoc->bigicon  = GMIconTheme::instance()->icon_image_big;
      assoc->miniicon = GMIconTheme::instance()->icon_image_small;
      bindings[imagetypes[i]]=assoc;
      }
    }

  FXFileAssoc* findFileBinding(const FXString& pathname){
    FXString ext = FXPath::extension(pathname);
    if (!ext.empty()) {
      FXFileAssoc * record = nullptr;
      if ((record = bindings[ext])!=nullptr) return record;
      if ((record = bindings[ext.lower()])!=nullptr) return record;
      }
    return bindings[defaultFileBinding];
    }

  FXFileAssoc* findDirBinding(const FXString&){
    return bindings[defaultDirBinding];
    }

  ~GMFileAssociations() {
    }
  };


class GMDirSelector : public FXDirSelector {
FXDECLARE(GMDirSelector)
protected:
  GMDirSelector(){}
private:
  GMDirSelector(const GMDirSelector&);
  GMDirSelector &operator=(const GMDirSelector&);
public:
  GMDirSelector(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  ~GMDirSelector();
  };


FXIMPLEMENT(GMDirSelector,FXDirSelector,nullptr,0);

GMDirSelector::GMDirSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXDirSelector(p,tgt,sel,opts,x,y,w,h) {
  delete accept->getParent();
  delete dirbox->getParent();
  delete dirname->getParent();

  FXHorizontalFrame * fields=new FXHorizontalFrame(this,LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
  new FXLabel(fields,tr("&Folder:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  dirname=new GMTextField(fields,25,this,ID_DIRNAME,LAYOUT_FILL_X|LAYOUT_CENTER_Y|FRAME_SUNKEN|FRAME_THICK);

  GMScrollFrame *frame=new GMScrollFrame(this);
  dirbox=new FXDirList(frame,this,ID_DIRLIST,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_TOP|TREELIST_SHOWS_LINES|TREELIST_SHOWS_BOXES|TREELIST_BROWSESELECT);

  GMScrollArea::replaceScrollbars(dirbox);
  ((FXVerticalFrame*)dirbox->getParent())->setFrameStyle(FRAME_LINE);

  GMFileAssociations * assoc = new GMFileAssociations(getApp());
  dirbox->setAssociations(assoc,true);
  }

GMDirSelector::~GMDirSelector(){
  }



class GMFileSelector : public FXFileSelector {
FXDECLARE(GMFileSelector)
protected:
  GMFileAssociations * fileassoc = nullptr;
protected:
  GMFileSelector(){}
private:
  GMFileSelector(const GMFileSelector&);
  GMFileSelector &operator=(const GMFileSelector&);
public:
  GMFileSelector(FXComposite *p,FXObject* tgt=nullptr,FXSelector sel=0,FXuint opts=0,FXint x=0,FXint y=0,FXint w=0,FXint h=0);
  ~GMFileSelector();

  void hideButtons();
  void getSelectedFiles(FXStringList & files);

  FXMatrix * optionFrame() const { return entryblock; }
  };

FXIMPLEMENT(GMFileSelector,FXFileSelector,nullptr,0);

GMFileSelector::GMFileSelector(FXComposite *p,FXObject* tgt,FXSelector sel,FXuint opts,FXint x,FXint y,FXint w,FXint h):FXFileSelector(p,tgt,sel,opts,x,y,w,h) {


  FXWindow * window=nullptr;
  while((window=getFirst())!=nullptr) delete window;
  delete bookmarkmenu; bookmarkmenu=nullptr;

  FXAccelTable *table=getShell()->getAccelTable();

  navbuttons=new FXHorizontalFrame(this,LAYOUT_SIDE_TOP|LAYOUT_FILL_X,0,0,0,0, 0,0,0,0, 0,0);
  entryblock=new FXMatrix(this,3,MATRIX_BY_COLUMNS|LAYOUT_SIDE_BOTTOM|LAYOUT_FILL_X,0,0,0,0, 0,0,0,0);
  new FXLabel(entryblock,tr("&File Name:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  filename=new GMTextField(entryblock,25,this,ID_ACCEPT,TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y);
  new GMButton(entryblock,tr("&OK"),nullptr,this,ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  accept=new FXButton(navbuttons,FXString::null,nullptr,nullptr,0,LAYOUT_FIX_X|LAYOUT_FIX_Y|LAYOUT_FIX_WIDTH|LAYOUT_FIX_HEIGHT,0,0,0,0, 0,0,0,0);
  accept->hide();
  new FXLabel(entryblock,tr("File F&ilter:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  FXHorizontalFrame *filterframe=new FXHorizontalFrame(entryblock,LAYOUT_FILL_COLUMN|LAYOUT_FILL_X|LAYOUT_FILL_Y,0,0,0,0, 0,0,0,0);
  filefilter=new GMComboBox(filterframe,10,this,ID_FILEFILTER,COMBOBOX_STATIC|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_CENTER_Y);
  filefilter->setNumVisible(4);
  readonly=new GMCheckButton(filterframe,tr("Read Only"),nullptr,0,ICON_BEFORE_TEXT|JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  cancel=new GMButton(entryblock,tr("&Cancel"),nullptr,nullptr,0,BUTTON_DEFAULT|FRAME_RAISED|FRAME_THICK|LAYOUT_FILL_X,0,0,0,0,20,20);
  fileboxframe=new GMScrollHFrame(this);
  filebox=new FXFileList(fileboxframe,this,ID_FILELIST,ICONLIST_MINI_ICONS|ICONLIST_BROWSESELECT|ICONLIST_AUTOSIZE|LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->setDraggableFiles(false);
  filebox->setFocus();
  GMScrollArea::replaceScrollbars(filebox);
  new FXLabel(navbuttons,tr("Directory:"),nullptr,LAYOUT_CENTER_Y);
  dirbox=new FXDirBox(navbuttons,this,ID_DIRTREE,DIRBOX_NO_OWN_ASSOC|FRAME_LINE|LAYOUT_FILL_X|LAYOUT_CENTER_Y,0,0,0,0,1,1,1,1);
  dirbox->setNumVisible(5);
  //dirbox->setAssociations(filebox->getAssociations(),false);    // Shared file associations
  GMTreeListBox::replace(dirbox);


  bookmarkmenu=new GMMenuPane(this,POPUP_SHRINKWRAP);
  new GMMenuCommand(bookmarkmenu,tr("&Set bookmark\t\tBookmark current directory."),bookaddicon,this,ID_BOOKMARK);
  new GMMenuCommand(bookmarkmenu,tr("&Unset bookmark\t\tRemove current directory bookmark."),bookdelicon,this,ID_UNBOOKMARK);
  new GMMenuCommand(bookmarkmenu,tr("&Clear all bookmarks\t\tClear all bookmarks."),bookclricon,&bookmarks,FXRecentFiles::ID_CLEAR);
  FXMenuSeparator* sep1=new FXMenuSeparator(bookmarkmenu);
  sep1->setTarget(&bookmarks);
  sep1->setSelector(FXRecentFiles::ID_ANYFILES);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_1);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_2);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_3);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_4);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_5);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_6);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_7);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_8);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_9);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_10);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_11);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_12);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_13);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_14);
  new GMMenuCommand(bookmarkmenu,FXString::null,nullptr,&bookmarks,FXRecentFiles::ID_FILE_15);
  new FXFrame(navbuttons,LAYOUT_FIX_WIDTH,0,0,4,1);
  new GMButton(navbuttons,tr("\tGo up one directory\tMove up to higher directory."),updiricon,this,ID_DIRECTORY_UP,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tGo to home directory\tBack to home directory."),homeicon,this,ID_HOME,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tGo to work directory\tBack to working directory."),workicon,this,ID_WORK,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  FXMenuButton *bookmenu=new FXMenuButton(navbuttons,tr("\tBookmarks\tVisit bookmarked directories."),bookmarkicon,bookmarkmenu,MENUBUTTON_NOARROWS|MENUBUTTON_ATTACH_LEFT|MENUBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  bookmenu->setTarget(this);
  bookmenu->setSelector(ID_BOOKMENU);
  new GMButton(navbuttons,tr("\tCreate new directory\tCreate new directory."),newicon,this,ID_NEW,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow list\tDisplay directory with small icons."),listicon,filebox,FXFileList::ID_SHOW_MINI_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow icons\tDisplay directory with big icons."),iconsicon,filebox,FXFileList::ID_SHOW_BIG_ICONS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMButton(navbuttons,tr("\tShow details\tDisplay detailed directory listing."),detailicon,filebox,FXFileList::ID_SHOW_DETAILS,BUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  new GMToggleButton(navbuttons,tr("\tShow hidden files\tShow hidden files and directories."),tr("\tHide Hidden Files\tHide hidden files and directories."),hiddenicon,shownicon,filebox,FXFileList::ID_TOGGLE_HIDDEN,TOGGLEBUTTON_TOOLBAR|FRAME_RAISED,0,0,0,0, 3,3,3,3);
  readonly->hide();
  if(table){
    table->addAccel(MKUINT(KEY_BackSpace,0),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_DIRECTORY_UP));
    table->addAccel(MKUINT(KEY_Delete,0),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_REMOVE));
    table->addAccel(MKUINT(KEY_h,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_HOME));
    table->addAccel(MKUINT(KEY_w,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_WORK));
    table->addAccel(MKUINT(KEY_n,CONTROLMASK),this,FXSEL(SEL_COMMAND,FXFileSelector::ID_NEW));
    table->addAccel(MKUINT(KEY_a,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SELECT_ALL));
    table->addAccel(MKUINT(KEY_b,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_BIG_ICONS));
    table->addAccel(MKUINT(KEY_s,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_MINI_ICONS));
    table->addAccel(MKUINT(KEY_l,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_SHOW_DETAILS));
//    table->addAccel(MKUINT(KEY_c,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_COPY_SEL));
//    table->addAccel(MKUINT(KEY_x,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_CUT_SEL));
//    table->addAccel(MKUINT(KEY_v,CONTROLMASK),filebox,FXSEL(SEL_COMMAND,FXFileList::ID_PASTE_SEL));
    }

  // Now use up to 15 bookmarked directories
  bookmarks.setMaxFiles(15);
  bookmarks.setTarget(this);
  bookmarks.setSelector(ID_VISIT);

  // For backward compatibility, this HAS to be the default!
  setSelectMode(SELECTFILE_ANY);

  // Initial pattern
  setPatternList(tr("All Files (*)"));

  // Consistent value in dir box; don't rescan just yet!
  dirbox->setDirectory(filebox->getDirectory());

  // Default is navigation is allowed
  navigable=true;



  fileassoc = new GMFileAssociations(getApp());
  fileassoc->initFileBindings();
  filebox->setAssociations(fileassoc,false);
  dirbox->setAssociations(fileassoc,false);
  }

GMFileSelector::~GMFileSelector(){
  delete fileassoc;
  }


void GMFileSelector::hideButtons() {
  entryblock->childAtIndex(2)->hide();
  cancel->hide();
  }

void GMFileSelector::getSelectedFiles(FXStringList & files) {
  if(selectmode==SELECTFILE_MULTIPLE_ALL){
    for(FXint i=0; i<filebox->getNumItems(); i++){
      if(filebox->isItemSelected(i) && filebox->getItemFilename(i)!=".." && filebox->getItemFilename(i)!="."){
        files.append(filebox->getItemPathname(i));
        }
      }
    }
  else {
    /// implement me
    FXASSERT(0);
    }
  }


FXIMPLEMENT(GMFileDialog,FXFileDialog,nullptr,0);

// Construct file fialog box
GMFileDialog::GMFileDialog(FXWindow* o,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXFileDialog(o,name,opts,x,y,w,h){
  delete filebox;
  filebox=new GMFileSelector(this,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->acceptButton()->setTarget(this);
  filebox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  filebox->cancelButton()->setTarget(this);
  filebox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }

// Construct free-floating file dialog box
GMFileDialog::GMFileDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  FXFileDialog(a,name,opts,x,y,w,h){
  delete filebox;
  filebox=new GMFileSelector(this,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  filebox->acceptButton()->setTarget(this);
  filebox->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
  filebox->cancelButton()->setTarget(this);
  filebox->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
  }



FXIMPLEMENT(GMExportDialog,GMFileDialog,nullptr,0);

// Construct file fialog box
GMExportDialog::GMExportDialog(FXWindow* o,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  GMFileDialog(o,name,opts,x,y,w,h){
  GMFileSelector * fileselector = static_cast<GMFileSelector*>(filebox);
  FXMatrix * entryblock = fileselector->optionFrame();
  new FXLabel(entryblock,tr("&Options:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  check_relative=new GMCheckButton(entryblock,"Relative Paths",nullptr,0,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);
  new FXFrame(entryblock,FRAME_NONE);
  }

// Construct free-floating file dialog box
GMExportDialog::GMExportDialog(FXApp* a,const FXString& name,FXuint opts,FXint x,FXint y,FXint w,FXint h):
  GMFileDialog(a,name,opts,x,y,w,h){
  GMFileSelector * fileselector = static_cast<GMFileSelector*>(filebox);
  FXMatrix * entryblock = fileselector->optionFrame();
  new FXLabel(entryblock,tr("&Options:"),nullptr,JUSTIFY_LEFT|LAYOUT_CENTER_Y);
  check_relative=new GMCheckButton(entryblock,"Relative Paths",nullptr,0,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);
  new FXFrame(entryblock,FRAME_NONE);
  }



FXDEFMAP(GMImportDialog) GMImportDialogMap[]={
  FXMAPFUNC(SEL_UPDATE,FXDialogBox::ID_ACCEPT,GMImportDialog::onUpdAccept),
  FXMAPFUNCS(SEL_UPDATE,GMImportDialog::ID_SYNC_NEW,GMImportDialog::ID_SYNC_UPDATE_MODIFIED,GMImportDialog::onUpdSync),
  FXMAPFUNCS(SEL_COMMAND,GMImportDialog::ID_SYNC_NEW,GMImportDialog::ID_SYNC_UPDATE_MODIFIED,GMImportDialog::onCmdSync),
  FXMAPFUNC(SEL_COMMAND,GMImportDialog::ID_PARSE_METHOD,GMImportDialog::onCmdParseMethod)

  };

FXIMPLEMENT(GMImportDialog,FXDialogBox,GMImportDialogMap,ARRAYNUMBER(GMImportDialogMap))

GMImportDialog::GMImportDialog(FXWindow *p,FXuint m) : FXDialogBox(p,FXString::null,DECOR_BORDER|DECOR_TITLE|DECOR_RESIZE,0,0,500,400,0,0,0,0,0,0), mode(m) {

  if (mode&REMOVE_FOLDER)
    setTitle(tr("Remove Folder"));
  else if (mode&IMPORT_SYNC)
    setTitle(tr("Synchronize Folder"));
  else if (mode&IMPORT_PLAYLIST)
    setTitle(tr("Import Playlist"));
  else
    setTitle(tr("Import Music"));


  /// Create a fixed font, about the same size as the normal font
  FXint size = FXApp::instance()->getNormalFont()->getSize();
  font_fixed = new FXFont(FXApp::instance(),"mono",(int)size/10,FXFont::Normal,FXFont::Straight,FONTENCODING_UNICODE,FXFont::NonExpanded,FXFont::Modern|FXFont::Fixed);

  target_track_from_filelist.connect(GMPlayerManager::instance()->getPreferences().import.track_from_filelist);
  target_replace_underscores.connect(GMPlayerManager::instance()->getPreferences().import.replace_underscores);
  target_parse_method.connect(GMPlayerManager::instance()->getPreferences().import.parse_method,this,ID_PARSE_METHOD);
  target_filename_template.connect(GMPlayerManager::instance()->getPreferences().import.filename_template);
  target_album_format_grouping.connect(GMPlayerManager::instance()->getPreferences().import.album_format_grouping);
  target_detect_compilation.connect(GMPlayerManager::instance()->getPreferences().import.detect_compilation);

  target_exclude_dir.connect(GMPlayerManager::instance()->getPreferences().import.exclude_folder);
  target_exclude_file.connect(GMPlayerManager::instance()->getPreferences().import.exclude_file);
  target_id3v1_encoding.connect(GMPlayerManager::instance()->getPreferences().import.id3v1_encoding);

  target_fetch_lyrics.connect(GMPlayerManager::instance()->getPreferences().import.fetch_lyrics);
  target_playback_only.connect(GMPlayerManager::instance()->getPreferences().import.playback_only);

  const FXuint labelstyle=LAYOUT_CENTER_Y|LABEL_NORMAL|LAYOUT_RIGHT;
  const FXuint textfieldstyle=TEXTFIELD_ENTER_ONLY|LAYOUT_FILL_X|FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN;

  FXGroupBox * grpbox;
  FXMatrix   * matrix;
  FXVerticalFrame * vframe;
  FXHorizontalFrame * hframe;

  FXVerticalFrame * main=new FXVerticalFrame(this,LAYOUT_FILL_X|LAYOUT_FILL_Y);
  GMTabBook * tabbook=new GMTabBook(main,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y|LAYOUT_RIGHT,0,0,0,0,0,0,0,0);


  if (mode&IMPORT_FROMFILE) {
    new GMTabItem(tabbook,tr("&File(s)"),nullptr,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);
    fileselector = new GMFileSelector(vframe,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    FXString searchdir = getApp()->reg().readStringEntry("directories","last-import-files",nullptr);
    if (searchdir.empty()) getDefaultSearchDirectory(searchdir);
    if (mode&IMPORT_PLAYLIST) {
      fileselector->setPatternList(playlist_patterns);
      fileselector->setSelectMode(SELECTFILE_EXISTING);
      }
    else {
      fileselector->setPatternList(gmfilepatterns);
      fileselector->setSelectMode(SELECTFILE_MULTIPLE);
      }
    fileselector->setCurrentPattern(0);
    fileselector->setMatchMode(FXPath::CaseFold);
    fileselector->setDirectory(searchdir);
    fileselector->hideButtons();
    fileselector->acceptButton()->setTarget(this);
    fileselector->acceptButton()->setSelector(FXDialogBox::ID_ACCEPT);
    fileselector->cancelButton()->setTarget(this);
    fileselector->cancelButton()->setSelector(FXDialogBox::ID_CANCEL);
    }
  else if (mode&IMPORT_FROMDIR) {
    new GMTabItem(tabbook,tr("&Folder"),nullptr,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);

    dirselector = new GMDirSelector(vframe,nullptr,0,LAYOUT_FILL_X|LAYOUT_FILL_Y);
    FXString searchdir = getApp()->reg().readStringEntry("directories","last-import-dirs",nullptr);
    if (searchdir.empty()) getDefaultSearchDirectory(searchdir);
    dirselector->setDirectory(searchdir);
    }

  if (mode&IMPORT_SYNC) {
    new GMTabItem(tabbook,tr("&Sync"),nullptr,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);
    grpbox =  new FXGroupBox(vframe,tr("Sync Operation"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
    grpbox->setFont(GMApp::instance()->getThickFont());

    new GMCheckButton(grpbox,tr("Import new tracks\tImports files not yet in the database."),this,ID_SYNC_NEW);
    new GMCheckButton(grpbox,tr("Remove tracks that have been deleted from disk"),this,ID_SYNC_REMOVE_MISSING);
    matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,0,0,0,0);
    new GMCheckButton(matrix,tr("Update existing tracks:"),this,ID_SYNC_UPDATE);
    new FXRadioButton(matrix,tr("Modified since last import\tOnly reread the tag when the file has been modified."),this,ID_SYNC_UPDATE_MODIFIED,RADIOBUTTON_NORMAL|LAYOUT_CENTER_Y|LAYOUT_LEFT,0,0,0,0);
    new FXFrame(matrix,FRAME_NONE);
    new FXRadioButton(matrix,tr("All\tAlways read the tags"),this,ID_SYNC_UPDATE_ALL,RADIOBUTTON_NORMAL|LAYOUT_CENTER_Y|LAYOUT_LEFT,0,0,0,0);
    }

  if ((mode&REMOVE_FOLDER)==0) {

    new GMTabItem(tabbook,tr("&Track"),nullptr,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);

    grpbox = new FXGroupBox(vframe,tr("Parse Settings"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
    grpbox->setFont(GMApp::instance()->getThickFont());

    matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X);
    new FXLabel(matrix,tr("Parse info from:"),nullptr,labelstyle);

    hframe = new FXHorizontalFrame(matrix,LAYOUT_FILL_COLUMN,0,0,0,0,0,0,0,0);
    new GMRadioButton(hframe,tr("Tag"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_TAG,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);
    new GMRadioButton(hframe,tr("Path"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_FILENAME,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);
    new GMRadioButton(hframe,tr("Both"),&target_parse_method,FXDataTarget::ID_OPTION+GMImportOptions::PARSE_BOTH,JUSTIFY_LEFT|ICON_BEFORE_TEXT|LAYOUT_CENTER_Y);

    new FXLabel(matrix,tr("ID3v1 Encoding:"),nullptr,labelstyle);
    id3v1_listbox = new GMListBox(matrix,&target_id3v1_encoding,FXDataTarget::ID_VALUE,FRAME_SUNKEN|FRAME_THICK|LAYOUT_FILL_COLUMN);
    for (int i=0;gmcodecnames[i]!=nullptr;i++)
      id3v1_listbox->appendItem(gmcodecnames[i]);
    id3v1_listbox->setNumVisible(9);

    if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_FILENAME) {
      id3v1_listbox->disable();
      }

    new FXFrame(matrix,FRAME_NONE);
    new GMCheckButton(matrix,tr("Set track number based on scan order."),&target_track_from_filelist,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

    new FXFrame(matrix,FRAME_NONE);
    new GMCheckButton(matrix,tr("Group albums by audio format.\tImported tracks that differ in audio format will be grouped in separate albums."),&target_album_format_grouping,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

    new FXFrame(matrix,FRAME_NONE);
    new GMCheckButton(matrix,tr("Detect compilations by folder.\tGroup tracks from the same folder without specific compilation/albumartist tags."),&target_detect_compilation,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

    new FXFrame(matrix,FRAME_NONE);
    new GMCheckButton(matrix,tr("Fetch Lyrics.\tFetch Lyrics"),&target_fetch_lyrics,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

    template_grpbox =  new FXGroupBox(vframe,tr("Path Template"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
    template_grpbox->setFont(GMApp::instance()->getThickFont());

    FXLabel * label = new FXLabel(template_grpbox,tr("%T - title              %A - album name\n"
                                                     "%P - album artist name  %p - track artist name \n"
                                                     "%N - track number       %d - disc number\n"
                                                     "%y - year"),nullptr,FRAME_LINE|JUSTIFY_LEFT,0,0,0,0,30);

    label->setFont(font_fixed);
    label->setBackColor(getApp()->getTipbackColor());
    label->setBorderColor(getApp()->getShadowColor());

    new FXSeparator(template_grpbox,SEPARATOR_GROOVE|LAYOUT_FILL_X);
    matrix = new FXMatrix(template_grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X);
    new FXLabel(matrix,tr("Template:"),nullptr,labelstyle);
    new GMTextField(matrix,10,&target_filename_template,FXDataTarget::ID_VALUE,textfieldstyle);
    new FXFrame(matrix,FRAME_NONE);
    new GMCheckButton(matrix,tr("Replace underscores with spaces"),&target_replace_underscores,FXDataTarget::ID_VALUE,CHECKBUTTON_NORMAL|LAYOUT_FILL_COLUMN);

    if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_TAG) {
      template_grpbox->hide();
      }
    else {
      template_grpbox->show();
      }


    /// Filter Panel
    new GMTabItem(tabbook,tr("&Filter"),nullptr,TAB_TOP_NORMAL,0,0,0,0,5,5);
    vframe = new GMTabFrame(tabbook);

    grpbox =  new FXGroupBox(vframe,tr("Exclude Files"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,36,4);
    new GMCheckButton(grpbox,tr("Unsupported Playback Formats"),&target_playback_only,FXDataTarget::ID_VALUE,LAYOUT_FILL_COLUMN|CHECKBUTTON_NORMAL);

    grpbox = new FXGroupBox(vframe,tr("Exclude by Pattern"),FRAME_NONE|LAYOUT_FILL_X,0,0,0,0,20);
    //grpbox->setFont(GMApp::instance()->getThickFont());

    matrix = new FXMatrix(grpbox,2,MATRIX_BY_COLUMNS|LAYOUT_FILL_X,0,0,0,0,16,4,0,0);
    new FXLabel(matrix,tr("Folders:"),nullptr,labelstyle);
    new GMTextField(matrix,20,&target_exclude_dir,FXDataTarget::ID_VALUE,textfieldstyle);
    new FXLabel(matrix,tr("Files:"),nullptr,labelstyle);
    new GMTextField(matrix,20,&target_exclude_file,FXDataTarget::ID_VALUE,textfieldstyle);
    new FXFrame(matrix,FRAME_NONE);






    }


  FXHorizontalFrame *closebox=new FXHorizontalFrame(main,LAYOUT_BOTTOM|LAYOUT_FILL_X|PACK_UNIFORM_WIDTH,0,0,0,0,0,0,0,0);
  if (mode&IMPORT_SYNC)
    new GMButton(closebox,tr("&Sync"),nullptr,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  else if(mode&REMOVE_FOLDER)
    new GMButton(closebox,tr("&Remove"),nullptr,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  else
    new GMButton(closebox,tr("&Import"),nullptr,this,FXDialogBox::ID_ACCEPT,BUTTON_INITIAL|BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  new GMButton(closebox,tr("&Cancel"),nullptr,this,FXDialogBox::ID_CANCEL,BUTTON_DEFAULT|LAYOUT_RIGHT|FRAME_RAISED|FRAME_THICK,0,0,0,0, 20,20);
  }

long GMImportDialog::onCmdAccept(FXObject*sender,FXSelector sel,void*ptr){
  return FXDialogBox::onCmdAccept(sender,sel,ptr);
  }

long GMImportDialog::onUpdAccept(FXObject*sender,FXSelector,void*){
  if (mode&IMPORT_SYNC) {
    if (GMPlayerManager::instance()->getPreferences().sync.import_new ||
        GMPlayerManager::instance()->getPreferences().sync.remove_missing ||
        GMPlayerManager::instance()->getPreferences().sync.update)
      sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    else
      sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);
    }
  else {
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
    }
  return 1;
  }

long GMImportDialog::onCmdSync(FXObject*,FXSelector sel,void*ptr){
  FXbool check=((FXint)(FXival)ptr)!=0;
  switch(FXSELID(sel)){
    case ID_SYNC_NEW            : GMPlayerManager::instance()->getPreferences().sync.import_new=check;
                                  break;
    case ID_SYNC_REMOVE_MISSING : GMPlayerManager::instance()->getPreferences().sync.remove_missing=check;
                                  break;
    case ID_SYNC_UPDATE         : GMPlayerManager::instance()->getPreferences().sync.update=check;
                                  break;
    case ID_SYNC_UPDATE_ALL     : GMPlayerManager::instance()->getPreferences().sync.update_always=check;
                                  break;
    case ID_SYNC_UPDATE_MODIFIED: GMPlayerManager::instance()->getPreferences().sync.update_always=!check;
                                  break;
    }
  return 1;
  }


long GMImportDialog::onUpdSync(FXObject*sender,FXSelector sel,void*){
  FXbool enabled=true;
  FXbool check=false;
  switch(FXSELID(sel)){
    case ID_SYNC_NEW            : check=GMPlayerManager::instance()->getPreferences().sync.import_new;
                                  break;
    case ID_SYNC_REMOVE_MISSING : check=GMPlayerManager::instance()->getPreferences().sync.remove_missing;
                                  break;
    case ID_SYNC_UPDATE         : check=GMPlayerManager::instance()->getPreferences().sync.update;
                                  break;
    case ID_SYNC_UPDATE_ALL     : check=GMPlayerManager::instance()->getPreferences().sync.update_always;
                                  enabled=(GMPlayerManager::instance()->getPreferences().sync.update);
                                  break;
    case ID_SYNC_UPDATE_MODIFIED: check=!GMPlayerManager::instance()->getPreferences().sync.update_always;
                                  enabled=(GMPlayerManager::instance()->getPreferences().sync.update);
                                  break;
    }
  if (enabled)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_ENABLE),nullptr);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_DISABLE),nullptr);

  if (check)
    sender->handle(this,FXSEL(SEL_COMMAND,ID_CHECK),nullptr);
  else
    sender->handle(this,FXSEL(SEL_COMMAND,ID_UNCHECK),nullptr);
  return 1;
  }


void GMImportDialog::getSelectedFiles(FXStringList & files){
  if (dirselector) {
    getApp()->reg().writeStringEntry("directories","last-import-dirs",dirselector->getDirectory().text());
    files.append(dirselector->getDirectory());
    }
  else if (fileselector){
    getApp()->reg().writeStringEntry("directories","last-import-files",fileselector->getDirectory().text());
    fileselector->getSelectedFiles(files);
    }
  }

FXString GMImportDialog::getFilename() const{
  getApp()->reg().writeStringEntry("directories","last-import-files",fileselector->getDirectory().text());
  return fileselector->getFilename();
  }

long GMImportDialog::onCmdParseMethod(FXObject*,FXSelector,void*){
  if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_TAG) {
    template_grpbox->hide();
    }
  else {
    template_grpbox->show();
    }

  if (GMPlayerManager::instance()->getPreferences().import.parse_method==GMImportOptions::PARSE_FILENAME) {
    id3v1_listbox->disable();
    }
  else{
    id3v1_listbox->enable();
    }

  template_grpbox->getParent()->recalc();
  template_grpbox->getParent()->layout();
  return 1;
  }



void GMImportDialog::getDefaultSearchDirectory(FXString & directory){
  const FXchar * musicdir[]={
    "Music",
    "music",
    "mp3",
    "MP3",
    "Mp3",
    "ogg",
    "OGG",
    "Ogg",
    "mp4",
    "MP4",
    "Mp4",
    nullptr
    };
  FXString test;
  FXString home=FXSystem::getHomeDirectory();
  for (int i=0;musicdir[i]!=nullptr;i++){
    test = home + PATHSEPSTRING + musicdir[i];
    if (FXStat::exists(test) && FXStat::isDirectory(test)){
      directory=test;
      break;
      }
    }
  if (directory.empty()) directory=home;
  }



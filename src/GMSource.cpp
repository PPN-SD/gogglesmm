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
#include "gmdefs.h"
#include "GMTrack.h"
#include "GMList.h"
#include "GMDatabase.h"
#include "GMTrackDatabase.h"
#include "GMTrackList.h"
#include "GMTrackView.h"
#include "GMSource.h"
#include "GMPlayerManager.h"
#include "GMWindow.h"
#include "GMIconTheme.h"
#include "GMClipboard.h"


FXIMPLEMENT(GMSource,FXObject,nullptr,0);

GMSource::GMSource() {
  }

GMSource::~GMSource() {
  }

FXbool GMSource::findCurrent(GMTrackList * list,GMSource * src) {
  if (src==nullptr || src->current_track==-1) return false;
  for (FXint i=0;i<list->getNumItems();i++){
    if (list->getItemId(i)==src->current_track) {
      list->setActiveItem(i);
      list->setCurrentItem(i);
      return true;
      }
    }
  return false;
  }

FXbool GMSource::findCurrentArtist(GMList *,GMSource *){
  return false;
  }

FXbool GMSource::findCurrentAlbum(GMAlbumList *,GMSource *){
  return false;
  }

void GMSource::dragged(GMTrackList * tracklist) {
  tracklist->markUnsorted();
  }

void GMSource::markCurrent(const GMTrackItem*item) {
  if (item)
    current_track = item->getId();
  else
    current_track = -1;
  }

FXint GMSource::getNumTracks() const{
  return 0;
  }

FXbool GMSource::getTrack(GMTrack &) const{
  return false;
  }

FXIcon* GMSource::getAlbumIcon() const {
  return GMIconTheme::instance()->icon_album;
  }



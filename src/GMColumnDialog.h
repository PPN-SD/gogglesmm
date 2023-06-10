/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2007-2021 by Sander Jansen. All Rights Reserved      *
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
#ifndef GMCOLUMNDIALOG_H
#define GMCOLUMNDIALOG_H

class GMColumnDialog : public FXDialogBox {
FXDECLARE(GMColumnDialog)
protected:
  GMList * list = nullptr;
protected:
  GMColumnDialog() {}
private:
  GMColumnDialog(const GMColumnDialog&);
  GMColumnDialog& operator=(const GMColumnDialog&);
public:
  enum {
    ID_MOVE_UP = FXDialogBox::ID_LAST,
    ID_MOVE_DOWN,
    ID_LIST,
    };
public:
  long onCmdMoveUp(FXObject*,FXSelector,void*);
  long onUpdMoveUp(FXObject*,FXSelector,void*);
  long onCmdMoveDown(FXObject*,FXSelector,void*);
  long onUpdMoveDown(FXObject*,FXSelector,void*);
  long onListLeftBtnPress(FXObject*,FXSelector,void*);
public:
  /// Construct button with text and icon
  GMColumnDialog(FXWindow* p,GMColumnList & cols);

  virtual FXuint execute(FXuint placement=PLACEMENT_CURSOR);

  void saveIndex();
  };

#endif

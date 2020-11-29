/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2021 by Sander Jansen. All Rights Reserved      *
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
#ifndef INPUT_H
#define INPUT_H

#include "ap_thread.h"
#include "ap_packet.h"
#include "ap_input_plugin.h"
#include "ap_reader_plugin.h"

namespace ap {

class AudioEngine;
class ReaderPlugin;
class InputPlugin;
class ConfigureEvent;
class MetaInfo;

class InputThread : public EngineThread, public IOContext, public InputContext {
protected:
  PacketPool     packetpool;
  InputPlugin  * input;
  ReaderPlugin * reader;
  FXuchar        state;

protected:
  enum {
    StateIdle       = 0, // doing nothing, waiting for events
    StateProcessing = 1, // processing events and reading input
    StateError      = 2
    };
  void set_state(FXuchar s,FXbool notify=false);

protected:
  Event * wait_for_packet();

protected:
  InputPlugin  * open_input(const FXString & uri);
  ReaderPlugin * open_reader();

protected:
  void ctrl_open_inputs(const FXStringList & url);

  void ctrl_open_input(const FXString & url);

  void ctrl_close_input(FXbool notify=false);

  void ctrl_seek_flush(FXlong offset);

  void ctrl_flush(FXbool close=false);

  void ctrl_seek(FXdouble pos);

  void ctrl_eos();

public:
  void post_configuration(ConfigureEvent*) override;

  void post_meta(MetaInfo*) override;

  void post_packet(Packet*) override;

  const Signal & signal() override { return fifo.signal();}

  FXbool aborted() override;

public:
  /// Constructor
  InputThread(AudioEngine*);

  FXint run() override;

  FXbool init() override;

  void free() override;

  /// Destructor
  virtual ~InputThread();
  };
}
#endif

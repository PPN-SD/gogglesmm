/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2018 by Sander Jansen. All Rights Reserved      *
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
#ifndef AP_EVENT_PRIVATE_H
#define AP_EVENT_PRIVATE_H

#include "ap_event.h"
#include "ap_device.h"
#include "ap_format.h"

namespace ap {

enum EventTypePrivate {

  /// These events abort reading
  Ctrl_Quit       = 0x81,
  Ctrl_Close      = 0x82,
  Ctrl_Open       = 0x83,
  Ctrl_Open_Flush = 0x84,

  Ctrl_Seek       = AP_LAST,
  Ctrl_Pause,
  Ctrl_Set_Output_Config,
  Ctrl_Get_Output_Config,
  Ctrl_Set_Replay_Gain,
  Ctrl_Get_Replay_Gain,
  Ctrl_Set_Cross_Fade,
  Ctrl_Get_Cross_Fade,
  Ctrl_Volume,

  Buffer,
  Configure,
  Flush,
  End,
  Input_Read,
  Meta = AP_META_INFO,
  };


class CtrlSeekEvent : public Event {
public:
  FXdouble pos;
protected:
  virtual ~CtrlSeekEvent();
public:
  CtrlSeekEvent(FXdouble);
  };

class CtrlVolumeEvent : public Event {
public:
  FXfloat vol;
protected:
  virtual ~CtrlVolumeEvent();
public:
  CtrlVolumeEvent(FXfloat);
  };

class FlushEvent : public Event {
public:
  FXlong offset;
  FXbool close;
public:
  FlushEvent(FXbool c=false);
  FlushEvent(FXlong offset);
  };

class ControlEvent : public Event {
public:
  FXString text;
protected:
  virtual ~ControlEvent();
public:
  ControlEvent(FXuchar type,const FXString & t=FXString::null);
  ControlEvent(FXuchar type,FXuint id);
  };


class SetCrossFade : public Event {
public:
  FXuint duration;  // duration in ms. 0 means off
protected:
  virtual ~SetCrossFade() {}
public:
  SetCrossFade(FXuint ms) : Event(Ctrl_Set_Cross_Fade), duration(ms) {}

  FXbool enabled() const { return duration > 0; }
  };

class SetReplayGain : public Event {
public:
  ReplayGainMode mode;
protected:
  virtual ~SetReplayGain() {}
public:
  SetReplayGain(ReplayGainMode m) : Event(Ctrl_Set_Replay_Gain), mode(m) {}
  };

class SetOutputConfig : public Event {
public:
  OutputConfig config;
protected:
  virtual ~SetOutputConfig() {}
public:
  SetOutputConfig(const OutputConfig & cfg) : Event(Ctrl_Set_Output_Config), config(cfg) {}
  };


/*
  SyncEvent events should be created on the stack.
  When unref'd they signal the condition of the waiting thread.
*/
class SyncEvent : public Event {
public:
  FXMutex     mutex;
  FXCondition condition;
public:

  //// Constructor
  SyncEvent(FXuchar t) : Event(t) {
    mutex.lock();
    }

  /// Destructor
  ~SyncEvent() {
    mutex.unlock();
    }

  /// Wait for unref
  FXbool waitForUnref() {
    return condition.wait(mutex);
    }

  /// Notify waiting thread we're done.
  void unref() {
    FXScopedMutex lock(mutex);
    condition.signal();
    }
  };


class GetOutputConfig : public SyncEvent {
public:
  OutputConfig config;
public:
  GetOutputConfig() : SyncEvent(Ctrl_Get_Output_Config) {}
  virtual ~GetOutputConfig() {}
  };


class GetReplayGain : public SyncEvent {
public:
  ReplayGainMode mode;
public:
  GetReplayGain() : SyncEvent(Ctrl_Get_Replay_Gain), mode(ReplayGainOff) {}
  virtual ~GetReplayGain() {}
  };


class GetCrossFade : public SyncEvent {
public:
  FXuint duration = 0;  // duration in milliseconds. 0 means off.
public:
  GetCrossFade() : SyncEvent(Ctrl_Get_Cross_Fade) {}

  FXbool enabled() const { return duration > 0; }

  virtual ~GetCrossFade() {}
  };


/*
class StreamInfo {
  FXlong  length;       /// Length in samples
  FXlong  position;     /// Position in samples
  FXshort padstart;     /// Start offset in samples
  FXshort padend;       /// End offset in samples
  };
*/

/* Decoder Specific Configuration */
class DecoderConfig {
public:
  virtual ~DecoderConfig() {}
  };


class DecoderSpecificConfig : public DecoderConfig {
public:
  FXuchar * config       = nullptr;
  FXuint    config_bytes = 0;
public:
  ~DecoderSpecificConfig() {
    freeElms(config);
    }
  };





class ConfigureEvent : public Event {
public:
  AudioFormat    af;
  DecoderConfig* dc                  = nullptr;
  FXuchar        codec               = Codec::Invalid;
  FXint          stream_length       = -1;
  void*          data                = nullptr;
  FXshort        stream_offset_start = 0;
  FXshort        stream_offset_end   = 0;
  ReplayGain     replaygain;
protected:
  virtual ~ConfigureEvent();
public:
  ConfigureEvent(const AudioFormat&,FXuchar codec=Codec::Invalid,FXint f=-1);
  };

}
#endif

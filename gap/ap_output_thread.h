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
#ifndef OUTPUT_H
#define OUTPUT_H

#include "ap_thread.h"
#include "ap_event_private.h"
#include "ap_buffer.h"
#include "ap_output_plugin.h"


namespace ap {

class AudioEngine;
class Packet;
class FrameTimer;
class CrossFader;

struct ReplayGainConfig {
  ReplayGainMode  mode;
  ReplayGain      value;

  ReplayGainConfig() : mode(ReplayGainOff) {}

  FXdouble gain() const { return (mode==ReplayGainAlbum) ? value.album      : value.track; }
  FXdouble peak() const { return (mode==ReplayGainAlbum) ? value.album_peak : value.track_peak; }
  };

class OutputThread : public EngineThread, public OutputContext {
protected:
  OutputConfig   output_config;
protected:
  Reactor           reactor;
  Reactor::Input*   fifoinput;
protected:
  /// Wait while pausing
  Event * wait_pause();

  /// Wait while draining
  Event * wait_drain();

  /// Wait normal events
  Event * wait_event();

  /// Get Next Event
  Event * get_next_event();
public:
  AudioFormat       af;
  OutputPlugin *    plugin;
  FXDLL             dll;
  MemoryBuffer      converted_samples;
  MemoryBuffer      src_input;
  MemoryBuffer      src_output;
  ReplayGainConfig  replaygain;
  Packet * packet_queue;
  CrossFader * crossfader;  
protected:
  FXbool draining;
  FXbool pausing;
protected:
  FXint     stream;
  FXlong    stream_length;
  FXint     stream_remaining;
  FXint     stream_written;
  FXlong    stream_position;
  FXint     timestamp;
protected:
  FXPtrListOf<FrameTimer> timers;
  void update_timers(FXint delay,FXint nframes);
  void clear_timers();
protected:
  void configure(const AudioFormat&);
  void load_plugin();
  void unload_plugin();
  void close_plugin();
  void process(Packet*);


#ifdef HAVE_SAMPLERATE
  void resample(Packet*,FXint & nframes);
#endif
  void drain(FXbool flush=true);

  void update_position(FXint stream,FXlong position,FXint nframes,FXlong length);
  void notify_position();
  void reset_position();

  void reconfigure();
public:
  OutputThread(AudioEngine*);

  FXbool init() override;

  FXint run() override;

  void notify_disable_volume() override;

  void notify_volume(FXfloat value) override;

  void wait_plugin_events() override;

  Reactor & getReactor() override { return reactor; }

  virtual ~OutputThread();
  };

}
#endif


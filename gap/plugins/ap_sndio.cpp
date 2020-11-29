/*******************************************************************************
*                         Goggles Audio Player Library                         *
********************************************************************************
*           Copyright (C) 2010-2020 by Sander Jansen. All Rights Reserved      *
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
#include "ap_defs.h"
#include "ap_output_plugin.h"

#include <sndio.h>

using namespace ap;

namespace ap {

class SndioOutput : public OutputPlugin {
protected:
  SndioConfig config;
  sio_hdl * handle = nullptr;
  FXint sio_delay = 0;
  FXuint sio_volume = 0;
  FXbool sio_started = false;
protected:
  FXbool open();
  void start();
  void stop();
protected:
  static void on_move(void * ptr, int delta) {
    auto output = reinterpret_cast<SndioOutput*>(ptr);
    output->sio_delay -= delta;
    }

  static void on_volume(void * ptr, unsigned int volume) {
    auto output = reinterpret_cast<SndioOutput*>(ptr);
    if (volume != output->sio_volume) {
        GM_DEBUG_PRINT("[sndio] volume: %d\n", volume);
        output->sio_volume = volume;
        output->context->notify_volume((float)volume / (float)SIO_MAXVOL);
        }
    }
public:
  SndioOutput(OutputContext* ctx);

  /// Configure
  FXbool configure(const AudioFormat &);

  /// Write frames to playback buffer
  FXbool write(const void*, FXuint);

  /// Return delay in no. of frames
  FXint delay();

  /// Empty Playback Buffer Immediately
  void drop();

  /// Wait until playback buffer is emtpy.
  void drain();

  /// Pause
  void pause(FXbool);

  /// Change Volume
  void volume(FXfloat);

  /// Close Output
  void close();

  /// Get Device Type
  FXchar type() const { return DeviceSndio; }

  /// Set Device Configuration
  FXbool setOutputConfig(const OutputConfig &);

  /// Destructor
  virtual ~SndioOutput();
  };


SndioOutput::SndioOutput(OutputContext * ctx) : OutputPlugin(ctx) {
  }


SndioOutput::~SndioOutput() {
  close();
  }


FXbool SndioOutput::setOutputConfig(const OutputConfig &c){
  config=c.sndio;
  return true;
  }


FXbool SndioOutput::open() {
  if (handle == nullptr) {
    handle = sio_open(config.device.text(), SIO_PLAY, 0);
    if (handle == nullptr) {
      GM_DEBUG_PRINT("[sndio] Unable to open device %s.\n", config.device.text());
      return false;
      }
    sio_onmove(handle, SndioOutput::on_move, this);
    sio_onvol(handle, SndioOutput::on_volume, this);
    sio_delay = 0;
    }
  return true;
  }


void SndioOutput::close() {
  if (handle != nullptr) {
     drop();
     sio_close(handle);
     handle = nullptr;
     af.reset();
     }
  }


void SndioOutput::start() {
  if (handle != nullptr && sio_started == false) {
    if (sio_start(handle) == 0) {
      GM_DEBUG_PRINT("[sndio] unable to start output\n");
      }
    else {
      sio_started = true;
      GM_DEBUG_PRINT("[sndio] switch to START\n");
      }
    }
  }


void SndioOutput::stop() {
  if (handle != nullptr && sio_started == true) {
    if (sio_stop(handle) == 0) {
      GM_DEBUG_PRINT("[sndio] unable to stop output\n");
      }
    else {
      sio_started = false;
      sio_delay = 0;
      GM_DEBUG_PRINT("[sndio] switch to STOP\n");
      }
    }
  }


void SndioOutput::volume(FXfloat v) {
  if (handle != nullptr) {
    sio_volume = (unsigned int)(v * SIO_MAXVOL);
    GM_DEBUG_PRINT("[sndio] volume: %d\n", sio_volume);
    sio_setvol(handle, sio_volume);
    }
  }


FXint SndioOutput::delay() {
  return sio_delay;
  }


void SndioOutput::drop() {
  GM_DEBUG_PRINT("[sndio] drop\n");
  stop();
  }


void SndioOutput::drain() {
  GM_DEBUG_PRINT("[sndio] drain\n");
  stop();
  }


void SndioOutput::pause(FXbool pausing) {
  if (pausing)
    stop();
  else
    start();
  }


FXbool SndioOutput::configure(const AudioFormat & fmt){
  sio_par parameters;
  sio_initpar(&parameters);

  if (handle && fmt==af)
    return true;

  if (handle) {
    drop();
    }

  if (!open())
    return false;

  parameters.bits = fmt.bps();    // bits per sample
  parameters.bps = fmt.packing(); // bytes per sample
  parameters.sig = (fmt.datatype() == Format::Signed || fmt.datatype() == Format::Float); // Signed or Unsigned format
  parameters.le = fmt.byteorder() == Format::Little; // Byteorder
  parameters.pchan = fmt.channels; // Channel Count
  parameters.rate = fmt.rate; // Sample Rate
  parameters.xrun = SIO_IGNORE; // xrun behaviour

  if (sio_setpar(handle, &parameters) == 0)
    goto failed;

  if (sio_getpar(handle, &parameters) == 0)
    goto failed;

  af.set(parameters.sig ? Format::Signed : Format::Unsigned,
         parameters.bits,
         parameters.bps,
         parameters.rate,
         parameters.pchan);

  return true;
failed:
  GM_DEBUG_PRINT("[sndio] Unsupported configuration:\n");
  af.debug();
  return false;
  }


FXbool SndioOutput::write(const void * buffer,FXuint nframes){
  FXival nwritten;
  FXival nbytes = nframes*af.framesize();
  const FXchar * buf = (const FXchar*)buffer;

  if (__unlikely(handle == nullptr)) {
    GM_DEBUG_PRINT("[sndio] device not opened\n");
    return false;
    }

  if (__unlikely(sio_started == false)) {
    start();
    }

  while(nbytes>0) {
    nwritten = sio_write(handle,buf,nbytes);
    buf+=nwritten;
    nbytes-=nwritten;
    }
  sio_delay += nframes;
  return true;
  }

}


AP_IMPLEMENT_PLUGIN(SndioOutput);

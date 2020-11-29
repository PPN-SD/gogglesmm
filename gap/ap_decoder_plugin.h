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
#ifndef DECODER_PLUGIN_H
#define DECODER_PLUGIN_H

#include "ap_format.h"

namespace ap {

class ConfigureEvent;
class Packet;

/* Interface to Decoder */
class DecoderContext {
public:
  // Get input packet
  virtual Packet * get_input_packet()=0;

  // Get output packet
  virtual Packet * get_output_packet()=0;

  // Post output packet
  virtual void post_output_packet(Packet*&,FXbool eos=false)=0;

  // Post output configuration
  virtual void post_configuration(ConfigureEvent*)=0;
  };


class DecoderPlugin {
protected:
  DecoderContext * context;
  AudioFormat   af;
  FXlong        stream_decode_offset;
public:
public:
  DecoderPlugin(DecoderContext*);

  virtual FXuchar codec() const { return Codec::Invalid; }

  virtual FXbool init(ConfigureEvent*);

  virtual FXbool process(Packet*)=0;

  virtual FXbool flush(FXlong offset=0);

  static DecoderPlugin* open(DecoderContext * ctx,FXuchar codec);

  virtual ~DecoderPlugin() {}
  };

}
#endif


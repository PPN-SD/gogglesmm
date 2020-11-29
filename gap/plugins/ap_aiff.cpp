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
#include "ap_defs.h"
#include "ap_event_private.h"
#include "ap_packet.h"
#include "ap_input_plugin.h"
#include "ap_reader_plugin.h"


namespace ap {


#define DEFINE_CHUNK(b1,b2,b3,b4) ((b4<<24) | (b3<<16) | (b2<<8) | (b1))


class AIFFReader : public ReaderPlugin {
protected:
  FXlong input_start;
protected:
  ReadStatus parse();
public:
  enum Chunk {
    FORM = DEFINE_CHUNK('F','O','R','M'),
    AIFF = DEFINE_CHUNK('A','I','F','F'),
    COMM = DEFINE_CHUNK('C','O','M','M'),
    SSND = DEFINE_CHUNK('S','S','N','D'),
    };

public:
  AIFFReader(InputContext*);
  FXbool init(InputPlugin*) override;
  ReadStatus process(Packet*) override;

  FXuchar format() const override { return Format::AIFF; };

  FXbool can_seek() const override;
  FXbool seek(FXlong) override;
  virtual ~AIFFReader();
  };


AIFFReader::AIFFReader(InputContext * ctx) : ReaderPlugin(ctx), input_start(0) {
  }

AIFFReader::~AIFFReader(){
  }

FXbool AIFFReader::init(InputPlugin*plugin) {
  ReaderPlugin::init(plugin);
  input_start=0;
  return true;
  }

FXbool AIFFReader::can_seek() const {
  return true;
  }


FXbool AIFFReader::seek(FXlong pos){
  FXlong offset=input_start + (FXCLAMP(0,pos,stream_length) * af.framesize());
  input->position(offset,FXIO::Begin);
  return true;
  }

ReadStatus AIFFReader::process(Packet*packet) {

  if (!(flags&FLAG_PARSED)) {
    if (parse()!=ReadOk)
      return ReadError;
    }

  FXint nbytes = (packet->space() / af.framesize()) * af.framesize();
  FXint nread = input->read(packet->ptr(),nbytes);
  if (nread<0) {
    packet->unref();
    return ReadError;
    }
  else if (nread==0){
    packet->unref();
    return ReadDone;
    }

  packet->af              = af;
  packet->wroteBytes(nread);
  packet->stream_position = static_cast<FXint>( (input->position()-input_start-nread) / af.framesize() );
  packet->stream_length   = stream_length;
  if (input->eof())
    packet->flags=FLAG_EOS;
  else
    packet->flags=0;

  context->post_packet(packet);

  return ReadOk;
  }




/* Kindly borrowed from FLAC
 *
 * flac - Command-line FLAC encoder/decoder
 * Copyright (C) 2000-2009  Josh Coalson
 * Copyright (C) 2011-2013  Xiph.Org Foundation
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
*/
static FXbool parse_extended(FXuint & val,const FXuchar buf[10])
	/* Read an IEEE 754 80-bit (aka SANE) extended floating point value from 'f',
	 * convert it into an integral value and store in 'val'.  Return false if only
	 * between 1 and 9 bytes remain in 'f', if 0 bytes remain in 'f', or if the
	 * value is negative, between zero and one, or too large to be represented by
	 * 'val'; return true otherwise.
	 */
{
	unsigned int i;
	FXulong p = 0;
	FXshort e;
	FXshort shift;
  val = 0;

	e = ((FXushort)(buf[0])<<8 | (FXushort)(buf[1]))-0x3FFF;
	shift = 63-e;
	if((buf[0]>>7)==1U || e<0 || e>63) {
		return false;
	}

	for(i = 0; i < 8; ++i)
		p |= (FXulong)(buf[i+2])<<(56U-i*8);

  val = (FXuint)((p>>shift)+(p>>(shift-1) & 0x1));
	return true;
}



ReadStatus AIFFReader::parse() {
  FXuint chunkid;
  FXuint chunksize;
  FXuint formsize;

  FXshort channels;
  FXuint  nsamples;
  FXshort samplesize;
  FXuchar extended[10];
  FXuint  rate;

  FXbool got_comm = false;
  FXbool got_ssnd = false;


  GM_DEBUG_PRINT("[aiff] parsing aiff header\n");

  // formchunk
  if (input->read(&chunkid,4)!=4 || chunkid!=FORM){
    GM_DEBUG_PRINT("[aiff] no FORM tag found\n");
    return ReadError;
    }

  // formsize
  if (!input->read_uint32_be(formsize)){
    return ReadError;
    }

  // formchunk
  if (input->read(&chunkid,4)!=4 || chunkid!=AIFF){
    GM_DEBUG_PRINT("[aiff] no AIFF tag found\n");
    return ReadError;
    }

  while(formsize>0) {

    // get the chunk id
    if (input->read(&chunkid,4)!=4){
      return ReadError;
      }

    // chunk size
    if (!input->read_uint32_be(chunksize)){
      return ReadError;
      }

    // COMM chunk
    if (chunkid==COMM) {
      if (got_comm) return ReadError;

      if (!input->read_int16_be(channels))
        return ReadError;

      if (!input->read_uint32_be(nsamples))
        return ReadError;

      if (!input->read_int16_be(samplesize))
        return ReadError;

      if (input->read(&extended,10)!=10 || !parse_extended(rate,extended))
        return ReadError;

      if (nsamples==0) return ReadError;

      af.set(Format::Signed|Format::Big,samplesize,samplesize>>3,rate,channels);
      got_comm=true;
      if (got_ssnd) break;
      }

    /// SSND chunk
    else if (chunkid==SSND) {

      if (got_ssnd || (input->serial() && got_comm==false)) {
        GM_DEBUG_PRINT("[aiff] multiple ssnd or ssnd before comm in serial stream\n");
        return ReadError;
        }

      got_ssnd = true;

      FXuint offset;
      FXuint blocksize;

      if (!input->read_uint32_be(offset))
        return ReadError;

      if (!input->read_uint32_be(blocksize))
        return ReadError;

      if (blocksize!=0) {
        GM_DEBUG_PRINT("[aiff] blocksize %u not supported\n",blocksize);
        return ReadError;
        }

      input_start = input->position() + offset;

      // Stop scanning
      if (got_comm==true) break;
      }
    else {
      input->position(chunksize,FXIO::Current);
      }
    formsize-=chunksize;
    }

  if (got_ssnd && got_comm) {
    stream_length = nsamples;
    flags|=FLAG_PARSED;
    GM_DEBUG_STREAM_LENGTH("aiff",stream_length,af.rate);
    context->post_configuration(new ConfigureEvent(af,Codec::PCM));
    return ReadOk;
    }
  return ReadError;
  }


ReaderPlugin * ap_aiff_reader(InputContext * ctx) {
  return new AIFFReader(ctx);
  }
}

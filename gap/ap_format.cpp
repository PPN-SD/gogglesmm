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
#include "ap_format.h"
#include "ap_common.h"


namespace ap {

static const FXchar * const channelnames[]={
  "NA","MONO","FL","FR","FC","BL","BR","BC","SL","SR","LFE"
  };

static const FXchar * const codecs[]={
  "Invalid",
  "PCM",
  "FLAC",
  "Vorbis",
  "MPEG",
  "AAC",
  "Opus",
  "ALAC",
  "DCA",
  "A52"
  };

static const FXchar * const byteorders[]={
  "le",
  "be"
  };

static const FXchar * const formats[]={
  "s",
  "u",
  "f",
  "iec958_frame"
  "Reserved1"
  "Reserved2"
  "Reserved3"
  "Reserved4"
  };


static const FXchar * const formatnames[]={
  "unknown",
  "wav",
  "ogg",
  "flac",
  "mp3",
  "mp4",
  "aac",
  "m3u",
  "pls",
  "xspf",
  "aiff",
  "matroska"
  };


const FXchar * Codec::name(FXuchar c){
  return codecs[c];
  }




#if defined(HAVE_OPUS) || defined(HAVE_VORBIS) || defined(HAVE_TREMOR)

// http://www.xiph.org/vorbis/doc/Vorbis_I_spec.html
extern const FXuint vorbis_channel_map[]={
  AP_CHANNELMAP_MONO,

  AP_CHANNELMAP_STEREO,

  AP_CMAP3(Channel::FrontLeft,
           Channel::FrontCenter,
           Channel::FrontRight),

  AP_CMAP4(Channel::FrontLeft,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP5(Channel::FrontLeft,
           Channel::FrontCenter,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight),

  AP_CMAP6(Channel::FrontLeft,
           Channel::FrontCenter,
           Channel::FrontRight,
           Channel::BackLeft,
           Channel::BackRight,
           Channel::LFE),

  AP_CMAP7(Channel::FrontLeft,
           Channel::FrontCenter,
           Channel::FrontRight,
           Channel::SideLeft,
           Channel::SideRight,
           Channel::BackCenter,
           Channel::LFE),

  AP_CMAP8(Channel::FrontLeft,
           Channel::FrontCenter,
           Channel::FrontRight,
           Channel::SideLeft,
           Channel::SideRight,
           Channel::BackLeft,
           Channel::BackRight,
           Channel::LFE)
  };

#endif


void AudioFormat::reset() {
  format=0;
  rate=0;
  channels=0;
  channelmap=0;
  }

void AudioFormat::setBits(FXushort bits) {
  if (bits>0 && bits<=32) {
    format|=(bits-1)<<Format::Bits_Shift|(((bits/8)-1)<<Format::Pack_Shift);
    }
  }

void AudioFormat::setChannels(FXuchar ch) {
  if (ch>0 && ch<=8) {
    channels=ch;
    switch(channels) {
      case  1: channelmap = AP_CHANNELMAP_MONO;   break;
      case  2: channelmap = AP_CHANNELMAP_STEREO; break;
      default: channelmap = 0;                    break;
      }
    }
  }


void AudioFormat::set(FXushort dt,FXushort bits,FXushort pack,FXuint r,FXuchar nc,FXuint map) {
  format=dt|((bits-1)<<Format::Bits_Shift)|((pack-1)<<Format::Pack_Shift);
  rate=r;
  channels=nc;
  channelmap=map;
  if (channelmap==0) {
    switch(channels) {
      case  1: channelmap = AP_CHANNELMAP_MONO;   break;
      case  2: channelmap = AP_CHANNELMAP_STEREO; break;
      default: channelmap = 0;                    break;
      };
    }
  }

void AudioFormat::set(FXushort fmt,FXuint r,FXuchar nc,FXuint map) {
  format=fmt;
  rate=r;
  channels=nc;
  channelmap=map;
  if (channelmap==0) {
    switch(channels) {
      case  1: channelmap = AP_CHANNELMAP_MONO;   break;
      case  2: channelmap = AP_CHANNELMAP_STEREO; break;
      default: channelmap = 0;                    break;
      };
    }
  }


FXbool AudioFormat::swap() {
  if (packing()>Format::Pack_1) {
    format^=(1<<Format::Order_Shift);
    return true;
    }
  else {
    return false;
    }
  }


/*
  24 -> 32 -> 16
  flt -> 32 -> 16
*/

FXbool AudioFormat::compatible()  {
  switch(format){
    case AP_FORMAT_S24_3BE  : format=AP_FORMAT_S24_BE; break;
    case AP_FORMAT_S24_3LE  : format=AP_FORMAT_S24_LE; break;
    case AP_FORMAT_S24_LE   : format=AP_FORMAT_S32_LE; break;
    case AP_FORMAT_S24_BE   : format=AP_FORMAT_S32_BE; break;
    case AP_FORMAT_S32_LE   : format=AP_FORMAT_S16_LE; break;
    case AP_FORMAT_S32_BE   : format=AP_FORMAT_S16_BE; break;
    case AP_FORMAT_FLOAT_LE : format=AP_FORMAT_S32_LE; break;
    case AP_FORMAT_FLOAT_BE : format=AP_FORMAT_S32_BE; break;
    default                 : return false;            break;
    }
  return true;
  }


FXString AudioFormat::debug_format() const {
  return FXString::value("%s%2d%s%d",formats[datatype()],bps(),byteorders[byteorder()],packing());
  }

void AudioFormat::debug() const {
  fxmessage("format: %6dHz, %dch, %s%2d%s%d",rate,channels,formats[datatype()],bps(),byteorders[byteorder()],packing());
  if (channels) {
    fxmessage(", (%s",channelnames[channeltype(0)]);
    for (FXuint i=1;i<channels;i++) {
      fxmessage(",%s",channelnames[channeltype(i)]);
      }
    fxmessage(")\n");
    }
  else {
    fxmessage("\n");
    }
  }


FXbool operator!=(const AudioFormat& af1,const AudioFormat& af2){
  if ( (af1.format!=af2.format) ||
       (af1.rate!=af2.rate) ||
       (af1.channels!=af2.channels) ||
       (af1.channelmap!=af2.channelmap) )
    return true;
  else
    return false;
  }

FXbool operator==(const AudioFormat& af1,const AudioFormat& af2){
  if ( (af1.format!=af2.format) ||
       (af1.rate!=af2.rate) ||
       (af1.channels!=af2.channels) ||
       (af1.channelmap!=af2.channelmap) )
    return false;
  else
    return true;
  }


extern FXuint ap_format_from_mime(const FXString & mime) {
  if (FXString::comparecase(mime,"audio/mpeg")==0) {
    return Format::MP3;
    }
  else if (FXString::comparecase(mime,"audio/ogg")==0 ||
           FXString::comparecase(mime,"application/ogg")==0 ||
           FXString::comparecase(mime,"audio/opus")==0){
    return Format::OGG;
    }
  else if (FXString::comparecase(mime,"audio/aacp")==0 ||
           FXString::comparecase(mime,"audio/aac")==0) {
    return Format::AAC;
    }
  else if (FXString::comparecase(mime,"audio/x-mpegurl")==0 ||
           FXString::comparecase(mime,"audio/mpegurl")==0) {
    return Format::M3U;
    }
  else if ((FXString::comparecase(mime,"application/pls+xml")==0) || /// wrong mimetype, but NPR actually returns this: http://www.npr.org/streams/mp3/nprlive24.pls
           (FXString::comparecase(mime,"audio/x-scpls")==0)){
    return Format::PLS;
    }
  else if (FXString::comparecase(mime,"application/xspf+xml")==0){
    return Format::XSPF;
    }
  else if (FXString::comparecase(mime,"audio/x-aiff")==0 ||
           FXString::comparecase(mime,"audio/aiff")==0) {
    return Format::AIFF;
    }
  else if (FXString::comparecase(mime,"video/webm")==0){
    return Format::Matroska;
    }

  else {
    return Format::Unknown;
    }
  }

extern FXuint ap_format_from_extension(const FXString & extension) {
  if (FXString::comparecase(extension,"wav")==0)
    return Format::WAV;
  else if (FXString::comparecase(extension,"flac")==0)
    return Format::FLAC;
  else if (FXString::comparecase(extension,"ogg")==0 || FXString::comparecase(extension,"oga")==0 || FXString::comparecase(extension,"opus")==0)
    return Format::OGG;
  else if (FXString::comparecase(extension,"mp3")==0)
    return Format::MP3;
  else if (FXString::comparecase(extension,"mp4")==0 ||
           FXString::comparecase(extension,"m4a")==0 ||
           FXString::comparecase(extension,"m4p")==0 ||
           FXString::comparecase(extension,"mov")==0 ||
           FXString::comparecase(extension,"m4b")==0 )
    return Format::MP4;
  else if (FXString::comparecase(extension,"aac")==0)
    return Format::AAC;
  else if (FXString::comparecase(extension,"aiff")==0 || FXString::comparecase(extension,"aif")==0)
    return Format::AIFF;
  else if (FXString::comparecase(extension,"m3u")==0)
    return Format::M3U;
  else if (FXString::comparecase(extension,"pls")==0)
    return Format::PLS;
  else if (FXString::comparecase(extension,"xspf")==0)
    return Format::XSPF;
  else if (FXString::comparecase(extension,"mkv")==0 || FXString::comparecase(extension,"webm")==0)
    return Format::Matroska;
  else
    return Format::Unknown;
  }


extern FXuint ap_format_from_buffer(const FXchar * /*buffer*/,FXival /*size*/) {
  return Format::Unknown;
  }

extern const FXchar * ap_format_name(FXuint format){
  return formatnames[format];
  }


extern FXString ap_get_gogglesmm_all_supported_files() {
  const FXchar * const supported = "*.("
#if defined(HAVE_OPUS)
  "opus,"
#endif
#if defined(HAVE_FLAC)
  "flac,oga,"
#endif
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  "ogg,"
#endif
#if defined(HAVE_MAD)
  "mp3,"
#endif
#if defined(HAVE_MP4)
  "mp4,m4a,m4b,m4p,mov,"
#endif
#if defined(HAVE_FAAD)
  "aac,"
#endif
#if defined(HAVE_MATROSKA)
  "mkv,mka,webm,"
#endif
  "wav,aiff)"
  ;
  return supported;
  }


extern FXString ap_get_gogglesmm_supported_files() {
  FXString pattern;
  const FXchar * const supported = "*.("
#if defined(HAVE_OPUS)
  "opus,"
#endif
#if defined(HAVE_FLAC)
  "flac,oga,"
#endif
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  "ogg,"
#endif
#if defined(HAVE_MAD)
  "mp3,"
#endif
#if defined(HAVE_MP4)
  "mp4,m4a,m4b,m4p,mov,"
#endif
#if defined(HAVE_FAAD)
  "aac,"
#endif
  ;

  if (strlen(supported)==3)
    return FXString::null;

  pattern = supported;
  pattern.tail() = ')';
  return pattern;
  }


extern FXString ap_get_gogglesmm_filepatterns() {
  const FXchar * const patterns =

 "All Music ("
#if defined(HAVE_OPUS)
  "*.opus,"
#endif
#if defined(HAVE_FLAC)
  "*.flac,*.oga,"
#endif
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  "*.ogg,"
#endif
#if defined(HAVE_MAD)
  "*.mp3,"
#endif
#if defined(HAVE_MP4)
  "*.mp4,*.m4a,*.m4b,*.m4p,*.mov,"
#endif
#if defined(HAVE_FAAD)
  "*.aac,"
#endif
#if defined(HAVE_MATROSKA)
  "*.mka,*.webm"
#endif
  "*.wav,*.aiff)\n"

// All Media

  "All Media ("
#if defined(HAVE_OPUS)
  "*.opus,"
#endif
#if defined(HAVE_FLAC)
  "*.flac,*.oga,"
#endif
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  "*.ogg,"
#endif
#if defined(HAVE_MAD)
  "*.mp3,"
#endif
#if defined(HAVE_MP4)
  "*.mp4,*.m4a,*.m4b,*.m4p,*.mov,"
#endif
#if defined(HAVE_FAAD)
  "*.aac,"
#endif
#if defined(HAVE_MATROSKA)
  "*.mkv,*.mka,*.webm"
#endif
  "*.wav,*.aiff)\n"

#if defined(HAVE_OPUS)
  "Opus (*.opus)\n"
#endif
#if defined(HAVE_FLAC)
  "Free Lossless Audio Codec (*.flac,*.oga)\n"
#endif
#if defined(HAVE_VORBIS) || defined(HAVE_TREMOR)
  "Ogg Vorbis (*.ogg)\n"
#endif
#if defined(HAVE_MAD)
  "MPEG-1 Audio Layer 3 (*.mp3)\n"
#endif
#if defined(HAVE_MP4)
  "MPEG-4 Part 14 (*.mp4,*.m4a,*.m4p,*.m4b,*.mov)\n"
#endif
#if defined(HAVE_FAAD)
  "Advanced Audio Coding (*.aac)\n"
#endif
#if defined(HAVE_MATROSKA)
  "Matroska (*.mka,*.webm)"
#endif
  "Wav (*.wav,*.aiff)\n";

  return patterns;
  }







}

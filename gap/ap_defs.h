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
#ifndef AP_DEFS_H
#define AP_DEFS_H

/*
  When building the output plugins, we always need to export
  their interface api (GMPLUGINAPI) and still need the ability
  to import the api from the gap library (GMAPI) at the same time.
  Hence the need for two separate macros.

  Plugin Modules should define GAP_PLUGIN before any other includes
*/
#ifdef _WIN32
  #ifndef GAP_DLL
    #ifdef GAP_PLUGIN
      #define GMAPI __declspec(dllimport)
    #else
      #define GMAPI __declspec(dllexport)
    #endif
  #else
    #define GMAPI
  #endif
  #define GMPLUGINAPI __declspec(dllexport)
#else
  #if defined(__GNUC__) && __GNUC__ >= 4
    #define GMAPI __attribute__ ((visibility("default")))
    #define GMPLUGINAPI __attribute__ ((visibility("default")))
  #else
    #define GMAPI
    #define GMPLUGINAPI
  #endif
#endif

#define AP_VERSION(major,minor,release) ((release)+(minor*1000)+(major*100000))

#include "fox.h"

#if FOX_BIGENDIAN == 0
#define INT32_LE(x) (((x)[3]<<24) | ((x)[2]<<16) | ((x)[1]<<8) |  ((x)[0]))
#define INT32_BE(x) ((((FXuint)(x)[0]) << 24) | \
                     (((FXuint)(x)[1]) << 16) | \
                     (((FXuint)(x)[2]) << 8) | \
                     (((FXuint)(x)[3]) ))

#define INT16_BE(x) ((((FXshort)(x)[0]) << 8) | \
                     (((FXshort)(x)[1]) ))

#define INT24_BE(x) ((((FXuint)(x)[0]) << 16) | \
                     (((FXuint)(x)[1]) << 8) | \
                     (((FXuint)(x)[2]) ))
#else
#define INT32_LE(x) ((((FXuint)(x)[0]) << 24) | \
                     (((FXuint)(x)[1]) << 16) | \
                     (((FXuint)(x)[2]) << 8) | \
                     (((FXuint)(x)[3]) ))
#define INT32_BE(x) (((x)[3]<<24) | ((x)[2]<<16) | ((x)[1]<<8) |  ((x)[0]))
#define INT16_BE(x) (((x)[1]<<8) | ((x)[0]))
#endif


#include "ap_config.h"

#ifdef DEBUG
namespace ap {
extern void ap_debug_stream_length(const FXchar*,FXlong,FXuint);
}
#define GM_DEBUG_STREAM_LENGTH(prefix,length,rate) ap_debug_stream_length(prefix,length,rate)
#else
#define GM_DEBUG_STREAM_LENGTH(prefix,length,rate) ((void)0)
#endif



enum {
  FLAG_EOS = 0x1
  };




#endif

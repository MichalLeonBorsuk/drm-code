/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2006
 *
 * Author(s):  Julian Cable, Ollie Haffenden, Andrew Murphy
 *
 ******************************************************************************
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
\******************************************************************************/

#ifndef _PLATFORM_H
#define _PLATFORM_H

#ifndef WIN32
#include <unistd.h>
#endif

#if _POSIX_TIMERS>0
#else
struct timespec
{
  unsigned long tv_sec;
  unsigned long tv_nsec;
};
#endif

#ifdef _MSC_VER
typedef __int64 int64_t;
typedef __int32 int32_t;
typedef __int16 int16_t;
typedef __int8 int8_t;
typedef unsigned __int64 uint64_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int8 uint8_t;
#else
#include <inttypes.h>
#endif

#ifdef WIN32
# include <winsock2.h>
# include <ws2tcpip.h>
typedef unsigned long in_addr_t;
# define MSG_TRUNC 0
# define sleep(n) Sleep(1000*n);
#else
//#include <netinet/in.h>
//#include <arpa/inet.h>
typedef int SOCKET;
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
#endif

// from WSPiApi.h
#if defined(__GOT_SECURE_LIB__) && __GOT_SECURE_LIB__ >= 200402L

#define _WSPIAPI_STRCPY_S strcpy_s
#define _WSPIAPI_STRCAT_S strcat_s
#define _WSPIAPI_STRNCPY_S strncpy_s
#define _WSPIAPI_SPRINTF_S_1 sprintf_s

#else

#define _WSPIAPI_STRCPY_S(_Dst, _Size, _Src) strcpy((_Dst), (_Src))
#define _WSPIAPI_STRCAT_S(_Dst, _Size, _Src) strcat((_Dst), (_Src))
#define _WSPIAPI_STRNCPY_S(_Dst, _Size, _Src, _Count) strncpy((_Dst), (_Src), (_Count)); (_Dst)[(_Size) - 1] = 0
#define _WSPIAPI_SPRINTF_S_1(_Dst, _Size, _Format, _Arg1) sprintf((_Dst), (_Format), (_Arg1))

#endif // defined(__GOT_SECURE_LIB__) && __GOT_SECURE_LIB__ >= 200402L

#endif // _PLATFORM_H

/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	Global definitions
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

#if !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)
#define DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_

#if defined(_MSC_VER) && (_MSC_VER >= 1400)
# pragma warning( disable : 4996 4351 4521)
#endif

#include <complex>
using namespace std; /* Because of the library: "complex" */
#include <string>
#include <stdio.h>
#include <math.h>
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "tables/TableDRMGlobal.h"


/* Definitions ****************************************************************/
/* When you define the following flag, a directory called
   "test" MUST EXIST in the windows directory (or linux directory if you use
   Linux)! */
#define _DEBUG_
#undef _DEBUG_

/* Choose algorithms -------------------------------------------------------- */
/* There are two algorithms available for frequency offset estimation for
   tracking mode: Using frequency pilots or the guard-interval correlation. In
   case of guard-interval correlation (which will be chosen if this macro is
   defined), the Hilbert filter in TimeSync must be used all the time -> more
   CPU usage. Also, the frequency tracking range is smaller */
#undef USE_FRQOFFS_TRACK_GUARDCORR

/* The sample rate offset estimation can be done using the frequency pilots or
   the movement of the estimated impulse response. Defining this macro will
   enable the frequency pilot based estimation. Simulations showed that this
   method is more vulnerable to bad channel situations */
#undef USE_SAMOFFS_TRACK_FRE_PIL

/* Using max-log MAP decoder. A lot more memory and CPU is needed for this
   method. This is just for showing the potential of an improved decoding
   method and should not be activated for the "regular" version of Dream */
#undef USE_MAX_LOG_MAP

/* This method tries to speed up the audio output after a re-synchronization
   when long symbol interleaver is used. We work with erasure symbols to mark
   data which is not yet received. We hope that the channel decoder can still
   decode audio even if not all data is yet received to fill the interleaver
   history */
#define USE_ERASURE_FOR_FASTER_ACQ

#ifdef USE_ERASURE_FOR_FASTER_ACQ

// this is scary - the code uses an equality match on a floating point!!
// TODO
/* Use max-value for showing that this is an erasure */
# define ERASURE_TAG_VALUE				numeric_limits<_REAL>::max()
#endif

/* If the following macro is defined, the Wiener filter for channel estimation
   in time direction will be a Decision-Directed channel estimation ->
   additional to the actual pilot cells, hard decisions about the data cells
   are used as new pilots, too */
#undef USE_DD_WIENER_FILT_TIME

#if HAVE_STDINT_H
# include <stdint.h>
#elif HAVE_INTTYPES_H
# include <inttypes.h>
#elif defined(_WIN32)
# ifndef HAVE_INT8_T
#  define HAVE_INT8_T 1
   typedef signed char int8_t;
# endif
# ifndef HAVE_INT16_T
#  define HAVE_INT16_T 1
   typedef signed __int16 int16_t;
# endif
# ifndef HAVE_INT32_T
#  define HAVE_INT32_T 1
   typedef signed __int32 int32_t;
# endif
   typedef unsigned char uint8_t;
# ifndef HAVE_U_INT16_T
#  define HAVE_U_INT16_T 1
   typedef unsigned __int16 uint16_t;
# endif
# ifndef HAVE_U_INT32_T
#  define HAVE_U_INT32_T 1
   typedef unsigned __int32 uint32_t;
# endif
typedef signed __int64 int64_t;
typedef unsigned __int64 uint64_t;
#else
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef signed int int16_t;
typedef unsigned int uint16_t;
typedef signed long int32_t;
typedef unsigned long uint32_t;
typedef signed long long int64_t;
typedef unsigned long long uint64_t;
#endif

/* Define the application specific data-types ------------------------------- */
typedef	double					_REAL;
typedef	complex<_REAL>			_COMPLEX;
typedef int16_t					_SAMPLE;
typedef uint8_t					_BYTE;
typedef uint8_t       			_BINARY;
const int BITS_BINARY = 8;
const _REAL RET_VAL_LOG_0 = -200.0; /* avoid infinity in the case: log10(0) */
/* Classes ********************************************************************/

class CGenErr
{
public:
	CGenErr(string strNE) : strError(strNE) {}
	string strError;
};

class CDumpable
{
public:
    virtual void dump(ostream&) const = 0;
    CDumpable() {}
    virtual ~CDumpable() {}
};

/* Prototypes for global functions ********************************************/

/* Debug error handling */
void DebugError(const char* pchErDescr, const char* pchPar1Descr,
				const double dPar1, const char* pchPar2Descr,
				const double dPar2);

void ErrorMessage(string strErrorString);

#endif // !defined(DEF_H__3B0BA660_CA63_4344_BB2B_23E7A0D31912__INCLUDED_)

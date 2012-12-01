/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2006
 *
 * Author(s):
 *	Andrea Russo, Julian Cable
 *
 * Description:
 *	Dream program version number
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
#include "Version.h"
#include "GlobalDefinitions.h"

const char dream_manufacturer[] = "drea";
#ifdef QT_VERSION
# if QT_VERSION >= 0x040000
const char dream_implementation[] = "Q4";
# elif QT_VERSION >= 0x030000
const char dream_implementation[] = "Q3";
# else
const char dream_implementation[] = "Q2";
# endif
#else
const char dream_implementation[] = "CL";
#endif
const int dream_version_major = 1;
const int dream_version_minor = 17;
const char dream_version_build[] = "";


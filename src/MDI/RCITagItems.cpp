/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2007
 *
 * Author(s):
 *	Volker Fischer, Julian Cable, Oliver Haffenden
 *
 * Description:
 *	Implements Digital Radio Mondiale (DRM)
 *	(RCI), Receiver Status and Control Interface (RSCI)
 *  and Distribution and Communications Protocol (DCP) as described in
 *	ETSI TS 102 349 and ETSI TS 102 821 respectively.
 *
 *  This module derives, from the CTagItemGenerator base class, tag item generators
 *  specialised to generate each of the tag items defined in RCI and RSCI.
 *  .
 *  An intermediate derived class, CTagItemGeneratorWithProfiles, is used as the
 *  base class for all these tag item generators. This takes care of the common
 *	task of checking whether a given tag is in a particular profile.
 *  The profiles for each tag are defined by the GetProfiles() member function.
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

#include "RCITagItems.h"

void CTagItemGeneratorCfre::GenTag(int iNewFreqkHz)
{
	/* Length: 4 bytes = 32 bits */
	PrepareTag(32);

	/* RF Frequency */
	Enqueue(1000*iNewFreqkHz, 32);

}

string CTagItemGeneratorCfre::GetTagName(void) {return "cfre";}

void CTagItemGeneratorCdmo::GenTag(const EModulationType eMode) // cdmo
{
	PrepareTag(4 * BITS_BINARY);
	string s;
	switch (eMode)
	{
	case DRM: s="drm_"; break;
	case AM: s="am__"; break;
	case USB: s="usb_"; break;
	case LSB: s="lsb_"; break;
	case NBFM: s="nbfm"; break;
	case WBFM: s="wbfm"; break;
	default: s="    "; break;
	}

	for (int i=0; i<4; i++)
		Enqueue((uint32_t) s[i], BITS_BINARY);
}

string CTagItemGeneratorCdmo::GetTagName(void) {return "cdmo";}

void CTagItemGeneratorCser::GenTag(const int iServiceID) // cser
{
	PrepareTag(1*BITS_BINARY);
	Enqueue((uint8_t) iServiceID, BITS_BINARY);
}

string CTagItemGeneratorCser::GetTagName(void) {return "cser";}

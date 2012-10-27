/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2004
 *
 * Author(s):
 *	Volker Fischer, Oliver Haffenden
 *
 * Description:
 *	see TagPacketDecoderRSCIControl.cpp
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

#ifndef TAG_PACKET_DECODER_RSCI_CONTROL_H_INCLUDED
#define TAG_PACKET_DECODER_RSCI_CONTROL_H_INCLUDED

#include "TagPacketDecoder.h"
#include "RSCITagItemDecoders.h"

class CRSISubscriber;

class CTagPacketDecoderRSCIControl : public CTagPacketDecoder
{
public:
	// constructor: adds all of the decoders in the vocabulary to the list
	CTagPacketDecoderRSCIControl(CRSISubscriber *pSubscriber);

private:
	// Decoders for each of the tag items in the vocabulary
	CTagItemDecoderCact			TagItemDecoderCact;
	CTagItemDecoderCfre			TagItemDecoderCfre;
	CTagItemDecoderCdmo			TagItemDecoderCdmo;
	CTagItemDecoderCrec			TagItemDecoderCrec;
	CTagItemDecoderCpro			TagItemDecoderCpro;
	CTagItemDecoderCser			TagItemDecoderCser;
	// TODO other RSCI control tag items
};

#endif

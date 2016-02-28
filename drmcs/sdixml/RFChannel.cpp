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

#include "RFChannel.h"
#include "BitRates.h"

const char* RFChannel::INTERLEAVINGS[]={"long","short", NULL};
const char* RFChannel::ROBMODES[]={"mode_a","mode_b","mode_c","mode_d", NULL};
const char* RFChannel::VSPPS[]={"1/2","4/7","3/5","2/3", NULL};
const char* RFChannel::MSC_MODES[]={"64-QAM, no hier.", "64-QAM, hier. on I", "64-QAM, hier. on I&Q", "16-QAM, no hier.", NULL};
const char* RFChannel::PROT[]={"Least","Low","High","Most", NULL};
const char* RFChannel::SDC_MODES[]={"16-QAM","4-QAM", NULL};

RFChannel::RFChannel()
:Persist(),
  protection_a(),
  protection_b(),
  VSPP(),
  spectral_occupancy(),
  interleaver_depth(),
  msc_mode(),
  sdc_mode(),
  robustness_mode(),
  max_spp(),
  max_vspp(),
  max_bitrate()
{
    clearConfig();
    tag="channel";
}

RFChannel::RFChannel(const RFChannel& c)
:Persist(c),
  protection_a(c.protection_a),
  protection_b(c.protection_b),
  VSPP(c.VSPP),
  spectral_occupancy(c.spectral_occupancy),
  interleaver_depth(c.interleaver_depth),
  msc_mode(c.msc_mode),
  sdc_mode(c.sdc_mode),
  robustness_mode(c.robustness_mode),
  max_spp(c.max_spp),
  max_vspp(c.max_vspp),
  max_bitrate(c.max_bitrate)
{
}

RFChannel& RFChannel::operator=(const RFChannel& c)
{
  *reinterpret_cast<Persist *>(this) = Persist(c);
  protection_a = c.protection_a;
  protection_b = c.protection_b;
  VSPP = c.VSPP;
  spectral_occupancy = c.spectral_occupancy;
  interleaver_depth = c.interleaver_depth;
  msc_mode = c.msc_mode;
  sdc_mode = c.sdc_mode;
  robustness_mode = c.robustness_mode;
  max_spp = c.max_spp;
  max_vspp = c.max_vspp;
  max_bitrate = c.max_bitrate;
  return *this;
}

void RFChannel::clearConfig()
{
  protection_a=0;
  protection_b=0;
  VSPP=0;
  spectral_occupancy=3;
  interleaver_depth=0;
  msc_mode=0;
  sdc_mode=0;
  robustness_mode=0;
}

RFChannel::~RFChannel()
{
}

/*
 <robustness_mode>mode_a</robustness_mode> 
  <spectrum_occupancy>5</spectrum_occupancy> 
  <interleaver_depth>short</interleaver_depth> 
  <msc_mode>64-QAM, no heir.</msc_mode> 
  <sdc_mode>4-QAM</sdc_mode> 
- <msc_protection_level>
   <part_a>3</part_a> 
   <part_b>2</part_b> 
   <heir>3</heir> 
  </msc_protection_level>
  </channel>

*/   

void RFChannel::parse_msc_protection_level(xmlNodePtr n)
{
	if(n==NULL) {
        misconfiguration = true;
		return;
	}
	for(xmlNodePtr c=n; c; c=c->next){
		if(c->type==XML_ELEMENT_NODE){
		    parseUnsigned(c, "part_a", &protection_a);
		    parseUnsigned(c, "part_b", &protection_b);
		    parseUnsigned(c, "hierarchical", &VSPP);
		}
	}
}

void RFChannel::GetParams(xmlNodePtr n)
{
  parseUnsigned(n, "robustness_mode", &robustness_mode);
  parseUnsigned(n, "spectrum_occupancy", &spectral_occupancy);
  parseUnsigned(n, "interleaver_depth", &interleaver_depth);
  parseUnsigned(n, "msc_mode", &msc_mode);
  parseUnsigned(n, "sdc_mode", &sdc_mode);
  if(!xmlStrcmp(n->name,(const xmlChar*)"msc_protection_level")) {
    parse_msc_protection_level(n->children);
  }
}

void RFChannel::ReConfigure(xmlNodePtr config)
{
  Persist::ReConfigure(config);
  max_spp = CalcFrameLengthSPP(robustness_mode, spectral_occupancy,
	msc_mode, protection_b, protection_b, 0);
  max_vspp = CalcFrameLengthVSPP(robustness_mode, spectral_occupancy,
	msc_mode, 0);
  max_bitrate=20*(max_spp+max_vspp);
  /* TODO (#1#): validate params and set misconfiguration */
  misconfiguration = false;
}

void RFChannel::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  PutUnsignedEnum(writer, "robustness_mode", ROBMODES, robustness_mode);
  xmlTextWriterStartComment(writer);
  xmlTextWriterWriteFormatString(writer, 
        "The tag '%s' can take the following values:\n    ", "spectrum_occupancy");
  xmlTextWriterWriteString(writer, BAD_CAST "spectrum_occupancy        0  1  2  3  4  5\n    ");
  xmlTextWriterWriteString(writer, BAD_CAST "Channel bandwidth (kHz) 4,5  5  9 10 18 20\n");
  xmlTextWriterEndComment(writer);
  PutUnsigned(writer, "spectrum_occupancy", spectral_occupancy);
  PutUnsignedEnum(writer, "interleaver_depth", INTERLEAVINGS, interleaver_depth);
  PutUnsignedEnum(writer, "msc_mode", MSC_MODES, msc_mode);
  PutUnsignedEnum(writer, "sdc_mode", SDC_MODES, sdc_mode);
  xmlTextWriterStartElement(writer, BAD_CAST "msc_protection_level");
  xmlTextWriterStartComment(writer);
  xmlTextWriterWriteString(writer, BAD_CAST "part_a, part_b and hierarchical protection levels\n");
  xmlTextWriterWriteString(writer, BAD_CAST "    can take the following values\n");
  xmlTextWriterWriteString(writer, BAD_CAST "    for the MSC with 16-QAM\n");
  xmlTextWriterWriteString(writer, BAD_CAST "    Protection level Rall   R0  R1 RYlcm\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                 0    0,5  1/3 2/3     3\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                 1    0,62 1/2 3/4     4\n");
  xmlTextWriterWriteString(writer, BAD_CAST "    for the MSC with 64-QAM\n");
  xmlTextWriterWriteString(writer, BAD_CAST "    Protection level Rall  R0  R1  R2 RYlcm\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                   0 0,5  1/4 1/2 3/4     4\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                   1 0,6  1/3 2/3 4/5    15\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                   2 0,71 1/2 3/4 7/8     8\n");
  xmlTextWriterWriteString(writer, BAD_CAST "                   3 0,78 2/3 4/5 8/9    45\n");
  xmlTextWriterEndComment(writer);
  PutUnsigned(writer, "part_a", protection_a);
  PutUnsigned(writer, "part_b", protection_b);
  PutUnsigned(writer, "hierarchical", VSPP);
  xmlTextWriterEndElement(writer);
}

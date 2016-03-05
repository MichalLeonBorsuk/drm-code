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

#include "mdiout.h"
#include <iostream>
using namespace std;

MdiOut::MdiOut()
{
    tag_tx_order.resize(12);
    tag_tx_order[0]="*ptr";
    tag_tx_order[1]="dlfc";
    tag_tx_order[2]="fac_";
    tag_tx_order[3]="sdc_";
    tag_tx_order[4]="sdci";
    tag_tx_order[5]="robm";
    tag_tx_order[6]="str0";
    tag_tx_order[7]="str1";
    tag_tx_order[8]="str2";
    tag_tx_order[9]="str3";
    tag_tx_order[10]="tist";
    tag_tx_order[11]="info";
}

MdiOut::~MdiOut()
{
}

void MdiOut::ReConfigure(const DrmMuxConfig& config)
{
    if(config.misconfiguration)
        return;
    frame.clear();
    bytevector bits;
    bits << "DMDI" << bitset<32>(0);
    frame["*ptr"] = bits.data();
    //frame["*ptr"] << "RSCI" << bitset<32>(0);
    //frame["rpro"] << bitset<8>('A');
    if(config.info.length()>0) {
        bits.clear();
        bits.put(config.info);
        frame["info"] = bits.data();
    }
}

void MdiOut::buildFrame(const DrmMux& mux, DrmTime& timestamp)
{
    // msc
    string n = "str0";
    for(size_t i=0; i<mux.msc_bytes.size(); i++) {
        n[3] = static_cast<char>('0'+i);
        frame[n]=mux.msc_bytes[i];
    }
    // sdci
    frame["sdci"] = mux.sdc.sdci;
    // sdc
    if(mux.sdc_bytes.size()>0) {
        frame["sdc_"]=mux.sdc_bytes;
    } else {
        frame.erase("sdc_");
    }

    // fac
    frame["fac_"]=mux.fac_bytes;

    bytevector robm,dlfc,tist;

    // robm
    robm.put(mux.robustness_mode, 8);

    // dlfc
    dlfc.put(mux.frame_count, 32);

    // tist
    tist.put(timestamp.utco,14);
    tist.put(timestamp.tist_second(), 40);
    tist.put(timestamp.tist_ms(), 10);

    frame["robm"]=robm.data();
    frame["dlfc"]=dlfc.data();
    frame["tist"]=tist.data();
}

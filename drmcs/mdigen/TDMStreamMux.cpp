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

#include "TDMStreamMux.h"
using namespace std;

TDMStreamMux::TDMStreamMux():StreamMux()
{
}

TDMStreamMux::TDMStreamMux(const TDMStreamMux& s):StreamMux(s)
{
}

TDMStreamMux& TDMStreamMux::operator=(const TDMStreamMux& s)
{
    *this = s;
    return *this;
}

TDMStreamMux::~TDMStreamMux()
{
}

void TDMStreamMux::ReConfigure(const Stream& config)
{
    cout << "TDMStreamMux::ReConfigure" << endl;
    cout.flush();
    StreamMux::ReConfigure(config);
}

void TDMStreamMux::NextFrame(bytevector& out)
{
    size_t max = static_cast<size_t>(current.bytes_per_frame);
    size_t avail = max;
    out.clear();
    for(size_t i=0; i<sce.size(); i++) {
        sce[i]->NextFrame(out, avail);
        avail = max - out.size();
    }
    if(avail>0) {
        cout << "not enough data to fill frame, adding " << avail << " nulls" << endl;
        out.insert(out.end(), avail, 0);
    }
}

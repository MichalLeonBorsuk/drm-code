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

#include "DcpUtil.h"
#include <sstream>

using namespace std;

bool parseDcpUri(map<string,string>& out, const string& uri)
{
    istringstream s(uri);
    stringbuf b,a,scheme;
    s.get(b, '.');
    if(b.str()=="dcp")
        s.ignore();
    else
        return false;
    s.get(scheme, ':');
    s.ignore();
    s.get(a, '?');
    while(s.good()) {
        s.ignore();
        stringbuf param, value;
        string p,v;
        s.get(param, '=');
        s.ignore();
        s.get(value, '&');
        p=param.str();
        v=value.str();
        out[param.str()]=value.str();
    }
    istringstream ss(scheme.str());
    b.str("");
    ss.get(b, '.');
    out["type"]=b.str();
    if(ss.good()) {
        ss.ignore();
        b.str("");
        ss.get(b);
        if(b.str()=="pft") {
            out["pft"] = "true";
        }
    }
    istringstream t(a.str());
    b.str("");
    t.get(b, ':');
    if(t.good()) {
        out["target"] = b.str();
        t.ignore();
        a.str("");
        t.get(a, ':');
        b.str("");
        if(t.good()) {
            t.ignore();
            t.get(b, ':');
        }
    } else {
        out["target"] = a.str();
    }
    if(a.str().size()>0) {
        if(b.str().size()>0) {
            out["src_addr"]=a.str();
            out["dst_addr"]=b.str();
        } else {
            out["dst_addr"]=a.str();
        }
    }
    return true;
}


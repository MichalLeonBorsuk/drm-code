/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2007, 2013
 *
 * Author(s):
 *  Julian Cable, David Flamand
 *
 * Decription:
 *  sound interfaces
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

#ifndef _SELECTIONINTERFACE_H
#define _SELECTIONINTERFACE_H

#include "../GlobalDefinitions.h"
#include <vector>
#include <map>
#include <string>

#define DEFAULT_DEVICE_NAME ""

typedef struct {
    string          name;
    string          desc; /* description is optional, set to empty string when not used */
    map<int,bool>   samplerates;
} deviceprop;

class CSelectionInterface
{
public:
    virtual             ~CSelectionInterface() {}
    /* new/updated interface should reimplement that one */
    virtual void        Enumerate(vector<deviceprop>& devs, const int* desiredsamplerates)
    {
        vector<string> names;
        vector<string> descriptions;
        Enumerate(names, descriptions);
        deviceprop dp;
        for (const int* dsr=desiredsamplerates; *dsr; dsr++)
            dp.samplerates[abs(*dsr)] = true;
        devs.clear();
        for (size_t i=0; i<names.size(); i++)
        {
            dp.name = names.at(i);
            dp.desc = i<descriptions.size() ? descriptions.at(i) : "";
            devs.push_back(dp);
        }
    };
    virtual string      GetDev()=0;
    virtual void        SetDev(string sNewDev)=0;
protected:
    /* for backward compatibility */
    virtual void        Enumerate(vector<string>& names, vector<string>& descriptions)
    {
        (void)names;
        (void)descriptions;
    }
};

#endif

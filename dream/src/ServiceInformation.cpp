/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2001-2014
 *
 * Author(s):
 *  Julian Cable
 *
 * Description:
 *  see ServiceInformation.h
 *
 *
 *******************************************************************************
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

#include "ServiceInformation.h"

#include <QDomDocument>
#include <QFile>
#include <QTextStream>


/*
<service>
<serviceID id="e1.ce15.c221.0" />
<shortName>Radio 1</shortName>
<mediumName>BBC Radio 1</mediumName>
</service>
    servicesFilename = dir + "/services.xml";
    loadChannels (servicesFilename);
    saveChannels (servicesFilename);
*/

void
CServiceInformation::addChannel (const string& label, uint32_t sid)
{
    map<uint32_t, set<string> >::iterator x = data.find(sid);
    if(x == data.end()) {
        set<string> s;
        s.insert(label);
        data[sid] = s;
    }
    else {
        x->second.insert(label);
    }
    saveChannels();
}

void
CServiceInformation::saveChannels ()
{
    if(savepath == "")
        return;
    string filename = savepath+"/services.xml";
    QFile f (QString::fromUtf8(filename.c_str()));
    if (!f.open (QIODevice::WriteOnly))
    {
        return;
    }
    QDomDocument doc ("serviceInformation");
    QDomElement root = doc.createElement ("serviceInformation");
    doc.appendChild (root);
    QDomElement ensemble = doc.createElement ("ensemble");
    root.appendChild (ensemble);
    for (map < uint32_t, set<string> >::const_iterator i = data.begin();
            i != data.end(); i++)
    {
        const set<string>& si = i->second;
        QDomElement service = doc.createElement ("service");
        QDomElement serviceID = doc.createElement ("serviceID");
        serviceID.setAttribute ("id", QString::number (ulong (i->first), 16));
        service.appendChild (serviceID);
        for (set<string>::const_iterator j = si.begin(); j != si.end(); j++)
        {
            QDomElement shortName = doc.createElement ("shortName");
            QDomText text = doc.createTextNode (QString().fromUtf8(j->c_str()));
            shortName.appendChild (text);
            service.appendChild (shortName);
        }
        ensemble.appendChild (service);
    }
    QTextStream stream (&f);
    stream << doc.toString ();
    f.close ();

}

void
CServiceInformation::loadChannels ()
{
    if(savepath == "")
        return;

    QDomDocument domTree;
    string filename = savepath+"/services.xml";
    QFile f (QString::fromUtf8(filename.c_str()));
    if (!f.open (QIODevice::ReadOnly))
    {
        addChannel ("BBC & DW", 0xE1C248);
        return;
    }
    if (!domTree.setContent (&f))
    {
        f.close ();
        return;
    }
    f.close ();
    QDomNodeList ensembles = domTree.elementsByTagName ("ensemble");
    QDomNode n = ensembles.item (0).firstChild ();
    while (!n.isNull ())
    {
        if (n.nodeName () == "service")
        {
            QDomNode e = n.firstChild ();
            string name;
            QString sid;
            while (!e.isNull ())
            {
                if (e.isElement ())
                {
                    QDomElement s = e.toElement ();
                    if (s.tagName () == "shortName")
                        name = s.text().toUtf8().constData();
                    if (s.tagName () == "serviceID")
                        sid = s.attribute ("id", "0");
                }
                e = e.nextSibling ();
            }
            if (name != "")
            {
                addChannel (name, sid.toUInt (NULL, 16));
            }
        }
        n = n.nextSibling ();
    }
}

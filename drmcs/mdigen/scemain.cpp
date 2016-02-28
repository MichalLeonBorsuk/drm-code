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

#include <sys/stat.h>
#include <iostream>
#include "../../sdixml/trunk/version.h"
#include "../../sdixml/trunk/ServiceComponent.h"
#include "BBCSceFactory.h"
#include "DcpOut.h"
#include "sdiout.h"
#include "timestamp.h"

const char *svn_version ();

#ifdef WIN32
class Cwinsock
{
public:
    Cwinsock ()
    {
        WORD wVersionRequested;
        WSADATA wsaData;
        int err;

        wVersionRequested = MAKEWORD (2, 2);

        err = WSAStartup (wVersionRequested, &wsaData);
        if (err != 0)
            exit (1);

        if (LOBYTE (wsaData.wVersion) != 2 || HIBYTE (wsaData.wVersion) != 2)
        {
            WSACleanup ();
            exit (1);
        }
    }
    ~Cwinsock ()
    {
        WSACleanup ();
    }
};

#endif

void
ReConfigure (DcpOut& dcp, SdiOut& sdi, ServiceComponentEncoder * &sce, const string & cfg, int &max_frames)
{
    BBCSceFactory factory;
    ServiceComponent comp;
    int bytes_per_frame=-1;
    xmlDocPtr config_doc = xmlParseFile (cfg.c_str ());
    if (config_doc == NULL)
    {
        throw string ("missing configuration file ") + cfg;
    }
    for (xmlNodePtr n = config_doc->children->children; n; n = n->next)
    {
        if (n->type == XML_ELEMENT_NODE)
        {
            xmlChar *s = xmlNodeGetContent (n);
            string v;
            if (s)
            {
                v = (char *) s;
                xmlFree (s);
            }
            if (xmlStrEqual (n->name, BAD_CAST "max_frames"))
            {
                max_frames = atoi (v.c_str ());
            }
            if (xmlStrEqual (n->name, BAD_CAST "bytes_per_frame"))
            {
                bytes_per_frame = atoi (v.c_str ());
            }
            if (xmlStrEqual (n->name, BAD_CAST "component"))
            {
                comp.ReConfigure (n);
                if (sce)
                {
                    if(sce->current.implementor != comp.implementor)
                    {
                        delete sce;
                        sce = factory.create (comp);
                    }
                }
                else
                {
                    sce = factory.create (comp);
                }
            }
            if (xmlStrEqual (n->name, BAD_CAST "stdout"))
            {
                freopen (v.c_str (), "a", stdout);
            }
            if (xmlStrEqual (n->name, BAD_CAST "stderr"))
            {
                freopen (v.c_str (), "a", stderr);
            }
            if (xmlStrEqual (n->name, BAD_CAST "destination"))
            {
                dcp.ReConfigure(v);
            }
        }
    }
    xmlFreeDoc (config_doc);
    if(sce)
    {
        comp.bytes_per_frame = bytes_per_frame;
        sce->ReConfigure(comp);
    }
    else
        throw "no service component found in config file";
    sdi.ReConfigure();
}

int
main (int argc, char **argv)
{
#ifdef WIN32
    Cwinsock w;					// ensures WSAStartup and WSACleanup are called
#endif
    string cfg = "null";
    time_t mtime;
    struct stat s;
    if (argc == 2)
    {
        cfg = argv[1];
    }
    if (stat (cfg.c_str (), &s) == 0)
    {
        mtime = s.st_mtime;
    }
    else
    {
        mtime = 0;
    }
    cout << "BBC SDI Generator Rev " << svn_version ()
         << ", using libsdixml Rev " << libsdixml::svn_version () << endl
         << "Implementing SDI Schema version " << libsdixml::
         SDI_Schema_version () << endl << "Reading config from " << cfg <<
         endl;
    cout.flush ();
    int max_frames = -1;
    ServiceComponentEncoder* sce = NULL;
    SdiOut sdi;
    DcpOut out;
    uint16_t af_seq=rand();
    try {
        ReConfigure (out, sdi, sce, cfg, max_frames);
    }
    catch (string e)
    {
        cerr << e << endl;
        exit(1);
    }
    catch (char const *e)
    {
        cerr << e << endl;
        exit(1);
    }
    catch (...)
    {
        cerr << "unknown exception reading config file " << cfg << endl;
        exit(1);
    }
    DrmTime timestamp;
    timestamp.initialise(1,0);
    for(int transmitted_frames = 0; max_frames == -1 || transmitted_frames <= max_frames; transmitted_frames++)
    {
        try
        {
            sdi.buildFrame(*sce, timestamp);
            out.sendFrame(sdi.frame, sdi.tag_tx_order, af_seq);
            timestamp.increment();
        }
        catch (string e)
        {
            cerr << e << endl;
            cerr.flush ();
        }
        catch (char const *e)
        {
            cerr << e << endl;
            cerr.flush ();
        }
        struct stat s;
        if (stat (cfg.c_str (), &s) == 0)
        {
            if (mtime < s.st_mtime)
            {
                mtime = s.st_mtime;
                ReConfigure (out, sdi, sce, cfg, max_frames);
            }
        }
        af_seq++;
    }
#ifdef WIN32
    system ("PAUSE");
#endif
}

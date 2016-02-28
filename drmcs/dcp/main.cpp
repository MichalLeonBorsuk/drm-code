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
#include <cstdlib>
#include <iostream>
#include <iomanip>
#include <string>
#include "libxml/xinclude.h"
#include "libxml/xmlsave.h"
#include "version.h"

#include "DrmMuxConfig.h"

using namespace std;

int main(int argc, char *argv[])
{
    DrmMuxConfig mux;
    string sdi_file = argv[1];
    cout << "BBC SDI xml library Rev " << libsdixml::svn_version() << endl
         << "Implementing SDI Schema version " << libsdixml::SDI_Schema_version()
         << endl << "Reading config from " << sdi_file << endl;
    cout.flush();
    xmlDocPtr mdi_config_doc = xmlParseFile(sdi_file.c_str());
    if(mdi_config_doc == NULL) {
        throw string("missing configuration file ") + sdi_file;
    }
    int n = xmlXIncludeProcessTree(mdi_config_doc->children);
    if(n<0) {
        throw string("errors in XInclude processing");
    }
    cout << "XInclude processed " << n << " files" << endl;
    xmlSaveCtxtPtr ctxt = xmlSaveToFilename("processed.xml", "UTF-8", 0);
    xmlSaveTree(ctxt,	mdi_config_doc->children);
    xmlSaveClose(ctxt);
    mux.ReConfigure(mdi_config_doc->children);
    xmlFreeDoc(mdi_config_doc);
    cout << "starting output" << endl;
    xmlTextWriterPtr writer;
    writer=xmlNewTextWriterFilename("output.xml", 0);
    xmlTextWriterSetIndent(writer, 1);
    xmlTextWriterStartDocument(writer, NULL, "UTF-8", NULL);
    mux.Configuration(writer);
    xmlTextWriterEndDocument(writer);
    xmlFreeTextWriter(writer);
    if(mux.misconfiguration==true) {
        cerr << "error in configuration file" << endl;
    }
#ifdef WIN32
    system("PAUSE");
#endif
    return EXIT_SUCCESS;
}

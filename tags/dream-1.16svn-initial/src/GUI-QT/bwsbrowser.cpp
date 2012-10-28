/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	 Julian Cable
 *
 * Description: Broadcast Website Specialisation of TextBrowser
 *
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

#include "bwsbrowser.h"

BWSBrowser::BWSBrowser(QWidget * parent)
: QTextBrowser(parent),decoder(NULL), shomeUrl(""), restricted(false)
{
}

bool BWSBrowser::changed()
{
	if(decoder==NULL)
        return false;

#if 0
    TTransportID tid = decoder->GetNextTid();
    if (tid>=0)
    {
        CMOTObject	obj = decoder->GetObject(tid);
#else
	if(decoder->NewObjectAvailable())
	{
	    CMOTObject	obj;
		decoder->GetNextObject(obj);
#endif
        pages[obj.strName.c_str()] = obj;
        CMOTDirectory MOTDir;

        decoder->GetDirectory(MOTDir);

        /* Checks if the DirectoryIndex has values */
        if (MOTDir.DirectoryIndex.size() > 0)
        {
            if(restricted)
            {
                if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                    shomeUrl = MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();
            }
            else
            {
                if(MOTDir.DirectoryIndex.find(UNRESTRICTED_PC_PROFILE) != MOTDir.DirectoryIndex.end())
                    shomeUrl = MOTDir.DirectoryIndex[UNRESTRICTED_PC_PROFILE].c_str();
                else if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                    shomeUrl = MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();
            }
        }
		return true;
    }
    return false;
}

QVariant BWSBrowser::loadResource( int, const QUrl & name )
{
    map<QString,CMOTObject>::const_iterator i = pages.find(name.toString());
	if(i == pages.end())
        return QVariant::Invalid;

    const CVector < _BYTE >& vecData = i->second.Body.vecData;
    QString r;
    for(int i=0; i<vecData.Size(); i++)
        r += vecData[i];
    return r;
}


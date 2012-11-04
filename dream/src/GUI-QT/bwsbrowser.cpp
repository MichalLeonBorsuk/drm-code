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
#include <QDir>

BWSBrowser::BWSBrowser(QWidget * parent)
    : QTextBrowser(parent),decoder(NULL), sPath("."), restricted(false)
{
}

bool BWSBrowser::changed()
{
    if(decoder==NULL)
        return false;
    CMOTObject obj;

    /* Poll the data decoder module for new object */
    if (decoder->GetMOTObject(obj, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
    {
        /* Store received MOT object on disk */
        const QString strObjName = obj.strName.c_str();
        const QString strFileName = sPath + "/" + strObjName;

        SaveMOTObject(obj.Body.vecData, strFileName);
	pages[strObjName] = obj;

        if (strObjName.contains('/') == 0) /* if has a path is not the main page */
        {
            /* Get the current directory */
            CMOTDirectory MOTDir;

            if (decoder->GetMOTDirectory(MOTDir, CDataDecoder::AT_BROADCASTWEBSITE) == TRUE)
            {
                /* Checks if the DirectoryIndex has values */
                if (MOTDir.DirectoryIndex.size() > 0)
                {
		    QString shomeUrl;
                    if(MOTDir.DirectoryIndex.find(UNRESTRICTED_PC_PROFILE) != MOTDir.DirectoryIndex.end())
                        shomeUrl =
                            MOTDir.DirectoryIndex[UNRESTRICTED_PC_PROFILE].c_str();
                    else if(MOTDir.DirectoryIndex.find(BASIC_PROFILE) != MOTDir.DirectoryIndex.end())
                        shomeUrl =
                            MOTDir.DirectoryIndex[BASIC_PROFILE].c_str();
		    if(shomeUrl!="")
			setSource(QUrl(shomeUrl));
                }
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

void BWSBrowser::SaveMOTObject(const CVector<_BYTE>& vecbRawData,
                                  const QString& strFileName)
{
    /* First, create directory for storing the file (if not exists) */
    CreateDirectories(strFileName.latin1());

    /* Data size in bytes */
    const int iSize = vecbRawData.Size();

    /* Open file */
    FILE* pFiBody = fopen(strFileName.latin1(), "wb");

    if (pFiBody != NULL)
    {
        /* Write data byte-wise */
        for (int i = 0; i < iSize; i++)
        {
            const _BYTE b = vecbRawData[i];
            fwrite(&b, size_t(1), size_t(1), pFiBody);
        }

        /* Close the file afterwards */
        fclose(pFiBody);
    }
}

void BWSBrowser::CreateDirectories(const QString& filename)
{
    int i = 0;

    while (i < filename.length())
    {
        _BOOLEAN bFound = FALSE;

        while ((i < filename.length()) && (bFound == FALSE))
        {
            if (filename[i] == '/')
                bFound = TRUE;
            else
                i++;
        }

        if (bFound == TRUE)
        {
            /* create directory */
            const QString sDirName = filename.left(i);

            if (!QFileInfo(sDirName).exists())
                QDir().mkdir(sDirName);
        }

        i++;
    }
}


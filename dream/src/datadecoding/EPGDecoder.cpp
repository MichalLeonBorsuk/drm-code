/******************************************************************************\
 * British Broadcasting Corporation
 * Copyright (c) 2009
 *
 * Author(s):
 *	Julian Cable
 *
 * Description:
 *	ETSI DAB/DRM Electronic Programme Guide class
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

#include "EPGDecoder.h"
#include "epg/epgutil.h"
#include "../Parameter.h"
#include <iostream>

EPGDecoder::EPGDecoder(CParameter& p):DataApplication(p),motdecoder(p)
{
    saveDir = p.sDataFilesDirectory + "/epg/";
    cerr << "new EPG decoder" << endl;
}

EPGDecoder::~EPGDecoder()
{
}

void EPGDecoder::AddDataUnit(CVector<_BINARY>& vecbidata)
{
    cerr << "EPG new data unit" << endl;
    motdecoder.AddDataUnit(vecbidata);
	if (motdecoder.NewObjectAvailable())
	{
	    TTransportID tid = motdecoder.GetNextTid();
		CMOTObject NewObj = motdecoder.GetObject(tid);
		string fileName;
		bool advanced = false;
		if (NewObj.iContentType == 7)
		{
			for (size_t i = 0; i < NewObj.vecbProfileSubset.size(); i++)
				if (NewObj.vecbProfileSubset[i] == 2)
				{
					advanced = true;
					break;
				}
			int iScopeId = NewObj.iScopeId;
			if (iScopeId == 0)
				iScopeId = serviceid;
			fileName = epgFilename(NewObj.ScopeStart, iScopeId,
								   NewObj.iContentSubType, advanced);
		}
		else
		{
			fileName = NewObj.strName;
		}

		string path = saveDir + fileName;
		mkdirs(path);
		FILE *f = fopen(path.c_str(), "wb");
		if (f)
		{
			fwrite(&NewObj.Body.vecData.front(), 1,
				   NewObj.Body.vecData.size(), f);
			fclose(f);
		}
	}
}

/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	See DataDecoder.cpp
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

#if !defined(DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_)
#define DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_

#include "../GlobalDefinitions.h"
#include "../util/ReceiverModul.h"
#include "../util/CRC.h"
#include "DataApplication.h"
#include "../Parameter.h"

class CNews;
class CMOTSlideShowEncoder;
class CMOTDirectory;
class CMOTObject;
typedef int TTransportID;

/* Definitions ****************************************************************/
/* Maximum number of packets per stream */
#define MAX_NUM_PACK_PER_STREAM					4

class CDataDecoder:public CReceiverModul < _BINARY, _BINARY >
{
  public:
    CDataDecoder ();
    virtual ~CDataDecoder ();
	/* dummy assignment operator to help MSVC8 */
	CDataDecoder& operator=(const CDataDecoder&)
	{ throw "should not happen"; return *this;}

	void setApplication(int domain, int appId, DataApplicationFactory* fact)
    {
        factory[domain][appId] = fact;
    }

    DataApplication *getApplication(int packetId=0);
    void SetStream(int id) { iStreamID = id; }

  protected:
    class CDataUnit
    {
      public:
	CVector < _BINARY > vecbiData;
	bool bOK;
	bool bReady;

	void Reset ()
	{
	    vecbiData.Init (0);
	    bOK = false;
	    bReady = false;
	}
    };

    int iTotalPacketSize;
    int iNumDataPackets;
    int iMaxPacketDataSize;
    int iStreamID;
    CVector < int >veciCRCOk;

    int iContInd[MAX_NUM_PACK_PER_STREAM];
    CDataUnit DataUnit[MAX_NUM_PACK_PER_STREAM];
    DataApplication* app[MAX_NUM_PACK_PER_STREAM];
    map<int, map<int, DataApplicationFactory*> > factory;

    virtual void InitInternal (CParameter&);
    virtual void ProcessDataInternal (CParameter&);
    DataApplication *createApp(const CDataParam&, CParameter&);
    void decodePacket(CVector<_BINARY>& data);
};


#endif // !defined(DATADECODER_H__3B0BA660_CA3452363E7A0D31912__INCLUDED_)

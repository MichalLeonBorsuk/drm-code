 /******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 *	MOT applications (MOT Slideshow and Broadcast Web Site)
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

#include "MOTSlideShow.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Encoder                                                                      *
\******************************************************************************/
void
CMOTSlideShowEncoder::GetDataUnit (CVector < _BINARY > &vecbiNewData)
{
    /* Get new data group from MOT encoder. If the last MOT object was
       completely transmitted, this functions returns true. In this case, put
       a new picture to the MOT encoder object */
    if (MOTDAB.GetDataGroup (vecbiNewData) == true)
		AddNextPicture ();
}

bool
CMOTSlideShowEncoder::GetTransStat (string & strCurPict, _REAL & rCurPerc)
{
/*
	Name and current percentage of transmitted data of current picture.
*/
    strCurPict = strCurObjName;
    rCurPerc = MOTDAB.GetProgPerc ();

    if (vecPicFileNames.size () != 0)
		return true;
    else
		return false;
}

void
CMOTSlideShowEncoder::Init ()
{
    /* Reset picture counter for browsing in the vector of file names. Start
       with first picture */
    iPictureCnt = 0;
    strCurObjName = "";

    MOTDAB.Reset();

    AddNextPicture();
}

void
CMOTSlideShowEncoder::AddNextPicture ()
{
    /* Make sure at least one picture is in container */
    if (vecPicFileNames.size() > 0)
      {
	  /* Get current file name */
	  strCurObjName = vecPicFileNames[iPictureCnt].strName;

	  /* Try to open file binary */
	  FILE *pFiBody = fopen (strCurObjName.c_str (), "rb");

	  if (pFiBody != NULL)
	    {
		CMOTObject MOTPicture;
		_BYTE byIn;

		/* Set file name and format string */
		MOTPicture.strName = vecPicFileNames[iPictureCnt].strName;
		MOTPicture.strFormat = vecPicFileNames[iPictureCnt].strFormat;

		/* Fill body data with content of selected file */
		MOTPicture.vecbRawData.Init (0);

		while (fread ((void *) &byIn, size_t (1), size_t (1), pFiBody)
		       != size_t (0))
		  {
		      /* Add one byte = BITS_BINARY bits */
		      MOTPicture.vecbRawData.Enlarge (BITS_BINARY);
		      MOTPicture.vecbRawData.Enqueue ((uint32_t) byIn,
						      BITS_BINARY);
		  }

		/* Close the file afterwards */
		fclose (pFiBody);

		MOTDAB.SetMOTObject (MOTPicture);
	    }

	  /* Set file counter to next picture, test for wrap around */
	  iPictureCnt++;
	  if (iPictureCnt == vecPicFileNames.size())
	      iPictureCnt = 0;
      }
}

void
CMOTSlideShowEncoder::AddFileName (const string & strFileName,
				   const string & strFormat)
{
    /* Only ContentSubType "JFIF" (JPEG) and ContentSubType "PNG" are allowed
       for SlideShow application (not tested here!) */
    /* Add file name to the list */
    SPicDescr d;
    d.strName = strFileName;
    d.strFormat = strFormat;
    vecPicFileNames.push_back(d);
}

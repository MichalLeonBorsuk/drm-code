/******************************************************************************\
 * Technische Universitaet Darmstadt, Institut fuer Nachrichtentechnik
 * Copyright (c) 2001-2004
 *
 * Author(s):
 *	Volker Fischer
 *
 * Description:
 * Implementation of the text message application of section 6.5
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

#include "TextMessage.h"


/* Implementation *************************************************************/
/******************************************************************************\
* Text message encoder (for transmitter)                                       *
\******************************************************************************/
void CTextMessageEncoder::Encode(CVector<_BINARY>& pData)
{
	/* Set data for this piece from segment data */
	for (int i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
	{
		if (iByteCnt < CurTextMessage.GetSegSize(iSegCnt))
		{
			for (int j = 0; j < BITS_BINARY; j++)
				pData[i * BITS_BINARY + j] =
					(_BINARY) CurTextMessage[iSegCnt].Separate(1);
		}
		else
		{
			/* If the length of the last segment is not a multiple of four then
			   the incomplete frame shall be padded with 0x00 bytes */
			for (int j = 0; j < BITS_BINARY; j++)
				pData[i * BITS_BINARY + j] = 0;
		}

		iByteCnt++;
	}


	/* Take care of byte count, segment count and message count ------------- */
	/* Check if current segment is completely done */
	if (iByteCnt >=	CurTextMessage.GetSegSize(iSegCnt))
	{
		iByteCnt = 0;

		/* Next segment (test for wrap around) */
		iSegCnt++;
		if (iSegCnt == CurTextMessage.GetNumSeg())
		{
			iSegCnt = 0;

			/* Use next message --------------------------------------------- */
			iMessCnt++;
			if (iMessCnt >= vecstrText.size())
				iMessCnt = 0;

			/* Take care of toggle bit. If only one message is sent, toggle bit
			   must not be changed */
			if (vecstrText.size() > 1)
				biToggleBit = !biToggleBit;

			if (vecstrText.size() > 0)
                CurTextMessage.SetText(vecstrText[iMessCnt], biToggleBit);
		}

		/* Reset bit access of segment which is used next */
		if (iSegCnt < CurTextMessage.GetNumSeg())
            CurTextMessage[iSegCnt].ResetBitAccess();
	}
}

void CTextMessageEncoder::SetMessage(const string& strMessage)
{
	/* Allocate memory for new text message, store text and increment counter */
	vecstrText.push_back(strMessage);

	/* Init first message. TODO: this is always done when an additional text
	   is set but can only be done once. But for that we need an initialization
	   routine */
	/* Reset counter for encoder function */
	iSegCnt = 0; /* Segment counter */
	iByteCnt = 0; /* Byte counter */
	iMessCnt = 0; /* Message counter */

	biToggleBit = 0;
	CurTextMessage.SetText(vecstrText[0], biToggleBit);
}

void CTextMessageEncoder::ClearAllText()
{
	vecstrText.clear();
	iSegCnt = 0; /* Segment counter */
	iByteCnt = 0; /* Byte counter */
	iMessCnt = 0; /* Message counter */
	biToggleBit = 0;
	CurTextMessage.SetText("", biToggleBit);
}

void CTextMessage::SetText(const string& strMessage, const _BINARY biToggleBit)
{
	size_t	iNumBodyBytes;
	_BINARY	biFirstFlag;
	_BINARY	biLastFlag;
	CCRC	CRCObject;

	size_t iLenBytesOfText = strMessage.length();

	/* Calculate required number of segments. The body shall contain 16 bytes
	   of character data */
	size_t iNumSeg = (int) ceil((_REAL) iLenBytesOfText / BYTES_PER_SEG_TEXT_MESS);

	/* The text message may comprise up to 8 segments. Check the number of
	   segments, if number is larger, cut message */
	if (iNumSeg > 8)
	{
		iNumSeg = 8;
		iLenBytesOfText = iNumSeg * BYTES_PER_SEG_TEXT_MESS;
	}

	/* Allocate memory for segments */

	vvbiSegment.resize(iNumSeg);

	/* Generate segments ---------------------------------------------------- */
	/* Reset position in string */
	size_t iPosInStr = 0;

	for (size_t i = 0; i < iNumSeg; i++)
	{
		/* Calculate number of bytes for body */
        size_t iRemainingBytes = iLenBytesOfText - iPosInStr;

		/* All segments should have the maximum number of bytes per segment
		   except the last one which takes the remaining bytes */
		if (iRemainingBytes > BYTES_PER_SEG_TEXT_MESS)
			iNumBodyBytes = BYTES_PER_SEG_TEXT_MESS;
		else
			iNumBodyBytes = iRemainingBytes;

		/* Init segment vector: "Beginning of segment" 4 * 8 bits,
		   Header 16 bits, body n * 8 bits, CRC 16 bits */
		vvbiSegment[i].Init(4 * 8 + 16 + iNumBodyBytes * 8 + 16);

		/* Reset enqueue function */
		vvbiSegment[i].ResetBitAccess();


		/* Generate "beginning of segment" identification by "all 0xFF" ----- */
		for (int j = 0; j < NUM_BYTES_TEXT_MESS_IN_AUD_STR; j++)
			vvbiSegment[i].Enqueue((uint32_t) 0xFF, BITS_BINARY);


		/* Header ----------------------------------------------------------- */
		/* Toggle bit */
		vvbiSegment[i].Enqueue((uint32_t) biToggleBit, 1);

		/* Determine position of segment */
		if (i == 0)
			biFirstFlag = 1;
		else
			biFirstFlag = 0;

		if (iLenBytesOfText - iPosInStr <= BYTES_PER_SEG_TEXT_MESS)
			biLastFlag = 1;
		else
			biLastFlag = 0;

		/* Enqueue first flag */
		vvbiSegment[i].Enqueue((uint32_t) biFirstFlag, 1);

		/* Enqueue last flag */
		vvbiSegment[i].Enqueue((uint32_t) biLastFlag, 1);

		/* Command flag. This is a data block -> command flag = 0 */
		vvbiSegment[i].Enqueue((uint32_t) 0, 1);

		/* Field 1: specify the number of bytes in the body minus 1 (It
		   shall normally take the value 15 except in the last segment) */
		vvbiSegment[i].Enqueue((uint32_t) iNumBodyBytes - 1, 4);

		/* Field 2. If First flag = "1", this field shall contain the
		   value "1111" */
		if (biFirstFlag == 1)
			vvbiSegment[i].Enqueue((uint32_t) 15 /* 1111 */, 4);
		else
		{
			/* Rfa. The bit shall be set to zero until it is defined */
			vvbiSegment[i].Enqueue((uint32_t) 0, 1);

			/* SegNum: specify the sequence number of the current segment
			   minus 1. The value 0 is reserved for future use */
			vvbiSegment[i].Enqueue((uint32_t) i, 3);
		}

		/* Rfa. These bits shall be set to zero until they are defined */
		vvbiSegment[i].Enqueue((uint32_t) 0, 4);


		/* Body ------------------------------------------------------------- */
		/* Set body bytes */
		for (size_t j = 0; j < iNumBodyBytes; j++)
		{
			vvbiSegment[i].Enqueue((uint32_t) strMessage.at(iPosInStr),
				BITS_BINARY);

			iPosInStr++;
		}


		/* CRC -------------------------------------------------------------- */
		/* Reset bit access and skip segment beginning piece (all 0xFFs) */
		vvbiSegment[i].ResetBitAccess();
		vvbiSegment[i].Separate(BITS_BINARY * NUM_BYTES_TEXT_MESS_IN_AUD_STR);

		/* Calculate the CRC and put it at the end of the segment */
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (size_t j = 0; j < iNumBodyBytes + 2 /* Header */; j++)
			CRCObject.AddByte((_BYTE) vvbiSegment[i].Separate(BITS_BINARY));

		/* Now, pointer in "enqueue"-function is back at the same place,
		   add CRC */
		vvbiSegment[i].Enqueue(CRCObject.GetCRC(), 16);


		/* Reset bit access for using segment in encoder function */
		vvbiSegment[i].ResetBitAccess();
	}
}

size_t CTextMessage::GetSegSize(size_t iSegID) const
{
    if(iSegID>= vvbiSegment.size())
        return 0;
    return vvbiSegment[iSegID].size() / BITS_BINARY;
}

/******************************************************************************\
* Text message decoder (for receiver)                                          *
\******************************************************************************/
void CTextMessageDecoder::Decode(CVector<_BINARY>& pData)
{
	int			i, j;
	bool	bBeginningFound;

	/* Reset binary vector function. Always four bytes of data */
	pData.ResetBitAccess();

	/* Identify the beginning of a segment, all four bytes are 0xFF, otherwise
	   store total new buffer in internal intermediate buffer. This buffer is
	   read, when a beginning was found */
	bBeginningFound = true;
	for (i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
	{
		if (pData.Separate(BITS_BINARY) != 0xFF)
			bBeginningFound = false;
	}

	if (bBeginningFound)
	{
		/* Analyse old stream buffer, which should be completed now --------- */
		/* Get header information. This function separates 16 bits from
		   stream */
		biStreamBuffer.ResetBitAccess();
		ReadHeader();

		/* Check CRC */
		biStreamBuffer.ResetBitAccess();
		CRCObject.Reset(16);

		/* "byLengthBody" was defined in the header */
		for (i = 0; i < byLengthBody + 2 /* Header */; i++)
			CRCObject.AddByte(_BYTE(biStreamBuffer.Separate(BITS_BINARY)));

		if (CRCObject.CheckCRC(biStreamBuffer.Separate(16)) == true)
		{
			if (biCommandFlag == 1)
			{
				switch (byCommand)
				{
				case 1: /* 0001 */
					/* The message shall be removed from the display */
					ClearText();
					break;
				}
			}
			else
			{
				/* Body ----------------------------------------------------- */
				/* Evaluate toggle bit */
				if (biToggleBit != biOldToggleBit)
				{
					/* A new message is sent, clear all old segments */
					ResetSegments();

					biOldToggleBit = biToggleBit;
				}


				/* Read all bytes in stream buffer -------------------------- */
				/* First, check if segment was already OK and if new data has
				   the same content or not. If the content is different, a new
				   message is being send, clear all other segments */
				if (Segment[bySegmentID].bIsOK == true)
				{
					/* Reset bit access and skip header bits to go directly to
					   the body bytes */
					biStreamBuffer.ResetBitAccess();
					biStreamBuffer.Separate(16);

					bool bIsSame = true;
					for (i = 0; i < byLengthBody; i++)
					{
						if (Segment[bySegmentID].byData[i] !=
							biStreamBuffer.Separate(BITS_BINARY))
						{
							bIsSame = false;
						}
					}

					/* If a new message is sent, clear all old segments */
					if (bIsSame == false)
						ResetSegments();
				}

				/* Reset bit access and skip header bits to go directly to the
				   body bytes */
				biStreamBuffer.ResetBitAccess();
				biStreamBuffer.Separate(16);

				/* Get all body bytes */
				for (i = 0; i < byLengthBody; i++)
				{
					Segment[bySegmentID].byData[i] =
						_BINARY(biStreamBuffer.Separate(BITS_BINARY));
				}

				/* Set length of this segment and OK flag */
				Segment[bySegmentID].iNumBytes = byLengthBody;
				Segment[bySegmentID].bIsOK = true;

				/* Check length of text message */
				if (biLastFlag == 1)
					iNumSegments = bySegmentID + 1;

				/* Set text to display */
				SetText();
			}
		}

		/* Reset bit count */
		iBitCount = 0;
	}
	else
	{
		for (i = 0; i < NUM_BYTES_TEXT_MESS_IN_AUD_STR; i++)
		{
			/* Check, if number of bytes is not too big */
			if (iBitCount < TOT_NUM_BITS_PER_PIECE)
			{
				for (j = 0; j < BITS_BINARY; j++)
					biStreamBuffer[iBitCount + j] = pData[i * BITS_BINARY + j];

				iBitCount += BITS_BINARY;
			}
		}
	}
}

void CTextMessageDecoder::ReadHeader()
{
	/* Toggle bit */
	biToggleBit = (_BINARY) biStreamBuffer.Separate(1);

	/* First flag */
	biFirstFlag = (_BINARY) biStreamBuffer.Separate(1);

	/* Last flag */
	biLastFlag = (_BINARY) biStreamBuffer.Separate(1);

	/* Command flag */
	biCommandFlag = _BINARY(biStreamBuffer.Separate(1));

	if (biCommandFlag == 1)
	{
		/* Command */
		/* Field 1 */
		byCommand = (_BYTE) biStreamBuffer.Separate(4);

		/* 0001: the message shall be removed from the display */

		/* Field 2: This field shall contain the value "1111", not
		   tested here */
		biStreamBuffer.Separate(4);

		/* When a command ist send, no body is present */
		byLengthBody = 0;
	}
	else
	{
		/* Field 1: specify the number of bytes in the body minus 1 */
		byLengthBody = (_BYTE) biStreamBuffer.Separate(4) + 1;

		/* Field 2, Rfa */
		biStreamBuffer.Separate(1);

		/* SegNum: specify the sequence number of the current segment
		   minus 1. The value 0 is reserved for future use */
		bySegmentID = (_BYTE) biStreamBuffer.Separate(3);

		if (biFirstFlag == 1)
			bySegmentID = 0;
	}

	/* Rfa */
	biStreamBuffer.Separate(4);
}

void CTextMessageDecoder::Init(string* pstrNewPText)
{
	pstrText = pstrNewPText;
	biOldToggleBit = 0;
	iBitCount = 0;

	/* Init segments */
	ResetSegments();

	/* Init and reset stream buffer */
	biStreamBuffer.Init(TOT_NUM_BITS_PER_PIECE, 0);
}

void CTextMessageDecoder::SetText()
{
	int i;

#ifndef _DEBUG_
	if (iNumSegments != 0)
#endif
	{
#ifndef _DEBUG_
		/* Check, if all segments are ready */
		bool bTextMessageReady = true;
		for (i = 0; i < iNumSegments; i++)
		{
			if (Segment[i].bIsOK == false)
				bTextMessageReady = false;
		}

		if (bTextMessageReady == true)
#endif
		{
			/* Clear text */
			*pstrText = "";

			for (i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
			{
				if (Segment[i].bIsOK == true)
				{
					for (int j = 0; j < Segment[i].iNumBytes; j++)
					{
						/* Get character */
						char cNewChar = Segment[i].byData[j];

#ifdef QT_GUI_LIB
						/* Append new character */
						(*pstrText).append(&cNewChar, 1);
#else
						/* NOT QT_GUI_LIB */
						switch (cNewChar)
						{
						case 0x0A:
							/* Code 0x0A may be inserted to indicate a preferred
							   line break */
						case 0x1F:
							/* Code 0x1F (hex) may be inserted to indicate a
							   preferred word break. This code may be used to
							   display long words comprehensibly */

							(*pstrText).append("\r\n", 2);
							break;

						case 0x0B:
							/* End of a headline */

							/* Two line-breaks */
							(*pstrText).append("\r\n\r\n", 4);
							cNewChar = 0x0A;
							break;

						default:
							/* Append new character */
							(*pstrText).append(&cNewChar, 1);
							break;
						}
#endif
					}
				}
			}
		}
	}
}

void CTextMessageDecoder::ClearText()
{
	/* Reset segments */
	ResetSegments();

	/* Clear text */
	*pstrText = "";
}

void CTextMessageDecoder::ResetSegments()
{
	for (int i = 0; i < MAX_NUM_SEG_TEXT_MESSAGE; i++)
		Segment[i].bIsOK = false;

	iNumSegments = 0;
}

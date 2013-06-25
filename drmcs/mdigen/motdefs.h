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

#ifndef _MOTDEFS_H
#define _MOTDEFS_H

#define CONTENT_SUBTYPE_OBJECT_TRANSFER	0
#define CONTENT_OBJECT_TRANSFER	0
#define CONTENT_SUBTYPE_TEXT 1
#define CONTENT_SUBTYPE_HTTP	1
#define CONTENT_SUBTYPE_HTML	2
#define CONTENT_SUBTYPE_XML		3
#define	CONTENT_SUBTYPE_XSL		4

#define SUBTYPE_COMP_START			60
#define CONTENT_SUBTYPE_COMP_HTML	63 //define this for compressed HTML
#define CONTENT_SUBTYPE_COMP_XML	64 //define this for compressed XML
#define CONTENT_SUBTYPE_COMP_XSL	65 //define this for compressed XSL
#define CONTENT_SUBTYPE_COMP_BMP    66 //define this for compressed XSL
#define SUBTYPE_COMP_END			70

#define CONTENT_GENERAL_DATA 0
#define CONTENT_TEXT		 1
#define CONTENT_IMAGE		 2
#define CONTENT_AUDIO		 3
#define CONTENT_PROP		63

#define CONTENT_SUBTYPE_GIF			0
#define CONTENT_SUBTYPE_JFIF		1
#define CONTENT_SUBTYPE_BMP		    2
#define CONTENT_SUBTYPE_PNG		    3

#define CONTENT_SUBTYPE_HVXC	20

#define CONTENT_NAME	12

#define MOT_VERSION_PAR 6
#define DATAGROUP_HEADER_TYPE 3
#define DATAGROUP_BODY_TYPE 4
#define DATAGROUP_DIRECTORY_TYPE 6

#define MAX_NO_FILES	30

#define CORE_SIZE	7

#define EXTENSION_SIZE	200

#define COMP_SRC_LEN 200
#define COMP_DEST_LEN 230

#define BLOCK_SIZE 200

#define SEGMENT_HDR_LEN			2
#define SESSION_HEADER_LEN_S	2
#define UA_FIELD_LEN_S			1
#define TRANS_ID_LEN				2

#define DATA_GROUP_HDR_LEN_L	4
#define DATA_GROUP_HDR_LEN_S	2

#define DG_CRC_LEN			2

#define EXTENSION_FLAG		128
#define	CRC_FLAG			64
#define SEGMENT_FLAG		32
#define USER_ACCESS_FLAG	16

#define CRC_LEN				2
#define	NO_CRC_BITS			16
#define CRC_KEY				0x11021

#define MAX_PACKET_SIZE		1000

#endif // !defined(_MOTDEFS_H)

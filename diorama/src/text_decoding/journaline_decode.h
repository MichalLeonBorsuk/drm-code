/******************************************************************************/
/*                                                                            */
/*  University of Kaiserslautern, Institute of Communications Engineering     */
/*  Copyright (C) 2005 Torsten Schorr                                         */
/*                                                                            */
/*  Author(s)    : Torsten Schorr (schorr@eit.uni-kl.de)                      */
/*  Project start: 09.02.2005                                                 */
/*  Last change  : 17.02.2005                                                 */
/*                                                                            */
/******************************************************************************/
/*                                                                            */
/*  This program is free software; you can redistribute it and/or modify      */
/*  it under the terms of the GNU General Public License as published by      */
/*  the Free Software Foundation; either version 2 of the License, or         */
/*  (at your option) any later version.                                       */
/*                                                                            */
/*  This program is distributed in the hope that it will be useful,           */
/*  but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             */
/*  GNU General Public License for more details.                              */ 
/*                                                                            */
/*  You should have received a copy of the GNU General Public License         */
/*  along with this program; if not, write to the Free Software               */
/*  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  journaline_decode.h (part of journaline_decode)                           */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*  This program is based on the 'NewsService Journaline(R) Decoder'          */
/*  Copyright (c) 2003, 2004 by Fraunhofer IIS, Erlangen, Germany             */
/*                                                                            */
/*                                                                            */
/*  To use the 'NewsService Journaline(R) Decoder' software for               */
/*  COMMERCIAL purposes, please contact Fraunhofer IIS for a                  */
/*  commercial license (see below for contact information)!                   */
/*                                                                            */
/* --------------------------------------------------------------------       */
/*                                                                            */
/*  Contact:                                                                  */
/*   Fraunhofer IIS, Department 'Broadcast Applications'                      */
/*   Am Wolfsmantel 33, 91058 Erlangen, Germany                               */
/*   http://www.iis.fraunhofer.de/dab                                         */
/*   mailto:bc-info@iis.fraunhofer.de                                         */
/*                                                                            */
/******************************************************************************/


const char *HTML_title_template = "\
<html>\n\
\t<head>\n\
\t\t<meta http-equiv=\"content-Type\" content=\"text/html; charset=utf-8\" />\n\
\t\t<Title>%s</Title>\n\
\t</head>\n\
\t<body bgcolor=\"#CAE8FF\">\n\
\t\t<font face=\"Arial,Helvetica\" color=\"#425294\">\n";

const char *HTML_title_refresh_template = "\
<html>\n\
\t<head>\n\
\t\t<meta http-equiv=\"content-Type\" content=\"text/html; charset=utf-8\" />\n\
\t\t<meta http-equiv=\"refresh\" content=\"10\">\n\
\t\t<Title>%s</Title>\n\
\t</head>\n\
\t<body bgcolor=\"#CAE8FF\">\n\
\t\t<font face=\"Arial,Helvetica\" color=\"#425294\">\n";

const char *HTML_end_template = "\
\t\t<br/><br/>\n\
\t\t</font>\n\
\t\t<hr/>\n\
\t\t<font face=\"Arial,Helvetica\" color=\"#425294\" size=\"2\">\n\
\t\t<p>This page was created by <a href=\"http://nt.eit.uni-kl.de/forschung/diorama/\">Diorama</a>, the open-source DRM receiver for Matlab</p>\n\
\t\t<p><a href=\"http://nt.eit.uni-kl.de/engl_index.html\">University of Kaiserslautern, Institute of Telecommunications</a></p>\n\
\t\t<p>Contact: <a href=\"mailto:diorama@eit.uni-kl.de\">diorama@eit.uni-kl.de</a></p>\n\
\t\t<font face=\"Arial,Helvetica\" color=\"#425294\" size=\"1\">\n\
\t\t<br/>\n\
\t\t<p>Features NewsService Journaline(R) decoder technology by</p>\n\
\t\t<p>Fraunhofer IIS, Erlangen, Germany.</p>\n\
\t\t<p>For more information visit <a href=\"http://www.iis.fhg.de/dab\">http://www.iis.fhg.de/dab</a></p>\n\
\t\t</font>\n\
\t</body>\n\
</html>\n";

const char *HTML_heading_template = "\t\t<h1>%s</h1>\n";
const char *HTML_link_template = "\t\t<p><a href=\"%04x.html\">%s</a></p>\n";
const char *HTML_item_template = "\t\t<p>%s</p>\n";

const char *mstruct_field_name0 = "filename";
const char *mstruct_field_name1 = "update";
const char *mstruct_field_name2 = "filebody";
const char *mstruct_field_names[] = {mstruct_field_name0, mstruct_field_name1, mstruct_field_name2};


#define PROGNAME "journaline_decode"

extern "C" int showDdNewsSvcDecErr;
extern "C" int showDdNewsSvcDecInfo;
extern "C" int showDdDabDgDecErr;
extern "C" int showDdDabDgDecInfo;

static int firstcall = 1;

static DAB_DATAGROUP_DECODER_t dgdec = NULL; // an instance of a DAB data group decoder
static NEWS_SVC_DEC_decoder_t newsdec = NULL; // an instance of a Journaline(R) news service decoder

static NMLEscapeSequences2HTML sequence_handler;

static NEWS_SVC_DEC_obj_availability_t *watchlist = NULL;
static int length_of_watchlist = 0;

static struct Objects_Available {
	 unsigned long number_of_elements;
	 NEWS_SVC_DEC_obj_availability_t *chg_list;
} objects_available;

struct Updated_Objects {
	unsigned short objID;
	NEWS_SVC_DEC_obj_availability_status_t status;
	NML *nml;
	struct Updated_Objects *next;
};

static struct LinkList {
	unsigned short linkObjID;
	unsigned short parentObjID;
	struct LinkList *next;
} *pending_link_list = NULL;

static struct News_Lists {
	NEWS_SVC_DEC_obj_availability_t **watchlist;
	int *length_of_watchlist;
	struct LinkList **linklist;
	struct Updated_Objects **objects_root;
	int no_of_updates;
} news_lists;


	

void mexFunction(int , mxArray *[], int , const mxArray *[]);

void datagroup_callback(const DAB_DATAGROUP_DECODER_msc_datagroup_header_t *, const unsigned long, const unsigned char *, void *);

void object_avail_callback(unsigned long, NEWS_SVC_DEC_obj_availability_t *, void * );

void journaline_decode_exit (void);

void explore_news_tree(struct News_Lists *, unsigned short);

int  update_watchlist(int, NEWS_SVC_DEC_obj_availability_t **, int *);

int  update_linklist(int, struct LinkList **, unsigned short, unsigned short);

void create_output(mxArray *[], struct News_Lists *);


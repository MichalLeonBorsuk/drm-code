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

#ifndef _TRANSLATINGTEXTSCE_H
#define _TRANSLATINGTEXTSCE_H

#include <libxml/encoding.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <map>
#include <list>
#include <platform.h>
#include <persist.h>
#include <ServiceComponentEncoder.h>
#include "TextEncode.h"

using namespace std;

class CTranslatingTextSCE : public ServiceComponentEncoder, public Persist
{
public:

  CTranslatingTextSCE();
  CTranslatingTextSCE(const CTranslatingTextSCE& e);
  CTranslatingTextSCE& operator=(const CTranslatingTextSCE& e);
  virtual ~CTranslatingTextSCE();
  virtual void ReConfigure(const ServiceComponent&);
  virtual void NextFrame(bytevector& buf, size_t max, double stoptime=0);
  virtual void GetParams(xmlNodePtr n);
  virtual void PutPrivateParams(xmlTextWriterPtr writer);
  virtual string next_message();

protected:

  virtual void per_frame_processing() {}
  CTextEncode textEncoder;

  map<string,string> translations, lowtimes, hightimes;
  list< pair<string,string> > sorted_translations;
  string source_encoding, source_timezone, destination_timezone;

  void init_internationalisation();
  string internationalise(const string& s);
  void replaceall(string& s, const string& bad, const string& good);
  void map_tz(time_t time, time_t offset, map<string,string> &times);
#ifdef WIN32
  struct TZI {
      LONG Bias, StandardBias, DaylightBias;
      SYSTEMTIME StandardDate, DaylightDate;
  };

  static const char *tzkey;
  void get_local_time(const string& zone, const FILETIME& from, FILETIME& to);
#else
  void get_local_time(const string& zone, time_t from, time_t& to);
#endif
  int get_tz_offset();

  int tz_offset; // seconds east or west
};

#endif

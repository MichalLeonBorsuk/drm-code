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

#include "TranslatingTextSCE.h"
#ifdef WIN32
#include <windows.h>
#endif
#include <cstdlib>
#include <cwchar>
#include <time.h>
#include <iostream>
#include <locale.h>

using namespace std;

static wchar_t ebu2unicode[] = {
/*          0      1      2      3      4      5      6      7      8      9      a      b      c      d      e      f */
/* 00 */  0x00,  0x01,  0x02,  0x03,  0x04,  0x05,  0x06,  0x07,  0x08,  0x09,  0x0a,  0x0b,  0x0c,  0x0d,  0x0e,  0x0f,
/* 01 */  0x10,  0x11,  0x12,  0x13,  0x14,  0x15,  0x16,  0x17,  0x18,  0x19,  0x1a,  0x1b,  0x1c,  0x1d,  0x1e,  0x1f,
/* 02 */  0x20,  0x21,  0x22,  0x23,  0xa4,  0x25,  0x26,  0x27,  0x28,  0x29,  0x2a,  0x2b,  0x2c,  0x2d,  0x2e,  0x2f,
/* 03 */  0x30,  0x31,  0x32,  0x33,  0x34,  0x35,  0x36,  0x37,  0x38,  0x39,  0x3a,  0x3b,  0x3c,  0x3d,  0x3e,  0x3f,
/* 04 */  0x40,  0x41,  0x42,  0x43,  0x44,  0x45,  0x46,  0x47,  0x48,  0x49,  0x4a,  0x4b,  0x4c,  0x4d,  0x4e,  0x4f,
/* 05 */  0x50,  0x51,  0x52,  0x53,  0x54,  0x55,  0x56,  0x57,  0x58,  0x59,  0x5a,  0x5b,  0x5c,  0x5d,0x2015,  0x5f,
/* 06 */0x2016,  0x61,  0x62,  0x63,  0x64,  0x65,  0x66,  0x67,  0x68,  0x69,  0x6a,  0x6b,  0x6c,  0x6d,  0x6e,  0x6f,
/* 07 */  0x70,  0x71,  0x72,  0x73,  0x74,  0x75,  0x76,  0x77,  0x78,  0x79,  0x7a,  0x7b,  0x7c,  0x7d,  0x20,  0x20,
/* 08 */  0xe1,  0xe0,  0xe9,  0xe8,  0xed,  0xec,  0xf3,  0xf2,  0xfa,  0xf9,  0xd1,  0xc7, 0x15e,  0xdf,  0xa1, 0x132,
/* 09 */  0xe2,  0xe4,  0xea,  0xeb,  0xee,  0xef,  0xf4,  0xf6,  0xfb,  0xfc,  0xf1,  0xe7, 0x15f, 0x11f,  0x20, 0x133,
/* 0a */  0xaa, 0x3b1,  0xa9,0x2030, 0x11e, 0x11b, 0x148, 0x151, 0x3c0,  0x20,  0xa3,  0x24,0x2190,0x2191,0x2192,0x2193,
/* 0b */  0xba,  0xb9,  0xb2,  0xb3,  0xb1, 0x130, 0x144, 0x171,  0xb5,  0xbf,  0xf7,  0xb0,  0xbc,  0xbd,  0xbe,  0xa7,
/* 0c */  0xc1,  0xc0,  0xc9,  0xc8,  0xcd,  0xcc,  0xd3,  0xd2,  0xda,  0xd9, 0x158, 0x10c, 0x160, 0x17d, 0x110, 0x13f,
/* 0d */  0xc2,  0xc4,  0xca,  0xcb,  0xce,  0xef,  0xd4,  0xd6,  0xdb,  0xdc, 0x159, 0x10d, 0x161, 0x17e, 0x111, 0x140,
/* 0e */  0xc3,  0xc5,  0xc6, 0x152, 0x177,  0xdd,  0xd5,  0xd8,  0xde, 0x397, 0x154, 0x106, 0x15a, 0x179,0x2213,  0xf0,
/* 0f */  0xe3,  0xe5,  0xe6, 0x153, 0x175,  0xfd,  0xf5,  0xf8,  0xfe, 0x3b7, 0x155, 0x107, 0x15b, 0x17a,  0x20,  0x00
};


CTranslatingTextSCE::CTranslatingTextSCE():ServiceComponentEncoder(),Persist(),textEncoder(),
  translations(), lowtimes(), hightimes(), sorted_translations()
{
// initialise things in case there is no private section in the config file
  source_encoding="";
#ifdef WIN32
  source_timezone="GMT Standard Time";
#else
  source_timezone="Europe/London";
#endif
  destination_timezone="UTC";
  tz_offset = get_tz_offset();
}

CTranslatingTextSCE::~CTranslatingTextSCE()
{
}

CTranslatingTextSCE::CTranslatingTextSCE(const CTranslatingTextSCE& e):ServiceComponentEncoder(e),
Persist(e),textEncoder(e.textEncoder), translations(e.translations),
lowtimes(e.lowtimes), hightimes(e.hightimes), sorted_translations(e.sorted_translations),
source_encoding(e.source_encoding),
source_timezone(e.source_timezone), destination_timezone(e.destination_timezone), tz_offset(e.tz_offset)
{
}

CTranslatingTextSCE& CTranslatingTextSCE::operator=(const CTranslatingTextSCE& e)
{
  ServiceComponentEncoder(*this) = e;
  Persist(*this) = e;
  textEncoder = e.textEncoder;
  translations = e.translations;
  lowtimes = e.lowtimes;
  hightimes = e.hightimes;
  sorted_translations = e.sorted_translations;
  source_encoding = e.source_encoding;
  source_timezone = e.source_timezone;
  destination_timezone = e.destination_timezone;
  tz_offset = e.tz_offset;
  return *this;
}

void CTranslatingTextSCE::NextFrame(bytevector& buf, size_t max, double)
{
  per_frame_processing();
  // see if we need to code a new message
  if (textEncoder.empty()) {
    string s = internationalise(next_message());
    textEncoder.m_toggle = ! textEncoder.m_toggle;
    if(s.length()>128) {
      s.erase(128);
      s.replace(125, 3, "\xE2\x80\xA6");
    }
    textEncoder.puttext(s.c_str());
  }
  // get whatever we have - might be all zeroes
  textEncoder.getsubsegment(buf);
}

string CTranslatingTextSCE::next_message()
{
  return current.source_selector;
}

void CTranslatingTextSCE::ReConfigure(const ServiceComponent& config)
{
  cerr << "CTranslatingTextSCE::ReConfigure" << endl;
  ServiceComponentEncoder::ReConfigure(config);
  tz_offset = get_tz_offset();
  xmlDocPtr private_config_doc = xmlParseMemory(current.private_config.c_str(), current.private_config.length());
  if(private_config_doc == NULL) {
    //throw string("bad private configuration") + current.private_config;
#ifdef WIN32
    source_timezone = destination_timezone = "GMT Standard Time";
#else
    source_timezone = destination_timezone = "UTC";
#endif
  }
  GetParams(private_config_doc->children);
  xmlFreeDoc(private_config_doc);
  init_internationalisation();
  if(((unsigned)current.bytes_per_frame)!=CTextEncode::BYTES_PER_FRAME)
    throw "Text Message bytes per frame must be 4";
  textEncoder.reset();
  setlocale(LC_ALL, "en_GB.UTF-8");
  if(source_encoding=="ebu")
    source_encoding="EBU";
}

void CTranslatingTextSCE::GetParams(xmlNodePtr n)
{
  if(xmlStrEqual(n->name, BAD_CAST "private")) {
    for(xmlNodePtr c=n->children; c; c=c->next){
      if(c->type==XML_ELEMENT_NODE) {
        if(!xmlStrcmp(c->name, BAD_CAST "source_timezone")){
          xmlChar *s = xmlNodeGetContent(c);
          source_timezone = (char*)s;
          xmlFree(s);
        }
        if(!xmlStrcmp(c->name, BAD_CAST "source_encoding")){
          xmlChar *s = xmlNodeGetContent(c);
          source_encoding = (char*)s;
          xmlFree(s);
        }
        if(!xmlStrcmp(c->name, BAD_CAST "destination_timezone")){
          xmlChar *s = xmlNodeGetContent(c);
          destination_timezone = (char*)s;
          xmlFree(s);
        }
        if(xmlStrEqual(c->name, BAD_CAST "translations")){
          for(xmlNodePtr d=c->children; d; d=d->next){
            if(d->type==XML_ELEMENT_NODE) {
              if(!xmlStrcmp(d->name, BAD_CAST "translate")){
                string from, to;
                for(xmlNodePtr e=d->children; e; e=e->next){
                  if(e->type==XML_ELEMENT_NODE) {
                    if(xmlStrEqual(e->name, BAD_CAST "from")){
                      xmlChar *f = xmlNodeGetContent(e);
                      from = (char*)f;
                      xmlFree(f);
                    }
                    if(xmlStrEqual(e->name, BAD_CAST "to")){
                      xmlChar *f = xmlNodeGetContent(e);
                      to = (char*)f;
                      xmlFree(f);
                    }
                  }
                }
                if(from.length()>0 && to.length()>0)
                  translations[from] = to;
              }
            }
          }
        }
      }
    }
  }
}

void CTranslatingTextSCE::PutPrivateParams(xmlTextWriterPtr writer)
{
  if(source_timezone.size()>0 || destination_timezone.size()>0 || translations.size()>0) {
    xmlTextWriterStartElement(writer, BAD_CAST "private");
    if(source_encoding.size()>0) {
      PutString(writer, "source_encoding", source_encoding);
    }
    if(source_timezone.size()>0 || destination_timezone.size()>0) {
      PutString(writer, "source_timezone", source_timezone);
      PutString(writer, "destination_timezone", destination_timezone);
    }
    if(translations.size()>0) {
      xmlTextWriterStartElement(writer, BAD_CAST "translations");
      for(map<string,string>::iterator i=translations.begin(); i!=translations.end(); i++) {
        xmlTextWriterStartElement(writer, BAD_CAST "translate");
        PutString(writer, "from", i->first);
        PutString(writer, "to", i->second);
        xmlTextWriterEndElement(writer);
      }
      xmlTextWriterEndElement(writer);
    }
    xmlTextWriterEndElement(writer);
  }
}

//"Display",0x00000002,"(GMT-06:00) Mexico City"
//"Dlt",0x00000002,"Mexico Daylight Time"
//"Std",0x00000002,"Mexico Standard Time"
//"MapID",0x00000002,"-1,85"
//"Index",0x00010001,30
//"TZI",0x00030001,

#ifdef WIN32
const char* CTranslatingTextSCE::tzkey
   = "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Time Zones\\";

void CTranslatingTextSCE::get_local_time(
  const string& zone, const FILETIME& from, FILETIME& to
)
{
  HKEY hTz;
  char key[1024];
  sprintf(key, "%s%s", tzkey, zone.c_str());
  TIME_ZONE_INFORMATION t_z_i;
  LONG r;
  memset(&t_z_i, 0, sizeof(t_z_i));
  r = RegOpenKeyEx( HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hTz);
  if(r == ERROR_SUCCESS) {
    DWORD type, len;
    TZI tzi;
    memset(&tzi, 0, sizeof(tzi));
    r=RegQueryValueEx(hTz, "TZI", NULL, &type, (BYTE*)&tzi, &len);
    if(r==ERROR_SUCCESS) {
      t_z_i.Bias=tzi.Bias;
      t_z_i.DaylightBias=tzi.DaylightBias;
      t_z_i.DaylightDate=tzi.DaylightDate;
      t_z_i.StandardBias=tzi.StandardBias;
      t_z_i.StandardDate=tzi.StandardDate;
      t_z_i.StandardName[0]=0;
      t_z_i.DaylightName[0]=0;
    }
    RegCloseKey(hTz);
  } else {
    // we will assume UTC
    SYSTEMTIME s = {0,0,0,0,0,0,0,0};
    t_z_i.Bias=0;
    t_z_i.DaylightBias=0;
    t_z_i.DaylightDate=s;
    t_z_i.StandardBias=0;
    t_z_i.StandardDate=s;
    t_z_i.StandardName[0]=0;
    t_z_i.DaylightName[0]=0;
  }
  SYSTEMTIME in, out;
  FileTimeToSystemTime(&from, &in);
  SystemTimeToTzSpecificLocalTime(&t_z_i, const_cast<LPSYSTEMTIME>(&in), &out);
  SystemTimeToFileTime(&out, &to);
}
#else
void CTranslatingTextSCE::get_local_time(
  const string& tz, time_t from, time_t& to
)
{
  char *platform_tz;
  platform_tz = getenv("TZ");
  //cout << " platform TZ is " << platform_tz << endl;
  setenv("TZ", tz.c_str(), 1);
  tzset();
  struct tm* tmp = localtime(&from);
  if(tmp)
    to = mktime(tmp);
  if (platform_tz)
    setenv("TZ", platform_tz, 1);
  else
    unsetenv("TZ");
  tzset();
}
#endif

int CTranslatingTextSCE::get_tz_offset()
{
#ifdef WIN32
  FILETIME now, from, to;
  GetSystemTimeAsFileTime(&now);
  get_local_time(source_timezone, now, from);
  get_local_time(destination_timezone, now, to);
  uint64_t t = *reinterpret_cast<uint64_t*>(&to);
  uint64_t f = *reinterpret_cast<uint64_t*>(&from);
  return static_cast<int>((f-t)/10000000ULL);
#else
  char *platform_tz;
  time_t t=time(NULL);
  int offset=0;
  platform_tz = getenv("TZ");
  setenv("TZ", source_timezone.c_str(), 1);
  tzset();
  struct tm* tmp;
  tmp = localtime(&t);
  if(tmp) {
    offset = tmp->tm_gmtoff;
    //cout << source_timezone << " offset " << tmp->tm_gmtoff << endl;
  }
  setenv("TZ", destination_timezone.c_str(), 1);
  tzset();
  tmp = localtime(&t);
  if(tmp) {
    offset += tmp->tm_gmtoff;
    //cout << destination_timezone << " offset " << tmp->tm_gmtoff << endl;
  }
  if (platform_tz)
    setenv("TZ", platform_tz, 1);
  else
    unsetenv("TZ");
  tzset();
  return offset;
#endif
}

void CTranslatingTextSCE::map_tz(time_t time, time_t offset, map<string,string> &times)
{
  char good[12], bad[12];
  struct tm *a, bdt = { 0,0,0,0,0,0,0,0,0 };
  time_t t;
  t = time+86400; // make sure we can subtract up to 24 hours
  a = gmtime(&t);
  if(a)
    bdt=*a;
  const char *ampm;
  int h,h24 = bdt.tm_hour, m = bdt.tm_min;
  if(h24>12) {
    ampm = "pm";
    h = h24 - 12;
  } else if(h24==12) { /* concensus is that 12pm is noon */
    ampm = "pm";
    h = h24;
  } else {
    ampm = "am";
    h = h24;
  }
  if(bdt.tm_min==0)
    sprintf(bad, " %d%s", h, ampm);
  else
    sprintf(bad, " %d.%02u%s", h, m, ampm);
  t -= offset;
  a = gmtime(&t);
  if(a)
    bdt=*a;
  strftime(good, sizeof(good), " %H:%M", &bdt);
  times[bad]=good;
  /* cope with duffers who put 16:30pm */
  if(h24>12) {
	if(bdt.tm_min==0)
	  sprintf(bad, " %d%s", h24, ampm);
	else
	  sprintf(bad, " %d.%02u%s", h24, m, ampm);
    times[bad]=good;
  }
}

static bool compare_by_len(
 const pair<string,string>& a,
 const pair<string,string>& b
)
{
  int la = a.first.length();
  int lb = b.first.length();
  return la<lb;
}

void CTranslatingTextSCE::init_internationalisation()
{
  tz_offset = get_tz_offset();
  lowtimes.clear();
  for(int i=0; i<10; i++){
    map_tz(60*60*i, tz_offset, lowtimes);
    map_tz(60*(60*i+30), tz_offset, lowtimes);
  }
  hightimes.clear();
  for(int i=10; i<24; i++){
    map_tz(60*60*i, tz_offset, hightimes);
    map_tz(60*(60*i+30), tz_offset, hightimes);
  }
  // maps are sorted which causes problems when some strings to replace are
  // a substring of others, so we use a list and sort by length
  sorted_translations.clear();
  for(map<string,string>::iterator i=lowtimes.begin(); i!=lowtimes.end(); i++)
    sorted_translations.push_back(*i);
  for(map<string,string>::iterator i=hightimes.begin(); i!=hightimes.end(); i++)
    sorted_translations.push_back(*i);
  for(map<string,string>::iterator i=translations.begin(); i!=translations.end(); i++)
    sorted_translations.push_back(*i);
  sorted_translations.sort(compare_by_len);
#if 0
  for(list< pair<string,string> >::iterator i=sorted_translations.begin();
       i!=sorted_translations.end(); i++)
    cout << "map " << i->first << " to " << i->second << ", len " << i->first.length() << endl;
#endif
}

string CTranslatingTextSCE::internationalise(const string& in)
{
  string out = in;
  int t = get_tz_offset();
  if(t!=tz_offset) {
    cout << "changing time zone offset to " << t << "s" << endl;
    init_internationalisation();
  }
  if(source_encoding == "") {
    out = in;
  } else {
    if(source_encoding == "EBU") {
	  out.resize(0);
      for(size_t i=0; i<in.length(); i++)
      {
	    char s[8];
	    size_t n = wctomb(s, ebu2unicode[int(in[i])]);
	    if(n>0)
	    {
	      s[n]=0;
	      out += s;
	    }
      }
    } else if(source_encoding == ""){
      out = in;
    } else {
	  const size_t max=256;
	  wchar_t ws[max];
      char* r = setlocale(LC_ALL, source_encoding.c_str());
      if(r==NULL)
        setlocale(LC_ALL, "C"); // default
	  size_t len = mbstowcs(ws, in.c_str(), max);
	  if(len == size_t(-1))
	    throw "bad locale";
      setlocale(LC_ALL, "en_GB.UTF-8");
	  out.resize(0);
      for(size_t i=0; i<len; i++)
      {
	    char s[8];
	    size_t n = wctomb(s, ws[i]);
	    if(n>0)
	    {
	      s[n]=0;
	      out += s;
	    }
      }
    }
  }
  // do the translations in the utf-8 domain
  for(list< pair<string,string> >::iterator i=sorted_translations.begin();
       i!=sorted_translations.end(); i++)
    replaceall(out, i->first, i->second);
  return out;
}

void CTranslatingTextSCE::replaceall(string& s, const string& bad, const string& good)
{
  while(true) {
    size_t loc = s.find( bad, 0 );
    if( loc != string::npos ) {
      s.erase(loc, bad.length());
      s.insert( loc, good );
    } else {
      return;
    }
  }
}


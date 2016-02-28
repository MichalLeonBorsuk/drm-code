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

#include "TimedTextSCE.h"
#include <unistd.h>

using namespace std;

CTimedTextSCE::CTimedTextSCE():CTranslatingTextSCE(),message(),next()
{
}

CTimedTextSCE::CTimedTextSCE(const CTimedTextSCE& e):CTranslatingTextSCE(e),message(e.message),next(message.begin())
{
}

CTimedTextSCE::~CTimedTextSCE()
{
}

CTimedTextSCE& CTimedTextSCE::operator=(const CTimedTextSCE& e)
{
  CTranslatingTextSCE(*this) = e;
  message = e.message; 
  next = message.begin();
  return *this;
}


string CTimedTextSCE::next_message()
{
  time_t now = time(NULL);
  if(next == message.end())
    next = message.begin();
  if(next == message.end())
    return ""; // empty!!!
  time_t next_sec = last + next->offset.tv_sec;
  if(next_sec > now)
    sleep(next_sec - now);
  string text = next->text;
  next++;
  last = now;
  cout << text << endl;
  return text;
}

void CTimedTextSCE::ReConfigure(const ServiceComponent& config)
{
  cerr << "CTimedTextSCE::ReConfigure" << endl;
  CTranslatingTextSCE::ReConfigure(config);
  xmlDocPtr text_doc = xmlParseFile(current.source_selector.c_str());
  if(text_doc == NULL) {
    throw string("bad timed text file") + current.source_selector;
  }
  xmlNodePtr n;
  for (n = text_doc->children; n->type != XML_ELEMENT_NODE; n = n->next);
  for (n = n->children; n; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (xmlStrEqual (n->name, BAD_CAST "TextSample"))
      {
	    timed_text m;
        xmlChar *s = xmlGetProp(n, BAD_CAST "sampleTime");
        if(s) {
		  char *st = (char*)s;
		  m.offset.tv_sec = (atoi(st)*60+atoi(&st[3]))*60+atoi(&st[6]);
		  m.offset.tv_nsec = atoi(&st[9])*1000000UL;
          xmlFree(s);
        }
        s = xmlGetProp(n, BAD_CAST "text");
        if(s) {
          m.text = (char*)s;
          xmlFree(s);
        }
		if(m.text[0]== '\'' && m.text[m.text.length()-1] == '\'')
		  m.text = m.text.substr(1, m.text.length()-2);
		message.push_back(m);
      }
    }
  }
  xmlFreeDoc(text_doc);
  next = message.begin();
  last = time(NULL);
}

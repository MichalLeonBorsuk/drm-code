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

#include "Schedule.h"

Schedule::Schedule():Persist(),interval()
{
  clearConfig();
    tag="schedule";
}

Schedule::Schedule(const Schedule& s):Persist(s),interval(s.interval)
{
}

Schedule& Schedule::operator=(const Schedule& s)
{
    Persist(*this) = s;
    interval = s.interval;
    return *this;
}

void Schedule::clearConfig()
{
  interval.clear();
}

Schedule::~Schedule()
{
  clearConfig();
}

void Schedule::GetParams(xmlNodePtr c)
{
      if(xmlStrEqual(c->name, BAD_CAST "interval")) {
        Interval v;
        for(xmlNodePtr d=c->children; d; d=d->next){
          if(d->type==XML_ELEMENT_NODE){
            if(xmlStrEqual(d->name, BAD_CAST "days")) {
              xmlChar *dc = xmlNodeGetContent(d);
              v.days = (char*)dc;
              xmlFree(dc);
            }
            if(xmlStrEqual(d->name, BAD_CAST "start_time")) {
              xmlChar *start = xmlNodeGetContent(d);
              sscanf((char*)start, "%u:%u", &v.start_hour, &v.start_minute);
	       xmlFree(start);
            }
            parseUnsigned(d, "duration", &v.duration);
          }
        }
        interval.push_back(v);
      }
}

void Schedule::ReConfigure(xmlNodePtr config)
{
  misconfiguration=false;
  Persist::ReConfigure(config);
  if(interval.size()>15)
    misconfiguration=true;
}

void Schedule::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  for(size_t i=0; i<interval.size(); i++) {
    xmlTextWriterStartElement(writer, BAD_CAST "interval");
	PutString(writer, "days", interval[i].days);
    xmlTextWriterStartElement(writer, BAD_CAST "start_time");
	xmlTextWriterWriteFormatString(writer, "%02u:%02u",
              interval[i].start_hour, interval[i].start_minute);
    xmlTextWriterEndElement(writer);
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteString(writer, BAD_CAST " duration is in minutes ");
    xmlTextWriterEndComment(writer);
    PutUnsigned(writer, "duration", interval[i].duration);    
    xmlTextWriterEndElement(writer);
  }
}

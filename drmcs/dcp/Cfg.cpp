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

#include "Cfg.h"

Cfg::Cfg():Persist(), config_file(), m_stdout(), m_stderr(),
  utco(-1), max_frames(-1), initial_reconfiguration_index(-1)
{
  tag="drm";
}

void Cfg::GetParams(xmlNodePtr n)
{
  xmlChar *s = xmlNodeGetContent(n);
  string v;
  if(s) {
    v = (char*)s;
    xmlFree(s);
  }
  parseUnsigned(n, "utco", &utco);
  parseUnsigned(n, "max_frames", &max_frames);
  parseUnsigned(n, "initial_reconfiguration_index", &initial_reconfiguration_index);
  if(xmlStrEqual(n->name, BAD_CAST "main_config")) {
    config_file = v;
  }
  if(xmlStrEqual(n->name, BAD_CAST "stdout")) {
     m_stdout = v;
     freopen(m_stdout.c_str(), "a", stdout);
  }
  if(xmlStrEqual(n->name, BAD_CAST "stderr")) {
     m_stderr = v;
     freopen(m_stderr.c_str(), "a", stderr);
  }
}

void Cfg::PutParams(xmlTextWriterPtr writer)
{
  Persist::PutParams(writer);
  PutString(writer, "main_config", config_file.c_str());
  if(m_stdout.length()>0)
    PutString(writer, "stdout", m_stdout);
  if(m_stderr.length()>0)
    PutString(writer, "stderr", m_stderr);
  if(utco != -1)
    PutUnsigned(writer, "utco", utco);
  if(max_frames != -1)
    PutUnsigned(writer, "max_frames", max_frames);
  if(initial_reconfiguration_index != -1)
    PutUnsigned(writer, "initial_reconfiguration_index", initial_reconfiguration_index);
}

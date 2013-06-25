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

#include "StreamMux.h"
#include <iostream>
using namespace std;

StreamMux::StreamMux():sce(),factory(),current()
{
}

StreamMux::StreamMux(const StreamMux& s):sce(s.sce),factory(s.factory),current(s.current)
{
}

StreamMux& StreamMux::operator=(const StreamMux& s)
{
  sce = s.sce;
  factory = s.factory;
  current = s.current;
  return *this;
}

StreamMux::~StreamMux()
{
  cout << "StreamMux destructor" << endl;
  for(size_t i=0; i<sce.size(); i++)
    delete sce[i];
}

void StreamMux::ReConfigure(const Stream& config)
{
  cout << "StreamMux::ReConfigure" << endl; cout.flush();
  size_t new_num_components = config.component.size();
  size_t old_num_components = current.component.size();
  // delete any components no longer in use
  if(new_num_components < old_num_components) {
    for(size_t i=new_num_components; i<old_num_components; i++) {
      delete sce[i];
    }
    sce.resize(new_num_components);
  }
  // update changed components
  for(size_t i=0; i<old_num_components; i++) {
    const ServiceComponent& comp = config.component[i];
    if(current.component[i].implementor != comp.implementor) {
      if(sce[i])
        delete sce[i];
      sce[i] = factory.create(comp);
    }
  }
  // add any new components
  for(size_t i=old_num_components; i<new_num_components; i++) {
    const ServiceComponent& comp = config.component[i];
    ServiceComponentEncoder* s = factory.create(comp);
    sce.push_back(s);
  }
  current = config;
  for(size_t i=0; i<sce.size(); i++) {
    sce[i]->ReConfigure(current.component[i]);
  }
}

void StreamMux::NextFrame(bytevector& out)
{
}

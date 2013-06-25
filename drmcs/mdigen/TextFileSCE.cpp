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

#include "TextFileSCE.h"
#include <iostream>
using namespace std;

void TextFileSCE::ReConfigure(const ServiceComponent& config)
{
  CTranslatingTextSCE::ReConfigure(config);
  if(file.is_open())
    file.close();
  file.open(current.source_selector.c_str());
  if(!file.is_open()) {
    cerr << "can't open " << current.source_selector << " as a text message file" << endl;
  }
}

string TextFileSCE::next_message()
{
  string message;

  while(file.is_open()) {

    getline(file, message);

    if(message.length()==0) {
    // loop file
      if(file.eof()) {
        file.clear();
        file.close();
        file.open(current.source_selector.c_str());
        if(!file.is_open())
          return ""; // someone stole out file! - no text to get
      }
    } else {
      break;
    }
  }
  for(size_t i=0; i<message.length(); i++) {
    if(message[i]=='^')
        message[i]=0x0b;
    if(message[i]=='\n') {
      message.erase(i);
      break;
    }
  }
  return message;
}


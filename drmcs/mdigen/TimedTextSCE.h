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

#ifndef _TIMEDTEXTSCE_H
#define _TIMEDTEXTSCE_H

#include "TranslatingTextSCE.h"
#include <time.h>
#include <vector>

using namespace std;

class CTimedTextSCE : public CTranslatingTextSCE
{
public:

  CTimedTextSCE();
  CTimedTextSCE(const CTimedTextSCE& e);
  CTimedTextSCE& operator=(const CTimedTextSCE& e);
  virtual ~CTimedTextSCE();
  virtual void ReConfigure(const ServiceComponent&);
  virtual string next_message();

protected:
  struct timed_text
  {
  	timespec offset;
	string text;
  };
  vector< timed_text > message;
  vector< timed_text >::iterator next;
  time_t last;
};

#endif

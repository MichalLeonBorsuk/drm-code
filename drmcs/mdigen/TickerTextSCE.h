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

#ifndef _TICKERTEXTSCE_H
#define _TICKERTEXTSCE_H

#include <platform.h>
#include <TranslatingTextSCE.h>
#include "Ticker.h"
#include "sockets.h"
#include <queue>
#include <map>
#include <vector>

using namespace std;

class CTickerTextSCE : public CTranslatingTextSCE
{
public:

  CTickerTextSCE();
  CTickerTextSCE(const CTickerTextSCE&);
  CTickerTextSCE& operator=(const CTickerTextSCE&);

  void clearConfig();
  virtual void ReConfigure(const ServiceComponent& config);
  virtual string next_message();

protected:

  void parse_source_selector(const string&);
  bool open();
  void check_source();
  void wait_for_data();
  void init_source();
	
	
    vector<string> hostname;
    vector<int> port;
    int host;
    time_t timer;
    bool need_clear;

    enum states {init, ready} state;

	class CMyTicker : public CTicker
    {
    public:
      string channel_name;
      int channel;
      queue<string> texts;

      virtual ~CMyTicker() {}
      virtual void added(const char *name, int id);
      virtual void deleted(int id);
      virtual void changed(int id, const char *text);
      virtual void closed();

      client_socket socket;
    };
    
    CMyTicker ticker;	
    map<string,string> socket_config;

};
#endif

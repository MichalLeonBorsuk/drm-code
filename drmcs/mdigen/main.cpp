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
#ifdef WIN32
# define CONFIG_FILE "mdigen.conf"
#else
# define CONFIG_FILE "/etc/drm/mdigen.conf"
#endif

#include <sys/stat.h>
#include <iostream>
#include <csignal>
#ifndef WIN32
#include <fam.h>
#endif
#include "mdigenerator.h"
#include "sdixml/version.h"
#include "DcpIn.h"

static bool reconfiguration_requested = false;

#ifndef WIN32
static void
signal_handler(int sig, siginfo_t *si, void *unused)
{
  cout << "Got SIGHUP" << endl; cout.flush();
  reconfiguration_requested = true;
}
#endif

extern const char* svn_version();

void readCfg(Cfg& config, const string& cfg)
{
  xmlDocPtr config_doc = xmlParseFile(cfg.c_str());
  if(config_doc == NULL) {
    throw string("missing configuration file ")+cfg;
  }
  config.ReConfigure(config_doc->children);
  xmlFreeDoc(config_doc);
}

/* TODO (jfbc#1#): Check the platforms time sync */
/* TODO (#1#): Add support to be a daemon on *nix systems */
/* TODO (#1#): log if overflow or underflow */
/* TODO (#1#): auto leap second */
/* TODO (#1#): calculate UTCO */


void ReConfigure(Mdigen& mdigen, const string& cfg, bool& ok, int& max_frames)
{
  Cfg config;
  try {
    readCfg(config, cfg);
  }
  catch(const char* e)
  {
    cerr << e << endl; cerr.flush();
    ok = false;
  }
  catch(string e)
  {
    cerr << e << endl; cerr.flush();
    ok = false;
  }
  try {
      mdigen.utco = config.utco;
	  if(config.initial_reconfiguration_index == -1)
	    mdigen.initial_reconfiguration_index = 7; // needed if changes during run - can't rely on constructor
	  else
	    mdigen.initial_reconfiguration_index = config.initial_reconfiguration_index;
      mdigen.ReConfigure(config.config_file);
      max_frames = config.max_frames;
  }
  catch(string e){
    cerr << e << endl; cerr.flush();
    ok = false;
  }
  catch(char const* e) {
    cerr << e << endl; cerr.flush();
    ok = false;
  }
  reconfiguration_requested = false;
}

int main(int argc, char **argv)
{
  vector<string> cfg;
  size_t cfg_num = 0;
  if(argc==2) {
    cfg.push_back(argv[1]);
  } else if(argc==3) {
    cfg.push_back(argv[1]);
    cfg.push_back(argv[2]);
  } else {
    cfg.push_back(CONFIG_FILE);
  }
  time_t mtime;
  struct stat s;
  if (stat(cfg[cfg_num].c_str(), &s) == 0) {
    mtime = s.st_mtime;
  } else {
    mtime = 0;
  }
  cout << "BBC MDI Generator Rev " << svn_version()
       << ", using libsdixml Rev " << libsdixml::svn_version() << endl
       << "Implementing SDI Schema version " << libsdixml::SDI_Schema_version()
       << endl << "Reading config from " << cfg[cfg_num] << endl; cout.flush();
  Mdigen mdigen;
  /*
  DcpIn dcpin;
  dcpin.ReConfigure("dcp.udp://:9998");
  tagpacketlist t;
  dcpin.getFrame(t);
  for(tagpacketlist::iterator i=t.begin(); i!=t.end(); i++)
    cout << i->first << " " << i->second.size() << endl;
  */
#ifdef WIN32
 int iResult;
 WSADATA wsaData;
 // Initialize Winsock

 iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
 if (iResult != 0) {
    printf("WSAStartup failed: %d\n", iResult);
    exit(1);
 }
#else
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = signal_handler;
  if (sigaction(SIGHUP, &sa, NULL) == -1)
  {
  	cerr << "error assigning signal handler" << endl;
	exit(1);
  }
  FAMConnection fc;
  if(FAMOpen(&fc)==0)
  {
    FAMRequest fr;
	if(cfg[0][0]!='/')
	{
			string path = getenv("PWD");
			cerr << path << endl;
			cfg[0] = path + "/" + cfg[0];
	}
    if(FAMMonitorFile(&fc, cfg[0].c_str(), &fr, NULL))
      cerr << "can't monitor " << cfg[0] << endl;
    else
      cout << "FAM Monitoring " << cfg[0] << endl;
  } else {
      cerr << "can't connect to file alteration monitor " << endl;
  }

#endif
  bool ok = true;
  int max_frames = -1;
  int reconf_interval = 32;
  int reconf = reconf_interval;
  ReConfigure(mdigen, cfg[cfg_num], ok, max_frames);
  while(ok) {
    try {
      mdigen.eachframe();
	  //cout << "Frame: " << mdigen.transmitted_frames << endl;
#ifndef WIN32
  while(FAMPending(&fc))
  {
    FAMEvent fe;
    FAMNextEvent(&fc, &fe);
	switch(fe.code)
	{
	case FAMDeleted:
	  break;
    case FAMChanged:
          cout << "file alteration monitor detected config change" << endl;
          reconfiguration_requested = true;
	  break;
    case FAMCreated:
    case FAMExists:
	  break;
    case FAMEndExist:
	  cout << "FAM initialised " << fe.filename << endl;
	  break;
    case FAMAcknowledge:
	  cout << "FAM cancel acknowledged " << fe.filename << endl;
	  break;
    case FAMStartExecuting:
    case FAMStopExecuting:
    case FAMMoved:
	  cout << "unexpected fam event " << fe.code << " '" << fe.filename << "'" << endl;
	  break;
    default:
	  cout << "unknown fam event " << fe.code << " '" << fe.filename << "'" << endl;
}
}
#endif
    }
    catch(char const* e) {
      cerr << e << endl; cerr.flush();
    }
    if(reconfiguration_requested) {
      ReConfigure(mdigen, cfg[cfg_num], ok, max_frames);
    }
	if(cfg.size()>1)
	{
	  reconf--;
	  if(reconf==0)
	  {
	    cfg_num = 1 - cfg_num;
        ReConfigure(mdigen, cfg[cfg_num], ok, max_frames);
		reconf = reconf_interval;
	  }
	}
    if(max_frames!=-1 && mdigen.transmitted_frames > max_frames)
      break;
  }
}

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

#include "MonitoredFile.h"
#include <unistd.h>
#include <fcntl.h>
#include <iostream>

using namespace std;

bool MonitoredFile::monitor(const string& path)
{
  this->path = path;
#ifndef WIN32
  if(FAMOpen(&fc)==0)
  {
    FAMRequest fr;
    if(FAMMonitorFile(&fc, path.c_str(), &fr, NULL)==0)
    {
      cout << "FAM Monitoring " << path << endl;
      return true;
    }
    else
    {
      cerr << "can't monitor " << path << endl;
    }
  }
  else
  {
      cerr << "can't connect to file alteration monitor " << endl;
  }
#endif
  return false;
}

bool MonitoredFile::open(int mode)
{
  fd = ::open(path.c_str(), mode);
  if(fd>0)
  {
#ifndef WIN32
      int r = lockf(fd, F_TLOCK, 0);
      if(r<0)
      {
          ::close(fd);
          fd=-1;
          return false;
      }
#endif
      return true;
  }
  return false;
}

void MonitoredFile::ignore()
{
#ifndef WIN32
  if(FAMCONNECTION_GETFD(&fc)>0)
  {
    (void)FAMClose(&fc);
  }
#endif
}

void MonitoredFile::close()
{
  if(fd>0)
  {
#ifndef WIN32
      lockf(fd, F_ULOCK, 0);
#endif
      ::close(fd);
  }
}

bool MonitoredFile::changed()
{
#ifdef WIN32
static bool first=true;
if(first) { first=false;
  return true;}
#else
  while(FAMPending(&fc))
  {
    FAMEvent fe;
    FAMNextEvent(&fc, &fe);
	switch(fe.code)
	{
	case FAMDeleted:
	  break;
    case FAMChanged:
          return true;
	  break;
    case FAMCreated:
    case FAMExists:
          return true;
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
  return false;
}

size_t MonitoredFile::read(void* buf, size_t bytes)
{
    return ::read(fd, buf, bytes);
}

size_t MonitoredFile::write(const void* buf, size_t bytes)
{
    return ::write(fd, buf, bytes);
}

size_t MonitoredFile::seek(size_t bytes, int from_where)
{
    return ::lseek(fd, bytes, from_where);
}

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

#ifndef _MONITORED_FILE_H
#define _MONITORED_FILE_H

#include <string>
#include <fam.h>

  /*
   * works just like an old fashioned file, but can be polled for change and
   * only opens if a lock can be obtained
   */

class MonitoredFile
{
public:
    MonitoredFile():fd(-1)
#ifndef WIN32
    ,fc()
#endif
    {}

    bool monitor(const std::string& path);
    void ignore();
    bool changed();

    bool open(int mode);
    void close();
    size_t read(void* buf, size_t bytes);
    size_t write(const void* buf, size_t bytes);
    size_t seek(size_t bytes, int from_where);

protected:
  int fd;
  std::string path;
#ifndef WIN32
  FAMConnection fc;
#endif

};

#endif

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

#ifndef WIN32
#include <stropts.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <errno.h>
#endif

#include <cstring>
#include "Ticker.h"
#include "wirelist.h"

void
CTicker::request_channels (SOCKET s, int services, int *servicelist)
{
  if (services == 0)
    {
      const char *argv[2];
      argv[0] = "Listen";
      argv[1] = "*";
      encode_strings (s, 2, argv);
    }
  else
    {
      encode_string_ints (s, "Listen", services, servicelist);
    }
  send (s, "\n", 1, 0);
#ifndef WIN32
  ioctl (s, I_FLUSH, FLUSHW);
#endif
}

void
CTicker::added (const char *, int)
{

}

void
CTicker::deleted (int)
{

}

void
CTicker::changed (int, const char *)
{

}

void
CTicker::closed ()
{

}

/* robust */

/* take the structure returned from the ticker host read functions and
 * call the cache functions to update the cache with the received data
 * change the service text field to the ETSI standard form, which is not
 * pure printable text
 * quit on any error
 */

/* get new data from the ticker host and update the cache
 * stop on any error but we can try again next time
 */

void
CTicker::check_for_input (SOCKET s)
{
  NDAT *ldat = decode_data (s);
  NDAT **data;
  int i;
  if (ldat == NULL)
    {
      closed ();
      goto exit;
    }
  if (ldat->type != 'l')
    goto exit;
  data = ldat->values;
  if (data == NULL || data[0] == NULL)
    goto exit;
  if (data[0]->type != 's')
    goto exit;
  if (data[0]->sval == NULL)
    goto exit;
  if (strcmp (data[0]->sval, "Added") == 0)
    {
      NDAT **keys;
      NDAT **vals;
      if (data[1] == NULL)
	goto exit;
      keys = data[1]->keys;
      vals = data[1]->values;
      if (keys == NULL || vals == NULL)
	goto exit;
      for (i = 0; i < data[1]->ival; i++)
	{
	  added (vals[i]->sval, keys[i]->ival);
	}
    }
  else if (strcmp (data[0]->sval, "Removed") == 0)
    {
      NDAT **items;
      if (data[1] == NULL)
	goto exit;
      items = data[1]->values;
      if (items == NULL)
	goto exit;
      for (i = 0; i < data[1]->ival; i++)
	{
	  deleted (items[i]->ival);
	}
    }
  else if (strcmp (data[0]->sval, "Changed") == 0)
    {
      NDAT **keys, **vals;
      if (data[1] == NULL)
	goto exit;
      keys = data[1]->keys;
      vals = data[1]->values;
      if (keys == NULL || vals == NULL)
	goto exit;
      for (i = 0; i < data[1]->ival; i++)
	{
	  int j;
	  NDAT **ckeys, **cvals;
	  if (vals[i] == NULL)
	    goto exit;
	  ckeys = vals[i]->keys;
	  cvals = vals[i]->values;
	  if (ckeys == NULL || cvals == NULL)
	    goto exit;
	  for (j = 0; j < data[1]->ival; j++)
	    {
	      if (ckeys[j] && ckeys[j]->sval
		  && (strcmp (ckeys[j]->sval, "Text") == 0))
		{
		  changed (keys[i]->ival, cvals[j]->sval);
		}
	    }
	}
    }
exit:
  free_data (ldat);
}

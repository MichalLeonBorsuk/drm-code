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

#ifdef __cplusplus
extern "C" {
#endif

#include "platform.h"

int write_netstring(SOCKET s, const char *data, size_t len);
int read_netstring(SOCKET s, char *data, int max);

int encode_int(SOCKET s, int data);
int encode_null(SOCKET s);
int encode_string(SOCKET s, const char *data);
int encode_ints(SOCKET s, int argc, const int *argv);
int encode_strings(SOCKET s, int argc, const char **argv);
int encode_string_int(SOCKET s, char *sval, int ival);
int encode_string_ints(SOCKET s, const char *sval, int argc, const int *argv);

extern int debug;

typedef struct ndat {
	int type;
	int ival;
	char *sval;
      struct ndat **values;
      struct ndat **keys;
} NDAT;

NDAT* decode_data(SOCKET s);
void free_data(NDAT *r);
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

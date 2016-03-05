/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: wirelist.h 472 2009-05-28 17:18:14Z julianc $
*
* Copyright (C) British Broadcasting Corporation 2006.
*
* All Rights Reserved.
*
* Contributor(s): Julian Cable, John Elliot, Ollie Haffenden, Andrew Murphy
*
* ***** END LICENSE BLOCK ***** */

#ifdef __cplusplus
extern "C" {
#endif

int write_netstring(int s, const char *data, size_t len);
int read_netstring(int s, char *data, int max);

int encode_int(int s, int data);
int encode_null(int s);
int encode_string(int s, const char *data);
int encode_ints(int s, int argc, const int *argv);
int encode_strings(int s, int argc, const char **argv);
int encode_string_int(int s, char *sval, int ival);
int encode_string_ints(int s, const char *sval, int argc, const int *argv);

extern int debug;

typedef struct ndat {
	int type;
	int ival;
	char *sval;
      struct ndat **values;
      struct ndat **keys;
} NDAT;

NDAT* decode_data(int s);
void free_data(NDAT *r);
#ifdef __cplusplus
} /* closing brace for extern "C" */
#endif

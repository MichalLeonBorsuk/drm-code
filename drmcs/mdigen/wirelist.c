/* ***** BEGIN LICENSE BLOCK *****
*
* $Id: wirelist.c 472 2009-05-28 17:18:14Z julianc $
*
* Copyright (C) British Broadcasting Corporation 2006.
*
* All Rights Reserved.
*
* Contributor(s): Julian Cable, John Elliot, Ollie Haffenden, Andrew Murphy
*
* ***** END LICENSE BLOCK ***** */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <stropts.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif
#include "wirelist.h"
#include "platform.h"

int debug;
/* robust */
int write_netstring(SOCKET s, const char *data, size_t len)
{
    if(data) {
        char h[16];
        _WSPIAPI_SPRINTF_S_1(h, 16, "%d:" , len);
        send(s, h, (int)strlen(h), 0);
        send(s, data, (int)strlen(data), 0);
        return send(s, ",", 1, 0);
    }
    else {
        if (debug) {
    	        fprintf(stderr, "null string pointer in write_netstring\n");
                fflush(stderr);
        }
    	return -1;	
    }   	
}

/* robust */
int read_netstring(SOCKET s, char *data, int max)
{
    int rr, n, length, x, l;
    char buf[1];
    length = 0;
    rr=recv(s, buf, 1, 0);
    if(rr==0) {
       return 0;
    }
    if(rr==-1) {
       if(debug)
         perror("read_netstring");
       return 0;
    }
    while ('0' <= buf[0] && buf[0] <= '9') {
        length = (length * 10) + buf[0] - '0';
        if(recv(s, buf, 1, 0)==-1) {
           if(debug)
             perror("read_netstring");
           return 0;
        }
    }
    if (length > max) {
        return 0;
    }
    if (buf[0] != ':') {
        return 0;
    }
    l = length;
    x = 0;
    n = 1;
    while (n && l > 0) {
        n = recv(s, &data[x], l, 0);
        l -= n;
	x += n;
    }
    if (l > 0) {
        return 0;
    }
    if(recv(s, buf, 1, 0)==-1) {
       if(debug)
         perror("read_netstring");
       return 0;
    }
    if (buf[0] != ',') {
        return 0;
    }
    data[length]=0;
    return length;
}

/* robust */
int encode_int(SOCKET s, int data)
{
    int n=1;
    char buf[16];
    write_netstring(s, "i", n);
    n = _WSPIAPI_SPRINTF_S_1(buf, 16, "%d", data);
    return write_netstring(s, buf, n);
}

int encode_null(SOCKET s)
{
    size_t n=1;
    return write_netstring(s, "0", n);
}

int encode_string(SOCKET s, const char *data)
{
    if(data) {
        write_netstring(s, "s", 1);
        return write_netstring(s, data, strlen(data));
    }
    else {
        if (debug) {
    	        fprintf(stderr, "null string pointer in encode_string\n");
                fflush(stderr);
        }
        return -1;
    }    
}

int encode_ints(SOCKET s, int argc, const int *argv)
{
    int i, n, err;
    char buf[128];
    if(argv==NULL) {
        if (debug) {
	            fprintf(stderr, "null pointer in encode_ints\n");
	            fflush(stderr);
        }
        return -1;
    }
    n = _WSPIAPI_SPRINTF_S_1(buf, 128, "l%d", argc);
    err=write_netstring(s, buf, n);
    if(err>=0){
        for(i=0; i<argc; i++)
            err=encode_int(s, argv[i]);
    }
    return err;
}

int encode_strings(SOCKET s, int argc, const char **argv)
{
    int i, n, err;
    char buf[64];
    n = _WSPIAPI_SPRINTF_S_1(buf, 64, "l%d", argc);
    err=write_netstring(s, buf, n);
    if(err>=0){
        for(i=0; i<argc; i++)
            encode_string(s, argv[i]);
    }
    return err;
}

int encode_string_int(SOCKET s, char *sval, int ival)
{
    int n;
    char buf[64];
    n = _WSPIAPI_SPRINTF_S_1(buf, 64, "l%d", 2);
    if(write_netstring(s, buf, n)<0)
        return -1;
    if(encode_string(s, sval)<0)
        return -1;
    return encode_int(s, ival);
}

int encode_string_ints(SOCKET s, const char *sval, int argc, const int *argv)
{
    int n;
    char buf[64];
    n = _WSPIAPI_SPRINTF_S_1(buf, 64, "l%d", 2);
    if(write_netstring(s, buf, n)<0)
        return -1;
    if(encode_string(s, sval)<0)
        return -1;
    return encode_ints(s, argc, argv);
}

/*
int encode_dict(SOCKET s, data) {
        write_netstring(writefn, "d%d" % (len(data), ));
        for key, value in data.items() {
            encode_data(writefn, key);
            encode_data(writefn, value);
}
*/

NDAT* decode_data(SOCKET s)
{
    NDAT *r;
    int i, size, n, max;
    char data[1024];
    max = sizeof(data);
    n = read_netstring(s, data, max);
    if(n==0)
        return NULL;
    r = (NDAT *)calloc(1, sizeof(NDAT));
    if(r==NULL)
        return NULL;
    r->type = data[0];
    switch(r->type) {
    case '0':
        return NULL;
    case 'i':
        n = read_netstring(s, data, max);
        if(n==0) {
            free(r);
            return NULL;
        }
        r->ival = atoi(data);
        return r;
    case 's':
        n = read_netstring(s, data, max);
        if(n==0) {
            free(r);
            return NULL;
        }
        r->sval = strdup(data);
        return r;
    case 'l':
        size = atoi(&data[1]);
        r->ival = size;
        r->values = (NDAT **)calloc(size, sizeof(NDAT));
        if(r->values) {
            for(i=0; i<size; i++) {
                r->values[i] = decode_data(s);
            }
        }
        return r;
    case 'd':
        size = atoi(&data[1]);
        r->ival = size;
        r->keys = (NDAT **)calloc(size, sizeof(NDAT));
        r->values = (NDAT **)calloc(size, sizeof(NDAT));
        if(r->values && r->keys) {
            for(i=0; i<size; i++) {
	        r->keys[i] = decode_data(s);
                r->values[i] = decode_data(s);
            }
        }
        return r;
    }
    free(r);
    return NULL;
}

void free_data(NDAT *r)
{
  int i;
  if(r==NULL)
    return;
  switch(r->type) {
  case '0':
      break;
  case 'i':
      break;
  case 's':
      free(r->sval);
      break;
  case 'l':
      for(i=0; i<r->ival; i++) {
          free_data(r->values[i]);
      }
      free(r->values);
      break;
  case 'd':
      for(i=0; i<r->ival; i++) {
          free_data(r->keys[i]);
          free_data(r->values[i]);
      }
      free(r->keys);
      free(r->values);
      break;
  }
  free(r);
}

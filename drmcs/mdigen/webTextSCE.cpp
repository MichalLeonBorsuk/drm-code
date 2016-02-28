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

#include "webTextSCE.h"
#include <iostream>
using namespace std;
#include <unistd.h>
#include <fcntl.h>

static size_t write_callback( void  *ptr,  size_t size, size_t nmemb, void *stream)
{
    char* buffer = (char*)stream;
    size_t n = size*nmemb;
    if(n>0)
        memcpy(buffer, ptr, n);
    for(size_t i=n-1; i>=0 && (buffer[i]=='\n' || buffer[i]=='\r'); i--)
        buffer[i] = 0;
    //cout << "w " << buffer << endl;
    return n;
}

static void* curl_callback(void* arg)
{
    webTextSCE* This = (webTextSCE*)arg;
    while(This->running) {
        if(This->handle==NULL) {
            This->handle = curl_easy_init( );
            curl_easy_setopt(This->handle, CURLOPT_URL, This->current.source_selector.c_str());
            curl_easy_setopt(This->handle, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(This->handle, CURLOPT_WRITEDATA, This->buffer);
        }
        memset(This->buffer, 0, sizeof(This->buffer));
        CURLcode err = curl_easy_perform(This->handle);
        if(err==0)
            This->changed = true;
        else
            cerr << "curl error " << err << endl;
        timespec delay;
        delay.tv_sec = 10;
        delay.tv_nsec = 0;
        nanosleep(&delay, NULL);
    }
    curl_easy_cleanup(This->handle);
    return NULL;
}

webTextSCE::webTextSCE():CTranslatingTextSCE(),
    latin1_message(),utf8_message(),in_message(false)
{
    handle = NULL;
    running = true;
    int err = pthread_create(&thread, NULL, curl_callback, this);
    if(err<0)
        cerr << "can't create thread " << err << endl;
}

webTextSCE::~webTextSCE()
{
    running = false;
    pthread_join(thread, NULL);
}

webTextSCE::webTextSCE(const webTextSCE& p)
    :CTranslatingTextSCE(p),
     latin1_message(),utf8_message(),
     in_message(p.in_message)
{
    handle = NULL;
    running = true;
    int err = pthread_create(&thread, NULL, curl_callback, this);
    if(err<0)
        cerr << "can't create thread " << err << endl;
}

webTextSCE& webTextSCE::operator=(const webTextSCE& e)
{
    *reinterpret_cast<CTranslatingTextSCE*>(this) = e;
    latin1_message = e.latin1_message;
    utf8_message = e.utf8_message;
    in_message = e.in_message;
    handle = NULL;
    running = true;
    int err = pthread_create(&thread, NULL, curl_callback, this);
    if(err<0)
        cerr << "can't create thread " << err << endl;
    return *this;
}

void webTextSCE::ReConfigure(const ServiceComponent& config)
{
    CTranslatingTextSCE::ReConfigure(config);
    changed = false;
    if(handle)
        curl_easy_cleanup(handle);
    handle = NULL;
}

string webTextSCE::next_message()
{
    if(changed) {
        utf8_message = buffer;
        changed = false;
    }
    //cout << utf8_message << endl;
    return utf8_message;
}

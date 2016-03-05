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

#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libxml/xmlwriter.h>
#include <string.h>
#include "persist.h"
#include <iostream>

using namespace std;

const char *Persist::boolvals[]= {"false", "true", NULL};

Persist::Persist():
    tag(),id(),misconfiguration(true),private_config()
{
}

Persist::Persist(const Persist& p):
    tag(p.tag), id(p.id), misconfiguration(p.misconfiguration),
    private_config(p.private_config)
{
}

Persist& Persist::operator=(const Persist& p)
{
    tag = p.tag;
    id = p.id;
    misconfiguration = p.misconfiguration;
    private_config = p.private_config;
    return *this;
}

Persist::~Persist()
{
    clearConfig();
}

void Persist::clearConfig()
{
    id.clear();
    misconfiguration=false;
    private_config.clear();
}

void Persist::walkConfig(xmlNodePtr n)
{
    if(n==NULL) {
        misconfiguration = true;
        cerr << "Persist - no configuration available" << endl;
        return;
    }
    for(xmlNodePtr c=n; c; c=c->next) {
        if(c->type==XML_ELEMENT_NODE) {
            if(xmlStrEqual(c->name, BAD_CAST "private")) {
                // do we need to duplicate this ?
#ifdef NODE
                private_config = c;
                xmlUnlinkNode(c);
#else
                xmlBufferPtr myBuffer=NULL;
                myBuffer=xmlBufferCreate();
                if(myBuffer)
                {
                    xmlBufferEmpty(myBuffer);
                    xmlNodeDump(myBuffer,c->doc,c,0,0);
                    private_config = (char*)xmlBufferContent(myBuffer);
                    xmlBufferFree(myBuffer);
                }
#endif
            } else {
                GetParams(c);
            }
        }
    }
}

void Persist::ReConfigure(xmlNodePtr config)
{
    clearConfig();
    xmlChar *s = xmlGetProp(config, BAD_CAST "id");
    if(s) {
        id = (char*)s;
        xmlFree(s);
    }
    if(config->children)
        walkConfig(config->children);
}

unsigned long Persist::xmlStringToUnsigned(const xmlChar *s)
{
    unsigned long n=0;
    int len=xmlStrlen(s);
    for(int i=0; i<len;) {
        int b=sizeof(int);
        int ch = xmlGetUTF8Char(&s[i], &b);
        i+=b;
        if('0'<=ch && ch<='9')
            n=10*n+(ch-'0');
        else
            break;
    }
    return n;
}

void Persist::parseBase64Binary(xmlNodePtr c , const char *xmltag, uint8_t *data, unsigned *len)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        /* TODO (jfbc#1#): decode base64 */
    }
}

void Persist::parseDouble(xmlNodePtr c, const char *xmltag, double *var)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        char *val = (char*)xmlNodeGetContent(c);
        *var = strtod(val, NULL);
        xmlFree(val);
    }
}

void Persist::parseUnsigned(xmlNodePtr c, const char *xmltag, int *var)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        *var = xmlStringToUnsigned(val);
        xmlFree(val);
    }
}

void Persist::parseUnsignedLong(xmlNodePtr c, const char *xmltag, int long *var)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        *var = xmlStringToUnsigned(val);
    }
}

void Persist::parseSigned(xmlNodePtr c, const char *xmltag, int *var)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        if(val[0]=='-') {
            *var = xmlStringToUnsigned(&val[1]);
            *var = -*var;
        }
        else {
            *var = xmlStringToUnsigned(val);
        }
        xmlFree(val);
    }
}

void Persist::parseEnum(const xmlChar *val, int *var, const char * const vals[])
{
    for(int i=0; vals[i]; i++) {
        if(xmlStrcmp(val,(const xmlChar*)vals[i])==0) {
            *var=i;
            return;
        }
    }
    // maybe its the index
    char *tailptr;
    unsigned long n = strtoul ((char*)val, &tailptr, 10);
    if(tailptr!=(char*)val)
        *var = n;
}

void Persist::parseEnum(xmlNodePtr c, const char *xmltag, int *var, const char * const vals[])
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        parseEnum(val, var, vals);
        xmlFree(val);
    }
}

int Persist::htob(int n)
{
    if('0'<= n && n <='9')
        return n-'0';
    if('a'<= n && n <='f')
        return n-'a'+10;
    if('A'<= n && n <='F')
        return n-'A'+10;
    return -1;
}

void Persist::parseHexBinary(xmlNodePtr c, const char *xmltag, uint8_t **data, unsigned *len)
{
    if(xmlStrEqual(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        if(val) {
            xmlChar* t=val;
            int octets = xmlStrlen(t)/2;
            if(*data)
                delete *data;
            uint8_t *s = new uint8_t[octets];
            for(int i=0; i<octets; i++) {
                s[i] = htob(*t++)<<4;
                s[i]+= htob(*t++);
            }
            *data = s;
            *len = octets;
            xmlFree(val);
        }
    }
}

void Persist::parseHexBinary(xmlNodePtr c, const char *xmltag, vector<uint8_t>& data)
{
    if(xmlStrEqual(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        if(val) {
            xmlChar* t=val;
            int octets = xmlStrlen(t)/2;
            for(int i=0; i<octets; i++) {
                uint8_t b = htob(*t++)<<4;
                b += htob(*t++);
                data.push_back(b);
            }
            xmlFree(val);
        }
    }
}

void Persist::parseBool(xmlNodePtr c, const char *xmltag, int *var)
{
    parseEnum(c, xmltag, var, boolvals);
}

void Persist::parseIPv4Addr(xmlNodePtr c, const char *xmltag, unsigned long *var)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        *var = inet_addr((char*)val);
    }
}

void Persist::parseIDREF(xmlNodePtr c, const char *xmltag, string& s)
{
    if(!xmlStrcmp(c->name,(const xmlChar*)xmltag)) {
        xmlChar *val = xmlNodeGetContent(c);
        char *p = (char*)val;
        while(isspace(*p)) p++;
        char *q = p + strlen(p) - 1;
        while(isspace(*q)) *q-- = 0;
        s = p;
        xmlFree(val);
    }
}

xmlNodePtr Persist::find(xmlNodePtr config, const char* search_id)
{
    char path[128];
    if(search_id)
        sprintf(path, "//%s[@id=%c%s%c]", tag.c_str(), '"', search_id, '"');
    else
        sprintf(path, "//%s", tag.c_str());
    misconfiguration = true;
    xmlNodeSetPtr s = findNodes(config, BAD_CAST path);
    if(s==NULL)
        return NULL;
    if(s->nodeNr != 1)
        return NULL;
    return s->nodeTab[0];
}

xmlNodeSetPtr Persist::findNodes(xmlNodePtr config, const xmlChar *path)
{
    xmlXPathContextPtr xpathCtx;
    xmlXPathObjectPtr xpathObj;

    xpathCtx = xmlXPathNewContext(config->doc);
    xpathObj=xmlXPathEvalExpression(path, xpathCtx);
    if(xpathObj==NULL) {
        fprintf(stderr, "Error: no result from xpath\n");
        fflush(stderr);
        xmlXPathFreeContext(xpathCtx);
        return NULL;
    }
    xmlXPathFreeContext(xpathCtx);
    return xpathObj->nodesetval;
}

void Persist::PutBase64Binary(xmlTextWriterPtr writer, const char *xmltag, const uint8_t *data, unsigned len)
{
    xmlTextWriterStartElement(writer, BAD_CAST xmltag);
    xmlTextWriterWriteBase64(writer, (char*)data, 0, len);
    xmlTextWriterEndElement(writer);
}

void Persist::PutHexBinary(xmlTextWriterPtr writer, const char *xmltag, const uint8_t *data, unsigned len)
{
    xmlTextWriterStartElement(writer, BAD_CAST xmltag);
    xmlTextWriterWriteBinHex(writer, (char*)data, 0, len);
    xmlTextWriterEndElement(writer);
}

void Persist::PutHexBinary(xmlTextWriterPtr writer, const char *xmltag, const vector<uint8_t>& data)
{
    xmlTextWriterStartElement(writer, BAD_CAST xmltag);
    for(size_t i=0; i<data.size(); i++) {
        uint8_t s[1];
        s[0]=data[i];
        xmlTextWriterWriteBinHex(writer, (char*)s, 0, 1);
    }
    xmlTextWriterEndElement(writer);
}


void Persist::PutBool(xmlTextWriterPtr writer, const char *xmltag, int var)
{
    PutUnsignedEnum(writer, xmltag, boolvals, var);
}

void Persist::PutString(xmlTextWriterPtr writer, const char *xmltag, const xmlChar *s)
{
    xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%s", (char*)s);
}

void Persist::PutString(xmlTextWriterPtr writer, const char *xmltag, const string& s)
{
    xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%s", s.c_str());
}

void Persist::PutUnsigned(xmlTextWriterPtr writer, const char *xmltag, int n)
{
    if(n<0)
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag, "(undefined)");
    else
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%u", n);
}

void Persist::PutSigned(xmlTextWriterPtr writer, const char *xmltag, int n)
{
    xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%d", n);
}

void Persist::PutDouble(xmlTextWriterPtr writer, const char *xmltag, double n)
{
    xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%f", n);
}

void Persist::PutUnsignedLong(xmlTextWriterPtr writer, const char *xmltag, long n)
{
    if(n<0)
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag, "(undefined)");
    else
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%lu", n);
}

void Persist::PutEnum(xmlTextWriterPtr writer, const char *xmltag, const char * const vals[], int n)
{
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteFormatString(writer,
                                   "The xmltag '%s' can take the following values:\n    ", xmltag);
    char sep=' ';
    for(int i=0; vals[i]; i++) {
        xmlTextWriterWriteFormatString(writer, "%c %s", sep, vals[i]);
        sep=',';
    }
    xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
    xmlTextWriterEndComment(writer);
    if(n<0)
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag, "(undefined)");
    else
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%s", vals[n]);
}

void Persist::PutUnsignedEnum(xmlTextWriterPtr writer, const char *xmltag, const char * const vals[], int n)
{
    xmlTextWriterStartComment(writer);
    xmlTextWriterWriteFormatString(writer,
                                   "The xmltag '%s' can take the following values:\n    ", xmltag);
    char sep=' ';
    for(int i=0; vals[i]; i++) {
        xmlTextWriterWriteFormatString(writer, "%c %u = %s", sep, i, vals[i]);
        sep=',';
    }
    xmlTextWriterWriteString(writer, BAD_CAST "\n  ");
    xmlTextWriterEndComment(writer);
    if(n<0)
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag, "(undefined)");
    else
        xmlTextWriterWriteFormatElement(writer, BAD_CAST xmltag,"%u", n);
}

void Persist::Configuration(xmlTextWriterPtr writer)
{
    xmlTextWriterStartElement(writer, BAD_CAST tag.c_str());
    if(id.length()>0)
        xmlTextWriterWriteAttribute(writer, BAD_CAST "id", BAD_CAST id.c_str());
    PutParams(writer);
    xmlTextWriterEndElement(writer);
}

void Persist::PutParams(xmlTextWriterPtr writer)
{
    PutPrivateParams(writer);
}

void Persist::PutPrivateParams(xmlTextWriterPtr writer)
{
#ifdef NODE
    if(private_config!="") {
        xmlTextWriterWriteRaw(writer, BAD_CAST "\n");
        xmlChar *txt;
        xmlBufferPtr myBuffer=NULL;
        myBuffer=xmlBufferCreate();
        if(myBuffer)
        {
            xmlBufferEmpty(myBuffer);
            xmlNodeDump(myBuffer,private_config->doc,private_config,0,0);
            txt = (xmlChar*)xmlBufferContent(myBuffer);
        }
        xmlBufferFree(myBuffer);
        xmlTextWriterWriteRaw(writer, txt);
        xmlTextWriterWriteRaw(writer, BAD_CAST "\n");
    }
#else
    if(private_config!="") {
        xmlTextWriterWriteRaw(writer, BAD_CAST "\n");
        xmlTextWriterWriteRaw(writer, BAD_CAST private_config.c_str());
        xmlTextWriterWriteRaw(writer, BAD_CAST "\n");
    }
#endif
}

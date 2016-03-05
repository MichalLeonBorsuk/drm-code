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


#ifndef _PERSIST_H
#define _PERSIST_H


#include <libxml/encoding.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlwriter.h>
#include <string>
#include <vector>
/* TODO (jfbc#2#): incorporate XML Namespaces */
/* TODO (jfbc#2#): Develop Schemas and use validating parsers */
/* TODO (jfbc#2#): Use XPointers for ids instead of literals, incorporate 
                   support for external files (XLink) */
/* TODO (jfbc#2#): Add a Dictionary class for the enumerated xml 
                   variables, share with DCP AF processing */
/* TODO (jfbc#2#): Improve separation between reading & writing and 
                   actual configuration so that the reading and writing 
                   classes are suitable for use in configuration file writing 
                   applications (e.g. GUIs) and external SCEs */


class Persist
{
public:
  Persist();
  Persist(const Persist&);
  Persist& operator=(const Persist&);
  virtual ~Persist();
  unsigned long xmlStringToUnsigned(const xmlChar *s);
  int htob(int n);
  virtual bool setParam(const char *param, const char *value) { return false; }
  virtual const char *putParams(const char *sep, xmlTextWriterPtr writer) { return sep;}
  void parseHexBinary(xmlNodePtr c, const char *tag, uint8_t **data, unsigned *len);
  void parseHexBinary(xmlNodePtr c, const char *tag, std::vector<uint8_t>& data);
  void parseEnum(const xmlChar *val, int *var, const char * const vals[]);
  void parseEnum(xmlNodePtr c, const char *tag, int *var, const char * const vals[]);
  void parseBase64Binary(xmlNodePtr c , const char *tag, uint8_t *data, unsigned *len);
  void parseIPv4Addr(xmlNodePtr c, const char *tag, unsigned long *var);
  void parseDouble(xmlNodePtr c, const char *tag, double *var);
  void parseUnsigned(xmlNodePtr c, const char *tag, int *var);
  void parseSigned(xmlNodePtr c, const char *tag, int *var);
  void parseUnsignedLong(xmlNodePtr c, const char *tag, int long *var);
  void parseBool(xmlNodePtr c, const char *tag, int *var);
  void parseIDREF(xmlNodePtr c, const char *tag, std::string& s);
  virtual void ReConfigure(xmlNodePtr config);
  virtual xmlNodePtr find(xmlNodePtr config, const char* id=NULL);
  virtual void walkConfig(xmlNodePtr n);
  virtual void clearConfig();
  virtual void GetParams(xmlNodePtr n) {}
  virtual xmlNodeSetPtr findNodes(xmlNodePtr config, const xmlChar *path);
  void PutHexBinary(xmlTextWriterPtr writer, const char *tag, const uint8_t *data, unsigned len);
  void PutHexBinary(xmlTextWriterPtr writer, const char *tag, const std::vector<uint8_t>& data);
  void PutBase64Binary(xmlTextWriterPtr writer, const char *tag, const uint8_t *data, unsigned len);
  void PutBool(xmlTextWriterPtr writer, const char *tag, int var);
  void PutString(xmlTextWriterPtr writer, const char *tag, const std::string &s);
  void PutString(xmlTextWriterPtr writer, const char *tag, const xmlChar *s);
  void PutDouble(xmlTextWriterPtr writer, const char *tag, double n);
  void PutUnsigned(xmlTextWriterPtr writer, const char *tag, int n);
  void PutUnsignedLong(xmlTextWriterPtr writer, const char *tag, long n);
  void PutSigned(xmlTextWriterPtr writer, const char *tag, int n);
  void PutUnsignedEnum(xmlTextWriterPtr writer, const char *tag, const char * const vals[], int n);
  void PutEnum(xmlTextWriterPtr writer, const char *tag, const char * const vals[], int n);

  virtual void Configuration(xmlTextWriterPtr writer);
  virtual void PutParams(xmlTextWriterPtr writer);
  virtual void PutPrivateParams(xmlTextWriterPtr writer);

  std::string tag;
  std::string id;
  bool misconfiguration;
//#define NODE
#ifdef NODE
  xmlNodePtr private_config;
#else
  std::string private_config;
#endif

protected:
  static const char *boolvals[];
};  

#endif


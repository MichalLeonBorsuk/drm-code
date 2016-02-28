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

#include <string.h>
#include <cstdlib>
#include <sstream>
#include "MotObject.h"
#include "timestamp.h"
#include <iostream>

MotObject::MotObject():
    file_size(0), object_version(0),
    mime_type(""), content_type(0), content_subtype(0),
    transport_id(0), content_name(""), file_name(""),
    scope_id(-1), compressed(false),always_send_mime_type(false)
{
    profiles.clear();
    scope_start.clear();
    scope_end.clear();
}

MotObject::MotObject(const MotObject& o)
    :
    file_size(o.file_size),
    object_version(o.object_version), mime_type(o.mime_type),
    content_type(o.content_type), content_subtype(o.content_subtype),
    transport_id(o.transport_id), content_name(o.content_name), file_name(o.file_name),
    compression_type(o.compression_type), profiles(o.profiles),
    scope_start(o.scope_start), scope_end(o.scope_end), scope_id(o.scope_id),
    compressed(o.compressed), always_send_mime_type(o.always_send_mime_type)
{
}

MotObject& MotObject::operator=(const MotObject& o)
{
    file_size = o.file_size;
    object_version = o.object_version;
    mime_type = o.mime_type;
    content_type = o.content_type;
    content_subtype = o.content_subtype;
    transport_id = o.transport_id;
    content_name = o.content_name;
    file_name = o.file_name;
    compression_type = o.compression_type;
    profiles = o.profiles;
    scope_start = o.scope_start;
    scope_end = o.scope_end;
    scope_id = o.scope_id;
    compressed = o.compressed;
    always_send_mime_type = o.always_send_mime_type;
    return *this;
}

MotObject::~MotObject()
{
    mime_type.clear();
    content_name.clear();
    file_name.clear();
    profiles.clear();
    scope_start.clear();
    scope_end.clear();
}

/* we output the extension headers in order of parameter id,
   which is asked for by some applications, for example EPG.

   0000 Complete EBU Latin based repertoire [2]
   0001 EBU Latin based common core, Cyrillic, Greek [2]
   0010 EBU Latin based core, Arabic, Hebrew, Cyrillic, Greek [2]
   0011 ISO Latin Alphabet No2 (see ISO/IEC 8859-2 [29])
   0100 ISO Latin Alphabet No1 (see ISO/IEC 8859-1 [8])
   1111 ISO/IEC 10646 [26] (UTF-8 encoding) (see ISO/IEC 10646 [26])

*/
void MotObject::putHeader(bytevector& out) const
{
    // build extension header
    bytevector ext;
    putStringParameter(ext, 0x0c, content_name, 0x40 /* ISO Latin 1*/);
    if(always_send_mime_type)
        putStringParameter(ext, 0x10, mime_type);
    if(compressed) {
        putExtensionParameterHeader(ext, 0x11, 1);
        ext.put(compression_type, 8);
    }
    if(profiles.size()>0)
        putExtensionParameterHeader(ext, 0x21, profiles.size());
    ext.put(profiles);
    if(scope_start.size()>0) {
        putExtensionParameterHeader(ext, 0x25, scope_start.size());
        ext.put(scope_start);
    }
    if(scope_end.size()>0) {
        putExtensionParameterHeader(ext, 0x26, scope_end.size());
        ext.put(scope_end);
    }
    if(scope_id>=0) {
        putExtensionParameterHeader(ext, 0x27, 3);
        ext.put(scope_id, 24);
    }
    // header core
    putHeaderCore(out, file_size, 7+ext.size(), content_type, content_subtype);
    out.put(ext);
}

void MotObject::putHeaderCore(bytevector& out,
                              size_t body_size, size_t hdr_size,
                              uint16_t content_type, uint16_t content_subtype)
{
    out.put(body_size, 28);
    out.put(hdr_size, 13);
    out.put(content_type, 6);
    out.put(content_subtype, 9);
}

void MotObject::putExtensionParameterHeader(bytevector& out,
        uint8_t id, size_t len)
{
    switch(len) {
    case 0:
        out.put(0, 2);
        out.put(id, 6);
        break;
    case 1:
        out.put(1, 2);
        out.put(id, 6);
        break;
    case 4:
        out.put(2, 2);
        out.put(id, 6);
        break;
    default:
        out.put(3, 2); // to indicate variable length content
        out.put(id, 6);
        if(len<128) {
            out.put(0, 1);
            out.put(len, 7);
        } else {
            out.put(0, 1);
            out.put(len, 15);
        }
    }
}

void MotObject::putStringParameter(bytevector& out, uint8_t param,
                                   const string& s, int prefix)
{
    string p = s;
    size_t max = 32767, len=0;
    if(prefix>=0) {
        max--;
        len++;
    }
    len += p.length();
    if(len>max) {
        len=max;
        p.resize(len); // truncate!
    }
    putExtensionParameterHeader(out, param, len);
    if(prefix>=0) {
        out.put(prefix, 8);
    }
    out.put(p);
}

void MotObject::setHeaders(const string& f)
{
    file_name = f;
    // pass the basename of the file name as the mime type, since we don't know any better
    /* TODO (jfbc#1#): Make the EPG Specials implemented by
                       configuration, not coding */
    string working_filename = f;
    size_t n = working_filename.rfind(".");
    string ext;
    if(n != string::npos)
        ext = working_filename.substr(n+1);
    if(ext == "gz") {
        compressed = true;
        setCompressionType(1);
        working_filename = working_filename.substr(0, n);
        n = working_filename.rfind(".");
        if(n != string::npos)
            ext = working_filename.substr(n+1);
        else
            ext = "";
    } else {
        compressed = false;
    }
    setMimeType(working_filename);
    setContentName(working_filename);
    /* the following code is very specific to the BBC WS File Naming convention
       for Basic and Advanced profile compressed EPG files, e.g.
       YYYYMMDD_EE_SSSSSS_T.EHB
       YYYYMMDD_EE_SSSSSS_T.EHA
    Where:
    YYYY represents the year,
    MM represents the month,
    DD represents the day of the month
    EE represents the number of days the file covers
    SSSSSS represents the service id.
    T represents the type of the file, service (S) programme  (P) or group (G)
    Each file should contain a single
    <schedule> element and should contain <programme> elements, ordered by start time, for all programmes carried on
    this service that are billed to start at or between 00:00:00 and 23:59:59 on the date indicated in the filename.
    NOTE: The scope element in a schedule indicates the time period covered by the schedule, from the billed start
    time of the first programme to the billed end time of the last programme.
      */
    if(ext=="EHB" || ext=="EHA") {
        ostringstream s;
        s << "o" << transport_id << "." << ext;
        setContentName(s.str());
        struct tm tm;
        time_t t;
        char *pEnd;
        tm.tm_year = strtoul(f.substr(0, 4).c_str(), &pEnd, 10) - 1900;
        tm.tm_mon = strtoul(f.substr(4, 2).c_str(), &pEnd, 10) - 1;
        tm.tm_mday = strtoul(f.substr(6, 2).c_str(), &pEnd, 10);
        /* Mingw did wrong thing with tm_dst=-1 */
        tm.tm_hour=0;
        tm.tm_min=0;
        tm.tm_sec=0;
        tm.tm_wday=0;
        tm.tm_yday=0;
        tm.tm_isdst=0;
        t = mktime(&tm); /* timezone ? */
        setScopeStart(t);
        time_t days = strtoul(f.substr(9,2).c_str(), &pEnd, 10);
        setScopeEnd(t+86400*days);
        uint32_t sid = strtoul(f.substr(12,6).c_str(), &pEnd, 16);
        setScopeId(sid);
        char type = f[19];
        content_type = 7;
        switch(type) {
        case 'S':
            content_subtype = 0;
            break;
        case 'P':
            content_subtype = 1;
            break;
        case 'G':
            content_subtype = 2;
        }
        if(ext=="EHA") {
            bytevector profiles;
            profiles.put(2, 8);
            setProfileSubset(profiles);
        }
    }
}

void MotObject::setMimeType(const string& mt)
{
    string major, minor;
    size_t p = mt.find('/');
    if(p==string::npos) { // assume the basename of the content name was passed
        find_mime_by_ext(major, minor, mt);
        mime_type = major + "/" + minor;
    } else {
        size_t q = mt.find(';');
        major = mt.substr(0, p);
        minor = mt.substr(p+1, q-p-1);
        mime_type = mt;
    }
    find_type_by_mime(content_type, content_subtype, major, minor);
}

void MotObject::setContentName(const string& pcontent_name)
{
    content_name = pcontent_name;
    //cout << "Content Name " << content_name << endl;
}

void MotObject::setProfileSubset(const bytevector& pprofiles)
{
    profiles = pprofiles;
}

void MotObject::setCompressionType(uint8_t pcompression_type)
{
    compression_type = pcompression_type;
}

/* From TS 102 371:
4.7.1	Date and time
All elements defined as timePointType are encoded as follows.

Figure 2: Date and time encoding (LTO flag == 1)

Figure 3: Date and time encoding (LTO flag == 0)
Rfa: This 1-bit field shall be reserved for future additions. The bit shall be set to zero for the currently specified definition of this field.
NOTE 1:	Receivers shall ignore this bit.
Date: This 17-bit unsigned binary number shall define the current date according to the Modified Julian Date (MJD) coding strategy (EN 300 401 [3]). This number increments daily at 0000 UTC and extends over the range 0 to 99999. As an example MJD 50000 corresponds to October 10th, 1995.
Rfa: This 1-bit field shall be reserved for future additions. The bit shall be set to zero for the currently specified definition of this field.
NOTE 2:	Receivers shall ignore this bit.
LTO flag: This 1-bit field indicates whether the LTO field (see below) is present or not, as follows:
0:	LTO not present, time is in UTC.
1:	LTO present, local time is UTC plus LTO.
UTC flag: This 1-bit field indicates whether the UTC (see below) takes the short form or the long form, as follows:
0:	UTC short form.
1:	UTC long form.
UTC (Co-ordinated Universal Time): Two forms are available depending upon the state of the UTC flag. They are defined as follows:
"	Short form: This 11-bit field contains two sub-fields, coded as unsigned binary numbers. The first sub-field is a 5-bit field which shall define the hours and the other sub-field is a 6-bit field which shall define the minutes.
"	Long form: In addition to the hours and minutes fields defined in the short form, this 27-bit field shall contain one further sub-field which shall be encoded as an unsigned binary number. This is a 6-bit field which shall define the seconds. The following 10-bits shall be reserved for future additions. These bits shall be set to zero for the currently specified definition of this field.
LTO (Local Time Offset): This 8-bit field shall give the Local Time Offset (LTO) for the time given. It is only present if the LTO flag is set to 1. The first two bits are reserved for future additions, they shall be set to zero for the currently specified definition and shall be ignored by receivers. The next bit shall give the sense of the LTO, as follows:
0:	Positive offset.
1:	Negative offset.
The final 5 bits define the offset in multiples of half-hours in the range -12 hours to +12 hours.
For example, a programme broadcast at 05:00 in the UK during Daylight Savings time would have a UTC of 04:00 and an LTO of +1 hour.

*/

void MotObject::putDateTime(bytevector& out,
                            uint32_t mjd, uint8_t hours, uint8_t minutes, uint8_t seconds, int8_t lto
                           )
{
    bool utc_flag = (seconds > 0);
    bool lto_flag = (lto != 0);
    out.put(0, 1); // rfa
    out.put(mjd, 17);
    out.put(0, 1); // rfa
    out.put(utc_flag?1:0, 1);
    out.put(lto_flag?1:0, 1);
    out.put(hours, 5);
    out.put(minutes, 6);
    if(utc_flag) {
        out.put(seconds, 6);
        out.put(0, 10); // rfa
    }
    if(lto_flag) {
        uint8_t sign, half_hours;
        if(lto>0) {
            sign = 0;
            half_hours = lto;
        } else {
            sign = 1;
            half_hours = - lto;
        }
        out.put(0, 2); // rfa
        out.put(sign, 1);
        out.put(half_hours, 5);
    }
}

/* From TS 102 371:

6.4.6	ScopeStart
This parameter is used for Programme Information EPG objects only;
it shall not be used for Service Information or Group Information objects.
For Programme Information objects, this parameter shall be mandatory and is used
to indicate the billed start date and time (in service local time) of the first
programme covered by EPG data contained within this Programme Information object.
 This shall be encoded as defined in clause 4.7.1, with the following restriction
  that only short-form UTC with an optional LTO shall be used. The start
   time shall be rounded down to the nearest minute.

*/

void MotObject::setScopeStart(time_t pscope_start, int8_t local_time_offset)
{
    uint32_t mjd;
    uint8_t hours, minutes;
    DrmTime::date_and_time(mjd, hours, minutes, pscope_start);
    scope_start.clear();
    putDateTime(scope_start, mjd, hours, minutes, 0, local_time_offset);
}

void MotObject::setScopeEnd(time_t pscope_end, int8_t local_time_offset)
{
    uint32_t mjd;
    uint8_t hours, minutes;
    DrmTime::date_and_time(mjd, hours, minutes, pscope_end);
    scope_end.clear();
    putDateTime(scope_end, mjd, hours, minutes, 0, local_time_offset);
}

/*
6.4.8	ScopeID
If the object contains Service Information or Group Information,
then this parameter indicates the ensembleID of the ensemble/channel
 for which the object contains data (coded as specified in clause 4.7.7).
If the object contains Programme Information, then this parameter
indicates the contentID of the service for which the object contains
 data, as specified in clause 4.7.6. If the object contains schedule
  data for more than one service then this parameter shall consist
  of a continuous sequence of the relevant contentIDs.
This parameter is encoded as a sequence of bytes representing the
contentID, or sequence of contentIDs, as defined in clause 4.7.6.
*/

void MotObject::setScopeId(uint32_t pscope_id)
{
    scope_id = pscope_id;
}

/*
Content type b14 b9 Interpretation Content subtype b8 b0 Interpretation
000000 general data                 000000000 Object Transfer
                                    000000001 MIME/HTTP [5], [6]
000001 text                         000000000 Text (US ASCII) [7]
                                    000000001 Text (ISO Latin 1) [8]
                                    000000010 HTML (default: ISO Latin 1) [9], [8]
000010 image                        000000000 GIF (see Bibliography)
                                    000000001 JFIF [10]
                                    000000010 BMP (see Bibliography)
                                    000000011 PNG [21]
000011 audio                        000000000 MPEG I audio Layer I [11]
                                    000000001 MPEG I audio Layer II [11]
                                    000000010 MPEG I audio Layer III [12]
                                    000000011 MPEG II audio Layer I [12]
                                    000000100 MPEG II audio Layer II [12]
                                    000000101 MPEG II audio Layer III [12]
                                    000000110 uncompressed PCM audio [13]
                                    000000111 AIFF (see Bibliography)
                                    000001000 ATRAC (see Bibliography)
                                    000001001 ATRAC II (see Bibliography)
                                    000001010 MPEG 4 audio [14]
000100 video                        000000000 MPEG I video [15]
                                    000000001 MPEG II video [16]
                                    000000010 MPEG 4 video [17]
                                    000000011 H263 [18]
000101 MOT transport                000000000 Header update
000110 system                       000000000 MHEG [19]
                                    000000001 Java (see Bibliography)
000111 Application                  000000000 to 111111111 User application specific
111111 proprietary table            000000000 to 111111111 proprietary
*/

void MotObject::find_type_by_mime(
    uint8_t& type, uint16_t& subtype,
    const string& major, const string& minor
) const
{
    if(major=="text") {
        if(minor == "html") {
            type = 1;
            subtype = 2;
        } else if(minor == "plain") {
            type = 1;
            subtype = 0;
        } else if(minor == "xml") {
            type = 0;
            subtype = 0;
        } else if(minor == "xsl") {
            type = 0;
            subtype = 0;
        } else {
            type = 0;
            subtype = 0;
        }
    } else if(major == "image") {
        type = 2;
        if(minor == "gif") {
            subtype = 0;
        } else if(minor == "jpeg") {
            subtype = 1;
        } else if(minor == "bmp") {
            subtype = 2;
        } else if(minor == "png") {
            subtype = 3;
        } else {
            type = 0;
            subtype = 0;
        }
    } else if(major == "audio") {
        type = 3;
        if(minor == "mpeg") {
            subtype = 5;
        } else if(minor == "mp4") {
            subtype = 10;
        } else {
            type = 0;
            subtype = 0;
        }
    } else if(major == "video") {
        type = 4;
        if(minor == "mpeg") {
            subtype = 1;
        } else if(minor == "mp4") {
            subtype = 2;
        } else {
            type = 0;
            subtype = 0;
        }
    } else if(major == "application") {
        type = 7;
        if(minor == "x-javascript") {
            subtype = 1;
        } else if(minor == "service") {
            subtype = 0;
        } else if(minor == "programme") {
            subtype = 1;
        } else if(minor == "group") {
            subtype = 2;
        } else {
            type = 0;
            subtype = 0;
        }
    } else {
        type = 0;
        subtype = 0;
    }
}

void MotObject::find_mime_by_ext(
    string& major, string& minor, const string& content_name
) const
{
    string ext = content_name.substr(content_name.find_last_of('.')+1);
    if(ext == "htm") {
        major = "text";
        minor = "html";
    } else if(ext == "html") {
        major = "text";
        minor = "html";
    } else if(ext == "txt") {
        major = "text";
        minor = "plain";
    } else if(ext == "xml") {
        major = "text";
        minor = "xml";
    } else if(ext == "xsl") {
        major = "text";
        minor = "xsl";
    } else if(ext == "js") {
        major = "application";
        minor = "x-javascript";
    } else if(ext == "gif") {
        major = "image";
        minor = "gif";
    } else if(ext == "jpg") {
        major = "image";
        minor = "jpeg";
    } else if(ext == "jpeg") {
        major = "image";
        minor = "jpeg";
    } else if(ext == "bmp") {
        major = "image";
        minor = "bmp";
    } else if(ext == "png") {
        major = "image";
        minor = "png";
    } else if(ext == "hvx") {
        major = "audio";
        minor = "mp4";
    } else if(ext == "aac") {
        major = "audio";
        minor = "mp4";
    } else if(ext == "mp3") {
        major = "audio";
        minor = "mpeg";
    } else if(ext == "EHB") {
        major = "application";
        minor = "programme";
    } else if(ext == "EHA") {
        major = "application";
        minor = "programme";
    }
}

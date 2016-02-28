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
#include "JML.h"
#include <time.h>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <zlib.h>

using namespace std;
using namespace JML;

void compress(bytevector& out, const bytevector& in, uint8_t type)
{
    if(type!=0x08)
    {
        throw(string("unrecognised compression type"));
        return;
    }
    Bytef* o = new Bytef[2*in.size()];
    z_stream strm;
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef*)in.data();
    strm.next_out = o;
    strm.avail_in = in.size();
    strm.avail_out = 2*in.size();
    if(deflateInit2(&strm, 9, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY)!=Z_OK)
        throw(string("zlib init error"));
    if(deflate(&strm, Z_FINISH)!=Z_STREAM_END)
        throw(string("zlib process error"));
    if(deflateEnd(&strm)!=Z_OK)
        throw(string("zlib finish error"));
    out.putbytes((char*)o, strm.total_out);
    delete[] o;
}

// untested
void uncompress(bytevector& out, const bytevector& in, uint8_t type)
{
    if(type!=0x08)
    {
        throw(string("unrecognised compression type"));
    }
    z_stream strm;
    strm.zalloc = 0;
    strm.zfree = 0;
    strm.next_in = (Bytef*)in.data();
    strm.avail_in = in.size();
    strm.next_out = (Bytef*)out.data();
    strm.avail_out = out.size();

    int err = inflateInit2(&strm, -15);
    if (err != Z_OK)
    {
        return;
    }

    err = inflate(&strm, Z_FINISH);
    if (err != Z_STREAM_END)
    {
        inflateEnd(&strm);
        return;
    }
    out.resize(strm.total_out);

    err = inflateEnd(&strm);
}

void Text::encode(bytevector& out) const
{
    for(size_t i=0; i<encoded_string.length(); i++)
        out.put(encoded_string[i]);
}

void Text::decode(bytevector& m)
{
    encoded_string = "";
    while(true) // Capture UTF-8 until control char
    {
        if(m.peek()<0x1f)
        {
            if(m.peek()==JML::End) // consume an End but not anything else
                (void)m.get();
            break;
        }
        else
        {
            encoded_string += m.get();
        }
    }
}

static const time_t epoch=0;

static void decodeDS(std::ostream& out, const unsigned char* buf, size_t n)
{
    std::ostringstream s;
    switch(buf[0])
    {
    case Padding:
        break;
    case AbsoluteTimeout:
    {
        time_t n = (buf[0] << 16)+(buf[1] << 8)+buf[2];
        n = epoch + 15*60*n;
        tm tm = *gmtime(&n);
        char s[32];
        putenv(const_cast<char*>("TZ=UTC"));
        strftime(s, sizeof(s), "%Y-%m-%dT%T", &tm);
        out << "<span AbsoluteTimeout='" << s << "'/>";
    }
    break;
    case RelativeTimeout:
    {
        int n = 15*60*((buf[0] << 16)+(buf[1] << 8)+buf[2]);
        out << "<span RelativeTimeout='" << n << "'/>";
    }
    break;
    case GeneralLinkTarger:
        break;
    case Keyword:
        break;
    case MacroDefinition:
        break;
    case MacroReference:
        break;
    case DefaultLanguage:
        break;
    case LanguageSection:
        break;
    case SpeechPhoneme:
        break;
    case SpeechBreak:
        break;
    case SpeechCharacters:
        break;
    case LatLongPositon:
        break;
    case LatLongRegion:
        break;
    case Proprietary:
        break;
    default:
        ;
    }
}

void Text::asHTML(std::ostream& out)
{
    std::istringstream in(encoded_string);
    while(in)
    {
        char c;
        in.get(c);
        if(in.fail())
            break;
        switch(c)
        {
        case PreferredLineBreak:
            out << "<span class='PreferredLineBreak'/>";
            break;
        case PreferredWordBreak:
            out << "<span class='PreferredWordBreak'/>";
            break;
        case HighlightStart:
            out << "<em>";
            break;
        case HighlightStop:
            out << "</em>";
            break;
        case EndOfIntroductorySection:
            out << "<span class='EndOfIntroductorySection'/>";
            break;
        case DataSectionStart:
        {
            char buf[4096];
            size_t p=0;
            in.get(c);
            size_t n = int(*reinterpret_cast<unsigned char*>(&c))+1;
            while(n>0)
            {
                in.read(&buf[p], n);
                p+=n;
                if(n==256 && in.peek()==DataSectionContinuation)
                {
                    in.ignore(1);
                    in.get(c);
                    n = int(*reinterpret_cast<unsigned char*>(&c))+1;
                }
            }
            decodeDS(out, reinterpret_cast<unsigned char*>(buf), p);
        }
        break;
        case DataSectionContinuation:
            in.ignore(1); // should not happen
            break;
        case ExtendedCodeBegin:
            in.get(c);
            out << "<span class='X" << int(c) << "'>";
            break;
        case ExtendedCodeEnd:
            in.ignore(1);
            out << "</span>";
            break;
        default:
            out.put(c);
        }
    }
}

void TitleBlock::asHTML(std::ostream& out)
{
    out << "<span class='title'>";
    text.asHTML(out);
    out << "</span>";
}

void TitleBlock::encode(bytevector& out) const
{
    out.put(JML::Title);
    text.encode(out);
}

void TitleBlock::decode(bytevector& m)
{
    JMLCode c = (JMLCode)m.get();
    if(c!=JML::Title)
        throw "parsing problem - Title must start with Title code";
    text.decode(m);
}

void ListBlock::asHTML(std::ostream& out)
{
    out << "<span class='list'>";
    if(col.size()==1)
    {
        col[0].asHTML(out);
    }
    else
    {
        for(size_t i=0; i<col.size(); i++)
        {
            out << "<span class='listColumn'>";
            col[i].asHTML(out);
            out << "</span>";
        }
    }
    out << "</span>";
}

void ListBlock::encode(bytevector& out) const
{
    out.put(JML::ListItem);
    col[0].encode(out);
    for(size_t i=1; i<col.size(); i++)
    {
        out.put(ListColumn);
        col[i].encode(out);
    }
}

void ListBlock::decode(bytevector& m)
{
    JMLCode c = (JMLCode)m.get();
    if(c!=JML::ListItem)
        throw "parsing problem - Title must start with Title code";
    col.clear();
    Text t;
    t.decode(m);
    col.push_back(t);
    while(m.peek()==ListColumn)
    {
        (void)m.get();
        t.decode(m);
        col.push_back(t);
    }
}

Link::Link(const JMLObject& o)
{
    ref = o.ObjectID;
    label = o.title.text;
}

void Link::asHTML(std::ostream& out)
{
    out << "<a href='" << ref << ".html'>";
    label.asHTML(out);
    out << "</a>";
}

void Link::encode(bytevector& out) const
{
    out.put(JML::LinkItem);
    out.put(ref, 16);
    label.encode(out);
}

void Link::decode(bytevector& m)
{
    JMLCode c = (JMLCode)m.get();
    if(c!=JML::LinkItem)
        throw "expected LinkItem";
    ref = m.get(16);
    label.decode(m);
}

JMLObject::JMLObject():
    ObjectID(0),ObjectType(undefined),
    Static(false),Compress(false),RevisionIndex(0),
    title(),body(),list(),link(),collection(NULL)
{
}

JMLObject::JMLObject(const JMLObject& o):
    ObjectID(o.ObjectID),ObjectType(o.ObjectType),
    Static(o.Static),Compress(o.Compress),
    RevisionIndex(o.RevisionIndex),
    title(o.title),body(o.body),list(o.list),link(o.link),
    collection(o.collection)
{
}

JMLObject::JMLObject(const uint16_t id, const string& t):
    ObjectID(id),ObjectType(TitleOnly),
    Static(false),Compress(false),
    RevisionIndex(0),title(t),body(),list(),link(),collection(NULL)
{
}

JMLObject::JMLObject(const uint16_t id, const string& t, const string& b):
    ObjectID(id),ObjectType(PlainText),
    Static(false),Compress(false),
    RevisionIndex(0),title(t),body(b),list(),link(),collection(NULL)
{
}

JMLObject::JMLObject(const uint16_t id, const string& t, const vector<ListBlock>& l):
    ObjectID(id),ObjectType(List),
    Static(false),Compress(false),
    RevisionIndex(0),title(t),body(),list(l),link(),collection(NULL)
{
}

JMLObject::JMLObject(const uint16_t id, const string& t, const vector<Link>& l):
    ObjectID(id),ObjectType(Menu),
    Static(false),Compress(false),
    RevisionIndex(0),title(t),body(),list(),link(l),collection(NULL)
{
}

void JMLObject::encode(bytevector& out) const
{
    bytevector raw, compressed;
    title.encode(raw);
    switch(ObjectType)
    {
    case undefined:
        break;
    case Menu:
        for(size_t i=0; i<link.size(); i++)
            link[i].encode(raw);
        break;
    case PlainText:
        raw.put(JML::BodyText);
        body.encode(raw);
        break;
    case TitleOnly:
        break;
    case List:
        for(size_t i=0; i<list.size(); i++)
            list[i].encode(raw);
        break;
    }
    compress(compressed, raw, 0x08);
    out.put(ObjectID, 16);
    out.put(uint8_t(ObjectType), 3);
    out.put(Static?1:0, 1);
    bool c = compressed.size()<raw.size();
    out.put(c?1:0, 1);
    out.put(RevisionIndex, 3);
    if(c)
    {
        //cout << "putting compressed " << compressed.size() << " < " << raw.size() << endl;
        out.put(0x08); // magic number
        out.put(compressed);
    }
    else
    {
        //cout << "putting raw " << raw.size() << " < " << compressed.size() << endl;
        out.put(raw);
    }
}

void JMLObject::decode(bytevector& m)
{
    ObjectID = m.get(16);
    ObjectType = eObjectType(m.get(3));
    Static = m.get(1);
    Compress = m.get(1);
    RevisionIndex = m.get(3);
    bytevector obj;
    if(Compress)
    {
        uint8_t type = m.get();
        uncompress(obj, m, type);
    }
    else
    {
        obj.put(m);
    }
    title.decode(obj);
    switch(ObjectType)
    {
    case undefined:
        break;
    case Menu:
        link.clear();
        while(obj.dataAvailable() && (JMLCode)obj.peek() == LinkItem)
        {
            Link l;
            l.decode(obj);
            link.push_back(l);
        }
        break;
    case PlainText:
        body.decode(obj);
        break;
    case TitleOnly:
        break;
    case List:
        list.clear();
        while(obj.dataAvailable() && (JMLCode)obj.peek() == ListItem)
        {
            ListBlock l;
            l.decode(obj);
            list.push_back(l);
        }
        if(list.size()<2)
            throw "need at least two link items in a List Message";
    }
}

void JMLObject::asHTML(ostream& out)
{
    out << "<html><body>" << endl;
    out << "<a id='" << ObjectID << "'/>";
    title.asHTML(out);
    switch(ObjectType)
    {
    case undefined:
        break;
    case Menu:
        //out << "<div class='Menu'>";
        out << "<ul>";
        for(size_t i=0; i<link.size(); i++)
        {
            out << "<li>";
            link[i].asHTML(out);
            out << "</li>";
        }
        //out << "</div>";
        out << "</ul>";
        break;
    case PlainText:
        body.asHTML(out);
        break;
    case TitleOnly:
        break;
    case List:
        out << "<div class='List'>";
        for(size_t i=0; i<list.size(); i++)
            list[i].asHTML(out);
        out << "</div>";
        break;
    }
    out << "</body></html>" << endl;
}

void JMLObjectCollection::add(const JMLObject& o)
{
    item[o.ObjectID] = o;
    item[o.ObjectID].collection = this;
    item[o.ObjectID].RevisionIndex = RevisionIndex;
}


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
#ifndef _JML_H
#define _JML_H

#include <string>
#include <vector>
#include <bytevector.h>
#include <map>

namespace JML
{

/*
ETSI TS 102 979 V1.1.1 (2008-06)

Object type: this 3-bit value indicates the type of JML object:
- 000: reserved.
- 001: Menu object.
- 010: Plain Text message.
- 011: Title-Only message.
- 100: List message.
- 101: reserved.
- 110: reserved.
- 111: reserved.
*/

enum eObjectType
{
    undefined = 0,
    Menu = 1,
    PlainText = 2,
    TitleOnly = 3,
    List = 4
};

enum JMLCode
{
    End = 0,
    Title = 1,
    LinkItem = 2,
    BodyText = 3,
    ListItem = 4,
    ListColumn = 5
};

enum JMLEscape
{
    PreferredLineBreak = 0x10,
    PreferredWordBreak = 0x11,
    HighlightStart = 0x12,
    HighlightStop = 0x13,
    EndOfIntroductorySection = 0x14,
    DataSectionStart = 0x1a,
    DataSectionContinuation = 0x1b,
    ExtendedCodeBegin = 0x1c,
    ExtendedCodeEnd = 0x1d
};

enum JMLDSCode
{
    Padding=0,
    AbsoluteTimeout=1,
    RelativeTimeout=2,
    GeneralLinkTarger=3,
    Keyword=0x20,
    MacroDefinition=0x21,
    MacroReference=0x22,
    DefaultLanguage=0xA0,
    LanguageSection=0xA1,
    SpeechPhoneme=0xA2,
    SpeechBreak=0xA3,
    SpeechCharacters=0xA4,
    LatLongPositon=0xB0,
    LatLongRegion=0xB1,
    Proprietary = 0xff
};

/*
class MemoryStream
{
public:
// TODO throw on out of bounds
    MemoryStream():length(0),pos(0) {}
    bool dataAvailable() { return pos<(length-1); }
    uint8_t get() { return buffer[pos++]; length--; }
    uint8_t peek() { return buffer[pos]; }
    void put (uint8_t value) { buffer[pos++] = value; length++; }
    void setPos(size_t n) { pos = n; }
    void write(const uint8_t* buf, size_t n)
        { memcpy(&buffer[pos],  buf, n); pos+=n; length+=n; }
    void read(uint8_t* buf, size_t n) { memcpy(buf, &buffer[pos], n); pos+=n; }
private:
    size_t length;
    size_t pos;
    uint8_t buffer[4092]; // maybe my bytevector
};
*/

class Text
{
public:
    Text():encoded_string(){}
    Text(const Text& t):encoded_string(t.encoded_string){}
    Text(const std::string& s):encoded_string(s){}
    std::string encoded_string;
    void asHTML(std::ostream&);
    void encode(bytevector&) const;
    void decode(bytevector&);
};

class TitleBlock
{
public:
    TitleBlock():text() {}
    TitleBlock(const TitleBlock& t):text(t.text) {}
    TitleBlock(const Text& t):text(t) {}
    Text text;
    void asHTML(std::ostream&);
    void encode(bytevector&) const;
    void decode(bytevector&);
};

class ListBlock
{
public:
    std::vector<Text> col;
    void asHTML(std::ostream&);
    void encode(bytevector&) const;
    void decode(bytevector&);
};

class JMLObject;

class Link
{
public:
    Link():ref(0),label() {}
    Link(uint16_t r, const std::string& l):ref(r),label(l) {}
    Link(const JMLObject&);
    uint16_t ref;
    Text label;
    void asHTML(std::ostream&);
    void encode(bytevector&) const;
    void decode(bytevector&);
};

class JMLObjectCollection;

class JMLObject
{
public:
    JMLObject();
    JMLObject(const JMLObject&);
    JMLObject(const uint16_t, const std::string&);
    JMLObject(const uint16_t, const std::string&, const std::string&);
    JMLObject(const uint16_t, const std::string&, const std::vector<ListBlock>&);
    JMLObject(const uint16_t, const std::string&, const std::vector<Link>&);
    void encode(bytevector&) const;
    void decode(bytevector&);
    void asHTML(std::ostream&);
    uint16_t ObjectID;
    eObjectType ObjectType;
    bool Static;
    bool Compress;
    uint8_t RevisionIndex;
    TitleBlock title;
    Text body;
    std::vector<ListBlock> list;
    std::vector<Link> link;
    JMLObjectCollection* collection;
};

class JMLObjectCollection
{
public:
    JMLObjectCollection():RevisionIndex(0),item() {}
    void add(const JMLObject& o);
    size_t size() const { return item.size(); }
    JMLObject& operator[](size_t i) { return item[i]; }
    bool contains(uint16_t o) const { return item.find(o) != item.end(); }
    uint8_t RevisionIndex;
protected:
    std::map<uint16_t,JMLObject> item;
};

};
#endif

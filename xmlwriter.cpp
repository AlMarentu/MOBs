// Bibliothek zur einfachen Verwendung serialisierbarer C++-Objekte
// für Datenspeicherung und Transport
//
// Copyright 2020 Matthias Lautner
//
// This is part of MObs https://github.com/AlMarentu/MObs.git
//
// MObs is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#include "xmlwriter.h"
#include "xmlout.h"
#include "logging.h"


#include <stack>
#include <sstream>
#include <iostream>


using namespace std;

namespace mobs {


static const wchar_t inval = L'\u00b8';            //       INVERTED QUESTION MARK
//static const wchar_t inval = L'\uFFFD';

wchar_t to_iso_8859_1(wchar_t c) {
  return (c & ~0xff) ? inval : c;
}

wchar_t to_iso_8859_9(wchar_t c) {
  switch (c) {
    case 0x011E: return wchar_t(0xD0); //     LATIN CAPITAL LETTER G WITH BREVE
    case 0x0130: return wchar_t(0xDD); //     LATIN CAPITAL LETTER I WITH DOT ABOVE
    case 0x015E: return wchar_t(0xDE); //     LATIN CAPITAL LETTER S WITH CEDILLA
    case 0x011F: return wchar_t(0xF0); //     LATIN SMALL LETTER G WITH BREVE
    case 0x0131: return wchar_t(0xFD); //     LATIN SMALL LETTER DOTLESS I
    case 0x015F: return wchar_t(0xFE); //     LATIN SMALL LETTER S WITH CEDILLA
    case 0xD0:
    case 0xDD:
    case 0xDE:
    case 0xF0:
    case 0xFD:
    case 0xFE:
      return inval;
    default: return (c & ~0xff) ? inval : c;
  }
}

wchar_t to_iso_8859_15(wchar_t c) {
  switch (c) {
    case 0x20AC: return wchar_t(0xA4); //       EURO SIGN
    case 0x0160: return wchar_t(0xA6); //       LATIN CAPITAL LETTER S WITH CARON
    case 0x0161: return wchar_t(0xA8); //       LATIN SMALL LETTER S WITH CARON
    case 0x017D: return wchar_t(0xB4); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0x017E: return wchar_t(0xB8); //       LATIN SMALL LETTER Z WITH CARON
    case 0x0152: return wchar_t(0xBC); //       LATIN CAPITAL LIGATURE OE
    case 0x0153: return wchar_t(0xBD); //       LATIN SMALL LIGATURE OE
    case 0x0178: return wchar_t(0xBE); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    case 0xA4:
    case 0xA6:
    case 0xA8:
    case 0xB4:
    case 0xB8:
    case 0xBC:
    case 0xBD:
    case 0xBE:
      return inval;
    default: return (c & ~0xff) ? inval : c;
  }
}


class XmlWriterData {
public:
  XmlWriterData(std::wostream &str, XmlWriter::charset c, bool i) : buffer(str), cs(c), indent(i) { setConFun(); }
  XmlWriterData(XmlWriter::charset c, bool i) : cs(c), indent(i) { setConFun(); }
  std::wostream &buffer = wstrBuff;
  XmlWriter::charset cs;
  bool indent;
  int level = 0;
  bool openEnd = false;
  bool hasValue = false;
  wstring prefix;
  stack<wstring> elements;
  wstringstream wstrBuff; // buffer für u8-Ausgabe in std::string
  wchar_t (*conFun)(wchar_t) = nullptr;

  void setConFun() {
    switch (cs) {
      case XmlWriter::CS_iso8859_1: conFun = &to_iso_8859_1; break;
      case XmlWriter::CS_iso8859_9: conFun = &to_iso_8859_9; break;
      case XmlWriter::CS_iso8859_15: conFun = &to_iso_8859_15; break;
      default: break;
    }
  }
  void write(wchar_t c) {
    if (conFun and (c & ~0x7f))
      buffer << wchar_t(conFun(c));
    else
      buffer << c;
  }
  void writeIndent() {
    if (indent)
    {
      wstring s;
      s.resize(level * 2, ' ');
      buffer << s;
    }
  }
  void closeTag() {
    if (openEnd)
    {
      buffer << '>';
      openEnd = false;
    }
  }

};






XmlWriter::XmlWriter(std::wostream &str, charset c, bool indent) {
  data = new XmlWriterData(str, c, indent);
}

XmlWriter::XmlWriter(charset c, bool indent) {
  data = new XmlWriterData(c, indent);
}

XmlWriter::~XmlWriter() {
  delete data;
}

int XmlWriter::level() const { return data->level; }

void XmlWriter::writeHead() {
  wstring encoding;
  std::locale lo;
  switch (data->cs) {
    case CS_iso8859_1: encoding = L"ISO-8859-1"; break;
    case CS_iso8859_9: encoding = L"ISO-8859-9"; break;
    case CS_iso8859_15: encoding = L"ISO-8859-15"; break;
    case CS_utf8_bom:
      if (data->buffer.tellp() == 0)  // writing BOM
        data->buffer << L'\u00ef' << L'\u00bb' << L'\u00bf';
    case CS_utf8:
      lo = std::locale(data->buffer.getloc(), new std::codecvt_utf8<wchar_t, 0x10ffff, std::little_endian>);
      data->buffer.imbue(lo);
      encoding = L"UTF-8";
      break;
    case CS_utf16_be:
      if (data->buffer.tellp() == 0)  // writing BOM
        data->buffer << L'\u00fe' << L'\u00ff';
      lo = std::locale(data->buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, codecvt_mode(0)>);
      data->buffer.imbue(lo);
      encoding = L"UTF-16";
      break;
    case CS_utf16_le:
      if (data->buffer.tellp() == 0)  // writing BOM
        data->buffer << L'\u00ff' << L'\u00fe';
      lo = std::locale(data->buffer.getloc(), new std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>);
      data->buffer.imbue(lo);
      encoding = L"UTF-16";
      break;
  }
  
  data->buffer << L"<?xml";
  data->openEnd = true;
  writeAttribute(L"version", version);
  writeAttribute(L"encoding", encoding);
  writeAttribute(L"standalone", standalone ? L"yes":L"no");
  data->buffer << L"?>";
  data->openEnd = false;
}

void XmlWriter::writeTagBegin(const std::wstring &tag) {
  data->closeTag();
  if (data->indent)
    data->buffer << '\n';
  data->writeIndent();
  data->buffer << '<' << data->prefix << tag;
  data->openEnd = true;
  data->elements.push(tag);
  data->level++;
}

void XmlWriter::writeAttribute(const std::wstring &attribute, const std::wstring &value) {
  if (not data->openEnd)
    LOG(LM_WARNING, "XmlWriter::writeAttribute error");
  data->buffer << ' ' << attribute << '=' << '"';
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '"': data->buffer << L"&quot;"; break;
      case '&': data->buffer << L"&amp;"; break;
      default: data->write(c);
    }
  data->buffer << '"';
}

void XmlWriter::writeValue(const std::wstring &value) {
  data->closeTag();
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '>': data->buffer << L"&gt;"; break;
      case '&': data->buffer << L"&amp;"; break;
      default: data->write(c);
    }
  data->hasValue = true;
}

void XmlWriter::writeCdata(const std::wstring &value) {
  data->closeTag();
  data->buffer << L"<![CDATA[" << value << L"]]>";
  data->hasValue = true;
}

void XmlWriter::writeTagEnd(bool forceNoNulltag) {
  if (data->elements.empty())
    throw runtime_error("XmlWriter::writeTagEnd unbalanced");
  data->level--;
  if (data->openEnd and not forceNoNulltag)
    data->buffer << L"/>";
  else {
    if (data->indent and not data->hasValue) {
      data->buffer << '\n';
      data->writeIndent();
    }
    data->buffer << L"</" << data->prefix << data->elements.top() << '>';
  }
  if (data->indent and level() == 0)
    data->buffer << '\n';
  data->elements.pop();
  data->hasValue = false;
  data->openEnd = false;
}

void XmlWriter::writeComment(const std::wstring &value, bool inNewLine) {
  data->closeTag();
  if (data->indent and inNewLine) {
    data->buffer << '\n';
    data->writeIndent();
  }
  data->buffer << L"<!-- ";
  for (const auto c:value)
    switch (c)
    {
      case '<': data->buffer << L"&lt;"; break;
      case '>': data->buffer << L"&gt;"; break;
      default: data->write(c);
    }
  data->buffer << L" -->";
}

void XmlWriter::clearString()
{
   data->wstrBuff.clear();
}

string XmlWriter::getString() const
{
  string result;
// ist leer wenn filebuffer
  switch (data->cs) {
    case CS_iso8859_1:
    case CS_iso8859_9:
    case CS_iso8859_15: {
      const wstring &t = data->wstrBuff.str();
      std::transform(t.begin(), t.cend(), std::back_inserter(result),
                     [](const wchar_t c) -> char{ return (c & ~0xff) ? char(0xbf) : c; });
      return result;
    }
    case CS_utf8_bom:
    case CS_utf8:
      return to_string(data->wstrBuff.str());
    case CS_utf16_le:
    case CS_utf16_be:
      return "";
  }
  return "";
}




}

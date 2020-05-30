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


#include "logging.h"
#include "objtypes.h"
#include "converter.h"


#include <sstream>
#include <vector>


using namespace std;

namespace mobs {


static const wchar_t inval = L'\u00bf';            //       INVERTED QUESTION MARK
static const wchar_t winval = L'\uFFFD';

wchar_t to_iso_8859_1(wchar_t c) {
  return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
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
    default: return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
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
    default: return (c < L'\u0000' or c > L'\u00ff') ? inval : c;
  }
}

inline wchar_t from_iso_8859_1(wchar_t c) { return c; }

wchar_t from_iso_8859_9(wchar_t c) {
  switch (c) {
    case 0xD0: return wchar_t(0x011E); //     LATIN CAPITAL LETTER G WITH BREVE
    case 0xDD: return wchar_t(0x0130); //     LATIN CAPITAL LETTER I WITH DOT ABOVE
    case 0xDE: return wchar_t(0x015E); //     LATIN CAPITAL LETTER S WITH CEDILLA
    case 0xF0: return wchar_t(0x011F); //     LATIN SMALL LETTER G WITH BREVE
    case 0xFD: return wchar_t(0x0131); //     LATIN SMALL LETTER DOTLESS I
    case 0xFE: return wchar_t(0x015F); //     LATIN SMALL LETTER S WITH CEDILLA
    default: return c;
  }
}

wchar_t from_iso_8859_15(wchar_t c) {
  switch (c) {
    case 0xA4: return wchar_t(0x20AC); //       EURO SIGN
    case 0xA6: return wchar_t(0x0160); //       LATIN CAPITAL LETTER S WITH CARON
    case 0xA8: return wchar_t(0x0161); //       LATIN SMALL LETTER S WITH CARON
    case 0xB4: return wchar_t(0x017D); //       LATIN CAPITAL LETTER Z WITH CARON
    case 0xB8: return wchar_t(0x017E); //       LATIN SMALL LETTER Z WITH CARON
    case 0xBC: return wchar_t(0x0152); //       LATIN CAPITAL LIGATURE OE
    case 0xBD: return wchar_t(0x0153); //       LATIN SMALL LIGATURE OE
    case 0xBE: return wchar_t(0x0178); //       LATIN CAPITAL LETTER Y WITH DIAERESIS
    default: return c;
  }
}


codec_iso8859_1::result codec_iso8859_1::do_out(mbstate_t& state,
                                                const wchar_t* from,
                                                const wchar_t* from_end,
                                                const wchar_t*& from_next,
                                                char* to,
                                                char* to_end,
                                                char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_1(*from));
//    cerr << to_string(*from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_1::result codec_iso8859_1::do_in (state_type& state,
                                                const char* from,
                                                const char* from_end,
                                                const char*& from_next,
                                                wchar_t* to,
                                                wchar_t* to_end,
                                                wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_1(*(unsigned char*)from);
//    cerr << to_string(*to);
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_9::result codec_iso8859_9::do_out(mbstate_t& state,
                                                const wchar_t* from,
                                                const wchar_t* from_end,
                                                const wchar_t*& from_next,
                                                char* to,
                                                char* to_end,
                                                char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_9(*from));
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_9::result codec_iso8859_9::do_in (state_type& state,
                                                const char* from,
                                                const char* from_end,
                                                const char*& from_next,
                                                wchar_t* to,
                                                wchar_t* to_end,
                                                wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_9(*(unsigned char*)from);
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_15::result codec_iso8859_15::do_out(mbstate_t& state,
                                                  const wchar_t* from,
                                                  const wchar_t* from_end,
                                                  const wchar_t*& from_next,
                                                  char* to,
                                                  char* to_end,
                                                  char*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = u_char(to_iso_8859_15(*from));
  }
  to_next = to;
  from_next = from;
  return ok;
}

codec_iso8859_15::result codec_iso8859_15::do_in (state_type& state,
                                                  const char* from,
                                                  const char* from_end,
                                                  const char*& from_next,
                                                  wchar_t* to,
                                                  wchar_t* to_end,
                                                  wchar_t*& to_next) const
{
  for (; from != from_end and to != to_end; from++, to++)
  {
    *to = from_iso_8859_15(*(unsigned char*)from);
  }
  to_next = to;
  from_next = from;
  return ok;
}




int from_base64(wchar_t c) {
  if (c < 0 or c > 127)
    return -1;
  static const vector<int> b64Chars = {
          -1, -1, -1, -1, -1, -1, -1, -1, -1, 99, 99, -1, 99, 99, -1, -1,
          -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
          52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
          -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
          15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
          -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
          41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 };
  return b64Chars[c];
}

wchar_t to_base64(int i) {
  const wstring base64 = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (i < 0 or i > 63)
    return winval;
  return base64[size_t(i)];
}




wchar_t from_html_tag(const std::wstring &tok)
{
  wchar_t c = '\0';
  if (tok == L"lt")
    c = '<';
  else if (tok == L"gt")
    c = '>';
  else if (tok == L"amp")
    c = '&';
  else if (tok == L"quot")
    c = '"';
  else if (tok == L"apos")
    c = '\'';
  else if (tok[0] == L'#') {
    size_t p;
    try {
      int i = std::stoi(mobs::to_string(tok.substr(tok[1] == 'x' ? 2:1)), &p, tok[1] == 'x' ? 16:10);
      if (p == tok.length() - (tok[1] == 'x' ? 2:1) and
          (i == 9 or i == 10 or i == 13 or (i >= 32 and i <= 0xD7FF) or
           (i >= 0xE000 and i <= 0xFFFD) or (i >= 0x10000 and i <= 0x10FFFF)))
        c = i;
    } catch (...) {}
  }
  return c;
}

std::wstring toLower(const std::wstring &tx) {
  std::locale loc;
  try {
    const char *cp = getenv("LANG");
    if (cp)
      loc = std::locale(cp);
    else
      loc = std::locale("de_DE.UTF-8");
  } catch (...) {
    loc = std::locale();
  }
  LOG(LM_DEBUG, "LOCALE = " << loc.name());
  wstring lo;
  lo.reserve(tx.length());
  std::transform(tx.begin(), tx.end(), std::back_inserter(lo), [loc](const wchar_t c) { return std::tolower(c, loc);} );
  return lo;
}

std::wstring toUpper(const std::wstring &tx) {
  std::locale loc;
  try {
    const char *cp = getenv("LANG");
    if (cp)
      loc = std::locale(cp);
    else
      loc = std::locale("de_DE.UTF-8");
  } catch (...) {
    loc = std::locale();
  }
  LOG(LM_DEBUG, "LOCALE = " << loc.name());
  wstring lo;
  lo.reserve(tx.length());
  std::transform(tx.begin(), tx.end(), std::back_inserter(lo), [loc](const wchar_t c) { return std::toupper(c, loc);} );
  return lo;
}

std::string toLower(const string &tx) {
  return mobs::to_string(mobs::toLower(mobs::to_wstring(tx)));
}

std::string toUpper(const string &tx) {
  return mobs::to_string(mobs::toUpper(mobs::to_wstring(tx)));
}


void Base64Reader::start() {
  base64.clear();
  b64Cnt = 0;
  b64Value = 0;
}

void Base64Reader::done() {
  if (b64Cnt > 0 and b64Cnt < 4)
    put('=');
}

#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
void Base64Reader::put(wchar_t c) {
  int v = from_base64(c);
  if (v < 0) {
    if (c == '=') { // padding
      switch (b64Cnt) {
        case 3:
          base64.push_back(b64Value >> 10);
          base64.push_back((b64Value >> 2) & 0xff);
          // fall into
        case 100:
          b64Cnt = 999; // Wenn noch ein = kommt -> fehler
          break;
        case 2:
          base64.push_back(b64Value >> 4);
          b64Cnt = 100; // es darf noch 1 = kommen
          break;
        case 1: throw std::runtime_error("base64 unexpected end");
        default: throw std::runtime_error("base64 unexpected padding");
      }
    }
    else
      throw std::runtime_error("base64 padding");
  }
  else if (v < 64) {
    if (b64Cnt > 3)
      throw std::runtime_error("base64 invalid");
    b64Value = (b64Value << 6) + v;
    if (++b64Cnt == 4) {
      base64.push_back(b64Value >> 16);
      base64.push_back((b64Value >> 8) & 0xff);
      base64.push_back(b64Value & 0xff);
      b64Cnt = 0;
      b64Value = 0;
    }
  }
}
#pragma clang diagnostic pop



bool StrConv<std::vector<u_char>>::c_string2x(const std::string &str, std::vector<u_char> &t, const ConvFromStrHint &) {
  try {
    Base64Reader r(t);
    for (auto c:str)
      r.put(c);
    r.done();
  }
  catch (exception &e) {
    LOG(LM_INFO, "Erro, in base64: " << e.what());
    return false;
  }
  return true;
}
bool StrConv<std::vector<u_char>>::c_wstring2x(const std::wstring &wstr, std::vector<u_char> &t, const ConvFromStrHint &) {
  try {
    Base64Reader r(t);
    for (auto c:wstr)
      r.put(c);
    r.done();
  }
  catch (exception &e) {
    LOG(LM_INFO, "Error, in base64: " << e.what());
    return false;
  }
  return true;
}


std::string StrConv<std::vector<u_char>>::c_to_string(const std::vector<u_char> &t, const ConvToStrHint &cts) {
  std::string u;
  copy_base64(t.cbegin(), t.cend(), std::back_inserter(u), cts.withIndentation() ? "\n  ":"");
  return u;
}
std::wstring StrConv<std::vector<u_char>>::c_to_wstring(const std::vector<u_char> &t, const ConvToStrHint &cts) {
  std::wstring u;
  copy_base64(t.cbegin(), t.cend(), std::back_inserter(u), cts.withIndentation() ? "\n  ":"");
  return u;
}


}

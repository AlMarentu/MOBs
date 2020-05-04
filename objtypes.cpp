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

#include "objgen.h"
#include <codecvt>

//#include <iostream>

using namespace std;


namespace mobs {

std::wstring to_wstring(std::string val) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  return c.from_bytes(val);
};

std::u32string to_u32string(std::string val) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  return c.from_bytes(val);
};

std::string to_quote(const std::string &s) {
  string result = "\"";
  if (s.length() > 1 or s[0] != 0)
  {
    size_t pos = 0;
    size_t pos2 = 0;
    while ((pos = s.find('"', pos2)) != string::npos)
    {
      result += s.substr(pos2, pos - pos2) + "\\\"";
      pos2 = pos+1;
    }
    result += s.substr(pos2);
  }
  result += '"';
  return result;
}

template<>
/// \private
bool string2x(const std::string &str, char32_t &t) {
  std::u32string s = mobs::to_u32string(str);
  if (s.length() > 1)
    return false;
  t = s.empty() ? U'\U00000000' : s[0];
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = c;
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, signed char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = c;
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, unsigned char &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffffff00))
    return false;
  t = c;
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, char16_t &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if ((c & 0xffff0000))
    return false;
  t = c;
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, wchar_t &t) {
  char32_t c;
  if (not string2x(str, c))
    return false;
  if (numeric_limits<wchar_t>::digits <= 16 and (c & 0xffff00))
    return false;
  t = c;
  return true;
};

template<>
/// \private
bool string2x(const std::string &str, bool &t) {
  if (str == "true")
    t = true;
  else if (str == "false")
    t = false;
  else
    return false;
  return true;
}

template<>
bool string2x(const std::string &str, u32string &t) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  t = c.from_bytes(str);
  return true;
}

template<>
bool string2x(const std::string &str, u16string &t) {
  std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> c;
  t = c.from_bytes(str);
  return true;
}

template<>
bool string2x(const std::string &str, wstring &t) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  t = c.from_bytes(str);
  return true;
}


template <>
bool wstring2x(const std::wstring &wstr, std::u32string &t) {
  std::copy(wstr.cbegin(), wstr.cend(), back_inserter(t));
  return true;
}

template <>
bool wstring2x(const std::wstring &wstr, std::u16string &t) {
  std::copy(wstr.cbegin(), wstr.cend(), back_inserter(t));
  return true;
}


std::string to_string(std::u32string t) {
  std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
  return c.to_bytes(t);
}

std::string to_string(std::u16string t) {
  std::wstring_convert<std::codecvt_utf8<char16_t>,char16_t> c;
  return c.to_bytes(t);
}

std::string to_string(std::wstring t) {
  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c;
  return c.to_bytes(t);
}

std::string to_string(float t) {
  std::stringstream str;
  str << t;
  return str.str();
}

std::string to_string(double t ){
  std::stringstream str;
  str << t;
  return str.str();
}

std::string to_string(long double t) {
  std::stringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(float t) {
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(double t ){
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(long double t) {
  std::wstringstream str;
  str << t;
  return str.str();
}

std::wstring to_wstring(const std::u32string &t) {
  wstring result;
  std::transform(t.cbegin(), t.cend(), std::back_inserter(result),
  [](const wchar_t c) { return (c < 0 or c > 0x10FFFF) ? L'\uFFFD' : c; });
//  copy(t.cbegin(), t.cend(), back_inserter(result));
  return result;
}

std::wstring to_wstring(const std::u16string &t) {
  wstring result;
  std::transform(t.cbegin(), t.cend(), std::back_inserter(result),
  [](const char16_t c) -> wchar_t{ return (c < 0) ? L'\uFFFD' : c; });
//  copy(t.cbegin(), t.cend(), back_inserter(result));
  return result;
}


static const wchar_t inval = L'\u00bf';            //       INVERTED QUESTION MARK
static const wchar_t winval = L'\uFFFD';

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

static vector<int> b64Chars = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, 99, 99, -1, 99, 99, -1, -1,
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  99, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
  -1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
  -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
  41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1 };

int from_base64(wchar_t c) {
  if (c < 0 or c > 127)
    return -1;

  return b64Chars[c];
}

wchar_t to_base64(int i) {
  const wstring base64 = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  if (i < 0 or i > 63)
    return winval;
  return base64[size_t(i)];
}




template<>
bool to_int64(int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<int>::min(); max = std::numeric_limits<int>::max(); return true; }
template<>
bool to_int64(short int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<short int>::min(); max = std::numeric_limits<short int>::max(); return true; }
template<>
bool to_int64(long int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<long int>::min(); max = std::numeric_limits<long int>::max(); return true; }
template<>
bool to_int64(long long int t, int64_t &i, int64_t &min, uint64_t &max)
{ i = t; min = std::numeric_limits<long long int>::min(); max = std::numeric_limits<long long int>::max(); return true; }
template<>
bool to_uint64(unsigned int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned int>::max(); return true; }
template<>
bool to_uint64(unsigned short int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned short int>::max(); return true; }
template<>
bool to_uint64(unsigned long int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned long int>::max(); return true; }
template<>
bool to_uint64(unsigned long long int t, uint64_t &u, uint64_t &max)
{ u = t; max = std::numeric_limits<unsigned long long int>::max(); return true; }
template<>
bool to_uint64(bool t, uint64_t &u, uint64_t &max)
{ u = t ? 1:0; max = 1; return true; }
template<>
bool to_double(double t, double &d) { d = t; return true; }
template<>
bool to_double(float t, double &d) { d = t; return true; }
//template<>
//bool to_double(long double t, double &d) { d = t; return true; }

template<>
bool from_number(int64_t i, int &t) {
  if (i > std::numeric_limits<int>::max() or i < std::numeric_limits<int>::min()) return false;
  t = ( int)i;
  return true;
}
template<>
bool from_number(int64_t i, short int &t) {
  if (i > std::numeric_limits<short int>::max() or i < std::numeric_limits<short int>::min()) return false;
  t = (short int)i;
  return true;
}
template<>
bool from_number(int64_t i, long int &t) {
  if (i > std::numeric_limits<long int>::max() or i < std::numeric_limits<long int>::min()) return false;
  t = (long int)i;
  return true;
}
template<>
bool from_number(int64_t i, long long int &t) {
  if (i > std::numeric_limits<long long int>::max() or i < std::numeric_limits<long long int>::min()) return false;
  t = (long long int)i;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned int &t) {
  if (u > std::numeric_limits<unsigned int>::max()) return false;
  t = (unsigned int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned short int &t) {
  if (u > std::numeric_limits<unsigned short int>::max()) return false;
  t = (unsigned short int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned long int &t) {
  if (u > std::numeric_limits<unsigned long int>::max()) return false;
  t = (unsigned long int)u;
  return true;
}
template<>
bool from_number(uint64_t u, unsigned long long int &t) {
  if (u > std::numeric_limits<unsigned long long int>::max()) return false;
  t = (unsigned long long int)u;
  return true;
}
template<>
bool from_number(uint64_t u, bool &t) {
  if (u > 1) return false;
  t = u != 0;
  return true;
}
template<>
bool from_number(double d, float &t) { t = d; return true; }
template<>
bool from_number(double d, double &t) { t = d; return true; }



class ConvFromStrHintDefault : virtual public ConvFromStrHint {
public:
  ConvFromStrHintDefault() {}
  virtual ~ConvFromStrHintDefault() {}
  virtual bool acceptCompact() const { return true; }
  virtual bool acceptExtented() const { return true; }
};

class ConvFromStrHintExplizit : virtual public ConvFromStrHint {
public:
  ConvFromStrHintExplizit() {}
  virtual ~ConvFromStrHintExplizit() {}
  virtual bool acceptCompact() const { return false; }
  virtual bool acceptExtented() const { return true; }
};

const ConvFromStrHint &ConvFromStrHint::convFromStrHintDflt = ConvFromStrHintDefault();
const ConvFromStrHint &ConvFromStrHint::convFromStrHintExplizit = ConvFromStrHintExplizit();


}

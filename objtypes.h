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


#ifndef MOBS_OBJTYPES_H
#define MOBS_OBJTYPES_H

#include <string>
#include <iostream>
#include <sstream>
#include <codecvt>
#include <vector>
#include <limits>


/** \file objtypes.h
 \brief Definitionen von Konvertierungsroutinen von und nach std::string */

/**
 \brief Deklaration einses \c enum
 
 Im Beispiel wird ein enum direction {Dleft, Dright, Dup, Ddown};
 deklariert. Zusätzlich wird eine Konvertierungsklasse \c StrdirectionConv sowie 2 Hilfs-Arrays erzeugt.
 \code
 MOBS_ENUM_DEF(direction, Dleft, Dright, Dup, Ddown)
 MOBS_ENUM_VAL(direction, "left", "right", "up", "down")
 \endcode
*/
#define MOBS_ENUM_DEF(typ, ... ) \
enum typ { __VA_ARGS__ }; \
namespace mobs { const std::vector<enum typ> mobsinternal_elements_##typ = { __VA_ARGS__ }; }
/// Deklaration der Ausgabewerte zu einem \c MOBS_ENUM_DEF
#define MOBS_ENUM_VAL(typ, ... ) \
namespace mobs { \
const std::vector<std::string> mobsinternal_text_##typ = { __VA_ARGS__ }; \
std::string typ##_to_string(enum typ x) { size_t p = 0; for (auto const i:mobsinternal_elements_##typ) { if (i==x) { return mobsinternal_text_##typ.at(p); } p++;} throw std::runtime_error(#typ "to_string fails"); }; \
bool string_to_##typ(std::string s, enum typ &x) { size_t p = 0; for (auto const &i:mobsinternal_text_##typ) { if (i==s) { x = mobsinternal_elements_##typ.at(p); return true; } p++;} return false; };  \
class Str##typ##Conv { \
public: \
  static inline bool c_string2x(const std::string &str, typ &t, const ::mobs::ConvFromStrHint &cfh) { if (cfh.acceptExtented()) { if (string_to_##typ(str, t)) return true; } \
                                                    if (not cfh.acceptCompact()) return false; int i; if (not ::mobs::string2x(str, i)) return false; t = typ(i); return true; } \
  static inline bool c_wstring2x(const std::wstring &wstr, typ &t, const ::mobs::ConvFromStrHint &cfh) { return c_string2x(::mobs::to_string(wstr), t, cfh); } \
  static inline std::string c_to_string(typ t, const ::mobs::ConvToStrHint &cth) { return cth.compact() ? ::mobs::to_string(int(t)) : typ##_to_string(t); } \
  static inline bool c_is_chartype(const ::mobs::ConvToStrHint &cth) { return not cth.compact(); } \
  static inline bool c_is_specialized() { return false; } \
  static inline typ c_empty() { return mobsinternal_elements_##typ.front(); } \
}; }


namespace mobs {

/// String-Konvertierung
/// @param val Wert in utf8
/// @return Wert als u32string
inline std::u32string to_u32string(std::string val) { std::wstring_convert<std::codecvt_utf8<char32_t>,char32_t> c;
return c.from_bytes(val); };

/// String-Konvertierung
/// @param val Wert in utf8
/// @return Wert als wstring
inline std::wstring to_wstring(std::string val) { std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> c; return c.from_bytes(val); };

//template <typename T>
//std::string to_string(T t) { std::stringstream s; s << t; return s.str(); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(std::string t) { return t; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::u32string t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::u16string t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(std::wstring t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char t) { return std::string() + t; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(signed char t) { return to_string(char(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned char t) { return to_string(char(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char16_t t) { return to_string(std::u16string() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(char32_t t) { return to_string(std::u32string() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(wchar_t t) { return to_string(std::wstring() + t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(short int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(long long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned short int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(unsigned long long int t) { return std::to_string(t); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(bool t) { return t ? u8"true":u8"false"; };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char *t) { return to_string(std::string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const wchar_t *t) { return to_string(std::wstring(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char16_t *t) { return to_string(std::u16string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
inline std::string to_string(const char32_t *t) { return to_string(std::u32string(t)); };
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(float t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(double t);
/// \brief Konvertierung nach std::string
/// @param t Wert
/// @return Wert als std::string
std::string to_string(long double t);

template <typename T>
/// \brief Konvertierung von std::string
/// @param str Konvertierter Wert
/// @param t Wert
/// @return true, wenn fehlerfrei
inline bool string2x(const std::string &str, T &t) {std::stringstream s; s.str(str); s >> t; return s.eof() and not s.bad() and not s.fail(); }
/// \private
template <> inline bool string2x(const std::string &str, std::string &t) { t = str; return true; }
/// \private
template <> bool string2x(const std::string &str, std::wstring &t);
/// \private
template <> bool string2x(const std::string &str, std::u32string &t);
/// \private
template <> bool string2x(const std::string &str, std::u16string &t);
/// \private
template <> bool string2x(const std::string &str, char &t);
/// \private
template <> bool string2x(const std::string &str, signed char &t);
/// \private
template <> bool string2x(const std::string &str, unsigned char &t);
/// \private
template <> bool string2x(const std::string &str, char16_t &t);
/// \private
template <> bool string2x(const std::string &str, char32_t &t);
/// \private
template <> bool string2x(const std::string &str, wchar_t &t);
/// \private
template <> bool string2x(const std::string &str, bool &t);

template <typename T>
/// \brief Konvertierung von std::wstring
/// @param wstr Konvertierter Wert
/// @param t Wert
/// @return true, wenn fehlerfrei
inline bool wstring2x(const std::wstring &wstr, T &t) {std::stringstream s; s.str(mobs::to_string(wstr)); s >> t; return s.eof() and not s.bad() and not s.fail(); }
/// \private
template <> inline bool wstring2x(const std::wstring &wstr, std::string &t) { t = to_string(wstr); return true; }
/// \private
template <> inline bool wstring2x(const std::wstring &wstr, std::wstring &t) { t = wstr; return true; }
/// \private
template <> bool wstring2x(const std::wstring &wstr, std::u32string &t);
/// \private
template <> bool wstring2x(const std::wstring &wstr, std::u16string &t);

// ist typename ein Character-Typ
template <typename T>
/// \brief prüft, ob der Typ T aus Text besteht - also zB. in JSON in Hochkommata steht
inline bool mobschar(T) { return not std::numeric_limits<T>::is_specialized; }
/// \private
template <> inline bool mobschar(char) { return true; };
/// \private
template <> inline bool mobschar(char16_t) { return true; };
/// \private
template <> inline bool mobschar(char32_t) { return true; };
/// \private
template <> inline bool mobschar(wchar_t) { return true; };
/// \private
template <> inline bool mobschar(unsigned char) { return true; };
/// \private
template <> inline bool mobschar(signed char) { return true; };

/// Hilfsklasse für Konvertierungsklasse
class ConvToStrHint {
public:
  ConvToStrHint() = delete;
  /// Konstructor
  /// @param print_compact führt bei einigen Typen zur vereinfachten Ausgabe
  /// @param altNames verwende alternative Namen wenn VOrhanden
  ConvToStrHint(bool print_compact, bool altNames = false) : comp(print_compact), altnam(altNames) {}
  virtual ~ConvToStrHint() {}
  /// \private
  virtual bool compact() const { return comp; }
  /// \private
  virtual bool useAltNames() const { return altnam; }

protected:
  /// \private
  bool comp;
  /// \private
  bool altnam;
};

/// Hilfsklasse für Konvertierungsklasse - Basisklasse
class ConvFromStrHint {
public:
  virtual ~ConvFromStrHint() {}
  /// darf ein nicht-kompakter Wert als Eingabe fungieren
  virtual bool acceptExtented() const = 0;
  /// darf ein kompakter Wert als Eingabe fungieren
  virtual bool acceptCompact() const = 0;

  /// Standard Konvertierungshinweis, kompake und erweiterte Eingaben sind erlaubt
  static const ConvFromStrHint &convFromStrHintDflt;
  /// Konvertierungshinweis, der nur erweiterte Eingabe zulässt
  static const ConvFromStrHint &convFromStrHintExplizit;

};

/// Ausgabeformat für \c to_string() Methode von Objekten
class ConvObjToString : virtual public ConvToStrHint {
public:
  ConvObjToString() : ConvToStrHint(false) {}
  /// \private
  bool toXml() const { return xml; }
  /// \private
  bool toJson() const { return not xml; }
  /// \private
  bool withQuotes() const { return quotes; }
  /// \private
  bool withIndentation() const { return indent; }
  /// \private
  bool omitNull() const { return onull; }
  /// Ausgabe als XML-Datei
  ConvObjToString exportXml() const { ConvObjToString c(*this); c.xml = true; return c; }
  /// Ausgabe als JSON
  ConvObjToString exportJson() const { ConvObjToString c(*this); c.xml = false; c.quotes = true; return c; }
  /// Verwende alternative Namen
  ConvObjToString exportAltNames() const { ConvObjToString c(*this); c.altnam = true; return c; }
  /// Export mit Einrückungen 
  ConvObjToString doIndent() const { ConvObjToString c(*this); c.indent = true; return c; }
  /// Export ohne Einrückungen
  ConvObjToString noIndent() const { ConvObjToString c(*this); c.indent = false; return c; }
  /// Verwende native Bezeichnmer von enums und Zeiten
  ConvObjToString exportCompact() const { ConvObjToString c(*this); c.comp = true; return c; }
  /// Ausgabe im Klartext von enums und Uhrzeit
  ConvObjToString exportExtendet() const { ConvObjToString c(*this); c.comp = false; return c; }
  /// Ausgabe von null-Werten überspringen
  ConvObjToString exportWoNull() const { ConvObjToString c(*this); c.onull = true; return c; }
private:
  bool xml = false;
  bool quotes = false;
  bool indent = false;
  bool onull = false;

};

/// Konfiguration für \c string2Obj
class ConvObjFromStr : virtual public ConvFromStrHint {
public:
  /// Enums für Behandlung NULL-Werte
  enum Nulls { ignore, omit, clear, force, except };
  virtual ~ConvObjFromStr() {}
  /// Eingabe ist im XML-Format
  virtual bool acceptXml() const { return xml; }
  /// darf ein kompakter Wert als Eingabe fungieren
  virtual bool acceptCompact() const { return compact; }
  /// darf ein nicht-kompakter Wert als Eingabe fungieren
  virtual bool acceptExtented() const { return extented; };
  /// Verwende alternative Namen
  virtual bool acceptAltNames() const { return altNam; };
  /// Verwende original Namen
  virtual bool acceptOriNames() const { return oriNam; };
  /// setze Arraysize auf letztes Element
  virtual bool shrinkArray() const { return shrink; };
  /// werfe eine Exception bei unbekannter Variable
  virtual bool exceptionIfUnknown() const { return exceptUnk; };
  /// Null-Werte behandeln
  virtual enum Nulls nullHandling() const { return null; };
  /// Verwenden den XML-Parser
  ConvObjFromStr useXml() const { ConvObjFromStr c(*this); c.xml = true; return c; }
  /// Werte in Kurzform akzeptieren (zB. Zahl anstatt enum-Text)
  ConvObjFromStr useCompactValues() const { ConvObjFromStr c(*this); c.compact = true; c.extented = false; return c; }
  /// Werte in Langform akzeptieren (zB. enum-Text, Datum)
  ConvObjFromStr useExtentedValues() const { ConvObjFromStr c(*this); c.compact = false; c.extented = true; return c; }
  /// Werte in beliebiger Form akzeptieren
  ConvObjFromStr useAutoValues() const { ConvObjFromStr c(*this); c.compact = true; c.extented = true; return c; }
  /// Nur Original-Namen akzeptieren
  ConvObjFromStr useOriginalNames() const { ConvObjFromStr c(*this); c.oriNam = true; c.altNam = false; return c; }
  /// Nur Alternativ-Namen akzeptieren
  ConvObjFromStr useAlternativeNames() const { ConvObjFromStr c(*this); c.oriNam = false; c.altNam = true; return c; }
  /// Original oder Alternativ-Namen akzeptieren
  ConvObjFromStr useAutoNames() const {  ConvObjFromStr c(*this); c.oriNam = true; c.altNam = true; return c; }
  /// Vektoren beim Schreiben entsprechens verkleinern
  ConvObjFromStr useDontShrink() const {  ConvObjFromStr c(*this); c.shrink = false; return c; }
  /// null-Elemente werden beim Einlesen abhängig von \c nullAlllowed  auf null gesetzt im anderen Fall wird statt den Fehler zu ignorieren, eine exception geworen
  ConvObjFromStr useExceptNull() const {  ConvObjFromStr c(*this); c.null = except; return c; }
  /// null-Elemente werden beim Einlesen üblesen
  ConvObjFromStr useOmitNull() const {  ConvObjFromStr c(*this); c.null = omit; return c; }
  /// null-Elemente werden beim Einlesen anhand \c nullAlllowed behandelt
  ConvObjFromStr useClearNull() const {  ConvObjFromStr c(*this); c.null = clear; return c; }
  /// null-Elemente werden beim Einlesen unabhängig von \c nullAlllowed  auf null gesetzt
  ConvObjFromStr useForceNull() const {  ConvObjFromStr c(*this); c.null = force; return c; }
  /// wird versucht eine unbekannte Variable einzulesen, wird eine exception geworfen
  ConvObjFromStr useExceptUnknown() const {  ConvObjFromStr c(*this); c.exceptUnk = true; return c; }
protected:
  /// \private
  bool xml = false;
  /// \private
  bool compact = true;
  /// \private
  bool extented = true;
  /// \private
  bool oriNam = true;
  /// \private
  bool altNam = false;
  /// \private
  bool shrink = true;
  /// \private
  bool exceptUnk = false;
  /// \private
  enum Nulls null = ignore;
};

template <typename T>
/// Standard Konvertierungs-Klasse für Serialisierung von und nach std::string
class StrConv {
public:
  /// liest eine Variable aus einem \c std::string
  static inline bool c_string2x(const std::string &str, T &t, const ConvFromStrHint &) { return mobs::string2x(str, t); }
  /// liest eine Variable aus einem \c std::wstring
  static inline bool c_wstring2x(const std::wstring &wstr, T &t, const ConvFromStrHint &) { return mobs::string2x(mobs::to_string(wstr), t); }
  /// Wandelt eine Variable in einen \c std::string um
  static inline std::string c_to_string(T t, const ConvToStrHint &) { return to_string(t); };
  /// Angabe, ob die Ausgabe als Text erfolgt (quoting, escaping nötig)
  static inline bool c_is_chartype(const ConvToStrHint &) { return mobschar(T()); }
  /// zeigt an, ob spezialisierter Typ vorliegt (\c \<limits>)
  static inline bool c_is_specialized() { return std::numeric_limits<T>::is_specialized; }
  /// liefert eine \e leere Variable die zum löschen oder Initialisieren einer Membervarieblen verwendet wird
  /// \see clear()
  static inline T c_empty() { return T(); }

};

template <typename T>
/// Konvertiertungs-Klasse für enums mit Ein-/Ausgabe als \c int
class StrIntConv {
public:
  /// \private
  static inline bool c_string2x(const std::string &str, T &t, const ConvFromStrHint &) { int i; if (not mobs::string2x(str, i)) return false; t = T(i); return true; }
  /// \private
  static inline bool c_wstring2x(const std::wstring &wstr, T &t, const ConvFromStrHint &) { int i; if (not mobs::wstring2x(wstr, i)) return false; t = T(i); return true; }
  /// \private
  static inline std::string c_to_string(T t, const ConvToStrHint &) { return to_string(int(t)); }
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &) { return false; }
  /// \private
  static inline bool c_is_specialized() { return std::numeric_limits<T>::is_specialized; }
  /// \private
  static inline T c_empty() { return T(0); }
};


}

#endif

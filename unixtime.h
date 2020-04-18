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


#ifndef MOBS_DATETIME_H
#define MOBS_DATETIME_H

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

#include "objtypes.h"

/** \file unixtime.h
 \brief Optional: Wrapper für die Unix-Zeit \c time_t */

#include <time.h>
#include <stdio.h>


namespace mobs {
/** \brief Klasse zur Behandlung von sekundengenauen Zeiten
 
 verwendet intern \c time_t und ist somit nur ab 1.1.1970 0:00 UTC definiert
  
 \todo fehlende exceptions bei unsinniger Eingabe
 
 */
class UxTime {
public:
  /// Konstruktor über unix-Zeit \c time_t
  UxTime(time_t t = -1) : m_time(t) {}
  /// \brief Konstruktor für lokale Zeit
  /// @param year  Jahr >= 1900
  /// @param month Monat 1..12
  /// @param day Tag 1..31
  /// @param hour 0..23
  /// @param minute 0..59
  /// @param second 0..59
  /// \throw std::runtime_error bei (einigen) Falscheingaben
  UxTime(int year, int month, int day, int hour = 0, int minute = 0, int second = 0);
  /// \brief Konstruktor als \c std::string, im Format ISO8601
  /// @param s Zeit im Format ISO8601 mit oder ohne Zeitoffset
  /// \throws std::runtime_error bei Fehler
  UxTime(std::string s);
  /// Ausgabe der unix-zeit in \c time_t
  inline time_t toUxTime() const { return m_time; }
  /// Ausgabe der Zeit im Format ISO8601 als localtime mit Offset (2007-04-05T12:30:00+02:00)
  std::string toISO8601() const;
  /// liefert aktuelle Uhrzeit
  static UxTime now() { return UxTime(time(0)); }
private:
  void parseChar(char c, const char *&cp);
  int parseDigit(const char *&cp);
  void parseInt2(int &i, const char *&cp);
  void parseYear(int &y, const char *&cp);
  void parseOff(long &i, const char *&cp);
  time_t m_time;
};

/// ermittelt eine Zeitdifferenz analog \c difftime
inline double operator-(UxTime t1, UxTime t2) {return ::difftime(t1.toUxTime(), t2.toUxTime()); }

/// \private
template <> bool string2x(const std::string &str, UxTime &t);

/// Konvertier-Funktion \c UxTime nach \c std::string im Format ISO8601
inline std::string to_string(UxTime t) {
  return t.toISO8601();
}

template <>
/// Konvertiertungs-Klasse für \c UxTime um sie mit Mobs verwenden zu können
class StrConv <UxTime> {
public:
  /// \private
  static inline bool c_string2x(const std::string &str, UxTime &t, const ConvFromStrHint &) { return mobs::string2x(str, t); }
  /// \private
  static inline std::string c_to_string(UxTime t, const ConvToStrHint &cth) { if (cth.compact()) return to_string(t.toUxTime()); return to_string(t); };
  /// \private
  static inline bool c_is_chartype(const ConvToStrHint &cth) { return not cth.compact(); }
  /// \private
  static inline bool c_is_specialized() { return false; }
  /// \private
  static inline UxTime c_empty() { return UxTime(); }
};
  

}

#endif
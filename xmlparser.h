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

/** \file xmlparser.h
\brief EInfacher XML-Parser */


#ifndef MOBS_XMLPARSER_H
#define MOBS_XMLPARSER_H

#include "logging.h"

#include<stack>
#include<exception>
#include<iostream>

namespace mobs {

/*! \class XmlParser
 \brief Einfacher XML-Parser.
 Virtuelle Basisklasse mit Callback-Funktionen. Die Tags werden nativ geparst,
 es erfolgt keine Zeichenumwandlung (\&lt; usw.); Die werde werden implace zurückgeliefert
 Im Fehlerfall werden exceptions geworfen.
 \code
 concept XmlParser {
 XmlParser(const std::string &input);
 
 // Starte Parser
 void parse();
 
 // Callback Funktionen
 void NullTag(const std::string &element);
 void Attribut(const std::string &element, const std::string &attribut, const std::string &value);
 void Value(const std::string &value);
 void Cdata(const char *value, size_t len);
 void StartTag(const std::string &element);
 void EndTag(const std::string &element);
 void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value);
 };
 \endcode
 */




/// Einfacher XML-Parser - Basisklasse
class XmlParser  {
public:
  /*! Konstruktor der XML-Parser Basisklasse
   @param input XML-Text der geparst werden soll   */
  XmlParser(const std::string &input) : Xml(input) {
    pos1 = pos2 = posS = posE = 0;
  };
  virtual ~XmlParser() { };
  
  /*! \brief Liefert XML-Puffer und aktuelle Position für detaillierte Fehlermeldung
   @param pos Position des Fehlers im Xml-Buffer
   @return zu parsender Text-Buffer
   */
  const std::string &info(size_t &pos) const {
    pos = pos1;
    return Xml;
  };
  /** \brief zugriff auf den Stack der Element-Struktur
   */
  const std::stack<std::string> &tagPath() const { return tags; };
  
  /** \brief Callback-Function: Ein Tag ohne Inhalt, impliziert EndTag(..)
   @param element Name des Elementes
   */
  virtual void NullTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Atributwert einses Tags
   @param element Name des Elementes
   @param attribut Name des Attributes
   @param value Wert des Attributes
   */
  virtual void Attribut(const std::string &element, const std::string &attribut, const std::string &value) = 0;
  /** \brief Callback-Function: Ein Inhalt eines Tags
   @param value Inhalt des Tags
   */
  virtual void Value(const std::string &value) = 0;
  /** \brief Callback-Function: Ein CDATA-Elemet
   @param value Inhalt des Tags
   @param len Länge des Tags
   */
  virtual void Cdata(const char *value, size_t len) = 0;  // TODO bei C++17 besser std::string_view
  /** \brief Callback-Function: Ein Start-Tag
   @param element Name des Elementes
   */
  virtual void StartTag(const std::string &element) = 0;
  /** \brief Callback-Function: Ein Ende-Tag , jedoch nicht bei NullTag(..)
   @param element Name des Elementes
   */
  virtual void EndTag(const std::string &element) = 0;
  /** \brief Callback-Function: Eine Verarbeitungsanweisung z.B, "xml", "encoding", "UTF-8"
   @param element Name des Tags
   @param attribut Name der Verarbeitungsanweisung
   @param value Inhalz der Verarbeitungsanweisung
   */
  virtual void ProcessingInstruction(const std::string &element, const std::string &attribut, const std::string &value) = 0;
  
  /// Starte den Parser
  void parse() {
    TRACE("");
    parse2LT();
    if (pos2 != 0)
      throw std::runtime_error(u8"Syntax Head");
    eat('<');
    if (peek() == '?')
    {
      // Parse primäre Verarbeitungsanweisung
      eat();
      parse2GT();
      if (getValue() != "xml")
        throw std::runtime_error(u8"Syntax");
      for (;peek() != '?';)
      {
        eat(' ');
        parse2GT();
        std::string a = getValue();
        eat('=');
        char c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        parse2Char(c);
        std::string v = getValue();
        eat(c);
        ProcessingInstruction("xml", a, v);
      }
      eat('?');
      eat('>');
      parse2LT();
    }
    // eigentliches Parsing
    for (;pos2 != std::string::npos;)
    {
      saveValue();
      eat('<');
      
      if (peek() == '/')
      {
        // Parse End-Tag
        eat();
        parse2GT();
        std::string element = getValue();
        if (element.empty())
          throw std::runtime_error("missing tag E");
        if (lastKey == element)
        {
          Value(decode(posS, posE));
          clearValue();
          lastKey = "";
        }
        EndTag(element);
        if (tags.empty())
          throw std::runtime_error(u8"unexpected closing tag " + element);
        if (tags.top() != element)
          throw std::runtime_error(u8"unmatching tag " + element + " expected " + tags.top());
        tags.pop();
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '!')
      {
        eat();
        // Parse CDATA Element
        if (peek() == '[')
        {
          eat('[');
          eat('C');
          eat('D');
          eat('A');
          eat('T');
          eat('A');
          eat('[');
          parse2CD();
          saveValue();
          Cdata(&Xml[posS], posE-posS);
          clearValue();
          lastKey = "";
          eat();
          eat();
        }
        else
        {
          // Parse Kommentar
          eat('-');
          eat('-');
          parse2Com();
        }
        eat('>');
        parse2LT();
        continue;
      }
      else if (peek() == '?')
      {
        // Parse Verarbeitungsanweisung
        eat();
        parse2GT();
        std::string element = getValue();
        for (;;)
        {
          if (peek() == '?')
          {
            eat();
            ProcessingInstruction(element, "", "");
            break;
          }
          eat(' ');
          parse2GT();
          std::string a = getValue();
          std::string v;
          if (peek() == '=')
          {
            eat('=');
            char c = peek();
            if (c == '"')
              eat('"');
            else
              eat('\'');
            parse2Char(c);
            v = getValue();
            eat(c);
          }
          ProcessingInstruction(element, a, v);
        }
        eat('>');
        parse2LT();
        continue;
      }
      // Parse Element-Beginn
      parse2GT();
      std::string element = getValue();
      if (element.empty())
        throw std::runtime_error("missing tag B");
      tags.push(element);
      StartTag(element);
      for (;;)
      {
        if (peek() == '>')  // Ende eines Starttags
        {
          eat();
          parse2LT();
          break;
        }
        else if (peek() == '/') // Leertag
        {
          eat();
          eat('>');
          NullTag(element);
          tags.pop();
          parse2LT();
          break;
        }
        eat(' ');
        parse2GT();
        std::string a = getValue();
        eat('=');
        char c = peek();
        if (c == '"')
          eat('"');
        else
          eat('\'');
        parse2Char(c);
        std::string v = getValue();
        eat(c);
        Attribut(element, a, v);
      }
      lastKey = element;
    }
    pos2 = Xml.length();
    saveValue();
    // nur noch für check Whitespace bis eof
    saveValue();
    if (not tags.empty())
      throw std::runtime_error(u8" expected tag at EOF: " + tags.top());
  };
  
private:
  
  void parse2LT() {
    pos2 = Xml.find('<', pos1);
    //cerr << "PLT " << pos2 << endl;
  };
  void parse2GT() {
    pos2 = Xml.find_first_of("/ <>=\"'?!", pos1);
    //cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  void parse2Char(char c) {
    pos2 = Xml.find(c, pos1);
    //cerr << "PGT " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  void parse2Com() {
    pos2 = Xml.find(u8"-->", pos1);
    //cerr << "P-- " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
    pos1 = pos2 + 2;
  };
  void parse2CD() {
    pos2 = Xml.find(u8"]]>", pos1);
    //cerr << "PCD " << pos2 << " " << pos1 << endl;
    if (pos2 == std::string::npos)
      throw std::runtime_error("Syntax");
  };
  std::string getValue() {
    if (pos2 == std::string::npos)
      throw std::runtime_error(u8"unexpected EOF");
    size_t p = pos1;
    pos1 = pos2;
    return decode(p, pos2);
  };
  void clearValue() { posS = posE; }; // der Zwischenraum fand Verwendung
                                      /// Verwaltet den Zischenraum zwischen den <..Tags..>
  void saveValue() {
    // wenn nicht verwendet, darf es nur white space sein
    if (posS != posE)
    {
      //cerr << "White '" <<  Xml.substr(posS, posE - posS) << "'" << endl;
      size_t p = Xml.find_first_not_of(" \n\r\t", posS);
      if (p < posE)
      {
        pos1 = p;
        throw std::runtime_error(u8"unexpected char");
      }
    }
    
    if (pos2 == std::string::npos)
      throw std::runtime_error(u8"unexpected EOF");
    posS = pos1;
    posE = pos2;
    pos1 = pos2;
  };
  void eat(char c) {
    if (Xml[pos1] != c)
      throw std::runtime_error(u8"Expected " + std::to_string(c) + " got " + std::to_string(Xml[pos1]));
    pos1++;
  };
  void eat() { pos1++; };
  char peek() {
    if (pos1 >= Xml.length())
      throw std::runtime_error(u8"unexpected EOF");
    //cerr << "Peek " << Xml[pos1] << " " << pos1 << endl;
    return Xml[pos1];
  };
  /// Wandelt einen Teilstring aus Xml von der HTML-Notation in ASCII zurück
  /// @param posS StartPosition
  /// @param posE EndePosition
  std::string decode(size_t posS, size_t posE) {
    // Da in XML Die Zeicxhe & und < immer escaped (in HTML) werden müssen, kann ein Rückwandlung
    // immer erfolgen, da ein '&' ansonsten nicht vorkommen sollte
    std::string result;
    for (;;)
    {
      size_t pos = Xml.find('&', posS);
      if (pos < posE) // & gefunden
      {
        result += std::string(&Xml[posS], pos-posS);
        posS = pos +1;
        pos = Xml.find(';', posS);
        if (pos < posE and pos < posS + 16) // Token &xxxx; gefunden
        {
          std::string tok = std::string(&Xml[posS], pos-posS);
          //          std::cerr << "TOK " << tok << std::endl;
          char c = '\0';
          if (tok == "lt")
            c = '<';
          else if (tok == "gt")
            c = '>';
          else if (tok == "amp")
            c = '&';
          else if (tok == "quot")
            c = '"';
          else if (tok == "apos")
            c = '\'';
          if (c)
          {
            result += c;
            posS = pos +1;
            continue;
          }
        }
        // wenn nichts passt dann einfach übernehmen
        result += '&';
      }
      else
      {
        result += std::string(&Xml[posS], posE-posS);
        break;
      }
    }
    return result;
  };
  const std::string &Xml;
  size_t pos1, pos2;  // current / search pointer for parsing
  size_t posS, posE;  // start / end pointer for last text
  std::stack<std::string> tags;
  std::string lastKey;
  
};
}

#endif

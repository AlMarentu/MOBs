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

/** \file maria.h
 \brief Datenbank-Interface für Zugriff auf MariaDB

 MariaDB is a registered trademarks of MariaDB.
 \see www.mariadb.com
 */


#ifndef MOBS_MARIA_H
#define MOBS_MARIA_H

#include "dbifc.h"

#include <mysql.h>


namespace mobs {

  /** \brief Datenbank-Verbindung zu einer MariaDB.
   *
   * MariaDB is a registered trademarks of MariaDB.
   * \see www.mariadb.com
   */
  class MariaDatabaseConnection : virtual public DatabaseConnection, public ConnectionInformation {
  public:
    /// \private
    friend DatabaseManagerData;
    /// \private
    explicit MariaDatabaseConnection(const ConnectionInformation &connectionInformation) :
            DatabaseConnection(), ConnectionInformation(connectionInformation) { };
    ~MariaDatabaseConnection() override;

    /// \private
    void open();
    /// \private
    bool load(DatabaseInterface &dbi, ObjectBase &obj) override;
    /// \private
    void save(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    bool destroy(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    void dropAll(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    void structure(DatabaseInterface &dbi, const ObjectBase &obj) override;
    /// \private
    std::shared_ptr<DbCursor> query(DatabaseInterface &dbi, ObjectBase &obj, const std::string &query, bool qbe) override;
    /// \private
    void retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) override;

    // ------------------------------

//    void createPrimaryKey(DatabaseInterface &dbi, ObjectBase &obj);

//    void create(DatabaseInterface &dbi, const ObjectBase &obj);

    /// Ermittle den Collection-Namen zu einem Objekt
    static std::string tableName(const ObjectBase &obj, const DatabaseInterface &dbi) ;

    /// Direkt-Zugriff auf die MariaDB
    MYSQL *getConnection();

  private:
    MYSQL *connection = nullptr;
  };
};

#endif //MOBS_MARIA_H

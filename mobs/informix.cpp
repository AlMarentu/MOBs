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

#include "informix.h"
#include "logging.h"
#include "objgen.h"
#include "mchrono.h"
#include "helper.h"
#include "infxtools.h"

#include <public/sqlca.h>
#include <public/sqlhdr.h>
#include <public/sqltypes.h>

#include <cstdint>
#include <iostream>
#include <iomanip>
#include <utility>
#include <vector>
#include <chrono>

namespace {
using namespace mobs;
using namespace std;

string getErrorMsg(int errNum) {
  string e = "SQL erro:";
  e += std::to_string(errNum);
  e += ":";
  mint len;
  char buf[1024];
  int32_t e2 = rgetlmsg(int32_t(errNum), buf, sizeof(buf), &len);
  if (not e2) {
    e += string(buf, len);
    size_t pos = e.find("%s");
    if (pos != string::npos)
      e.replace(pos, 2, infx_error_msg2());
  }  else
    e += "infx error in getErrorMsg";
  return e;
}



class informix_exception : public std::runtime_error {
public:
  informix_exception(const std::string &e, int errNum) : std::runtime_error(string("informix: ") + e + " " + getErrorMsg(errNum)) {
    LOG(LM_DEBUG, "Informix: " << getErrorMsg(errNum)); }
//  const char* what() const noexcept override { return error.c_str(); }
//private:
//  std::string error;
};

class SQLInformixdescription : public mobs::SQLDBdescription {
public:
  explicit SQLInformixdescription(const string &dbName) : dbPrefix(dbName + ".") {
    changeTo_is_IfNull = false;
  }

  std::string tableName(const std::string &tabnam) override { return dbPrefix + tabnam;  }


  std::string createStmtIndex(std::string name) override {
    return "INT NOT NULL";
  }

  std::string createStmt(const MemberBase &mem, bool compact) override {
    std::stringstream res;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    double d;
    if (mi.isTime and mi.granularity >= 86400000000)
      res << "DATE";
    else if (mi.isTime and mi.granularity >= 1000000)
      res << "DATETIME YEAR TO SECOND";
    else if (mi.isTime and mi.granularity >= 100000)
      res << "DATETIME YEAR TO FRACTION(1)";
    else if (mi.isTime and mi.granularity >= 10000)
      res << "DATETIME YEAR TO FRACTION(2)";
    else if (mi.isTime and mi.granularity >= 1000)
      res << "DATETIME YEAR TO FRACTION(3)";
    else if (mi.isTime and mi.granularity >= 100)
      res << "DATETIME YEAR TO FRACTION(4)";
    else if (mi.isTime)
      res << "DATETIME YEAR TO FRACTION(5)";
    else if (mi.isUnsigned and mi.max == 1)
      res << "BOOLEAN"; //"SMALLINT";
    else if (mem.toDouble(d))
      res << "FLOAT";
    else if (mem.is_chartype(mobs::ConvToStrHint(compact))) {
      if (mi.is_specialized and mi.size == 1)
        res << "CHAR(1)";
      else {
        MemVarCfg c = mem.hasFeature(LengthBase);
        size_t n = c ? (c - LengthBase) : 30;
        if (n <= 4)
          res << "CHAR(" << n << ")";
        else if (n <= 255)
          res << "VARCHAR(" << n << ")";
        else
          res << "LVARCHAR(" << n << ")";
      }
    }
    else if (mi.isSigned and mi.max <= INT16_MAX)
      res << "SMALLINT";
    else if (mi.isSigned and mi.max <= INT32_MAX)
      res << "INT";
    else if (mi.isSigned or mi.isUnsigned) // uint64_t is not supported
      res << "BIGINT";
    else
      res << "SMALLINT";
    if (not mem.nullAllowed())
      res << " NOT NULL";
    return res.str();
  }

  std::string valueStmtIndex(size_t i) override {
    LOG(LM_DEBUG, "Informix SqlVar index: " << fldCnt << "=" << i);
    if (fldCnt == 0)
      pos = 0;
    ifx_sqlvar_t &sql_var = descriptor->sqlvar[fldCnt++];
    memset(&sql_var, 0, sizeof(ifx_sqlvar_t));
    descriptor->sqld = fldCnt;
    sql_var.sqltype = SQLINT;
    setBuffer(sql_var);
    *(int32_t *)(sql_var.sqldata) = i;
    return "?";
  }
  void setBuffer(ifx_sqlvar_t &sql_var, unsigned sz = 0) {
    pos = (short)rtypalign(pos, sql_var.sqltype);
    sql_var.sqldata = buf + pos;
    sql_var.sqllen = sz;
    mint size = rtypmsize(sql_var.sqltype, sql_var.sqllen);
    pos += size;
    if (sql_var.sqllen <= 0) {
      sql_var.sqllen = size;
      if (sql_var.sqllen <= 0)
        throw runtime_error("error in setBuffer");
    }
  }
  std::string valueStmt(const MemberBase &mem, bool compact, bool increment, bool inWhere) override {
    if (fldCnt == 0)
      pos = 0;
    double d;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    if (not descriptor or not buf)
    {
      if (mem.isNull())
        return u8"null";
      if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
        std::stringstream s;
        struct tm ts{};
        mi.toLocalTime(ts);
        s << std::put_time(&ts, "%F");
        return mobs::to_squote(s.str());
      }
      else if (mi.isTime) {
        MTime t;
        if (not from_number(mi.i64, t))
          throw std::runtime_error("Time Conversion");
        return mobs::to_squote(to_string_ansi(t));
      }
      else if (mi.isUnsigned and mi.max == 1) // bool
        return (mi.u64 ? "1" : "0");
      else if (mem.is_chartype(mobs::ConvToStrHint(compact)))
        return mobs::to_squote(mem.toStr(mobs::ConvToStrHint(compact)));

      return mem.toStr(mobs::ConvToStrHint(compact));
    }
    ifx_sqlvar_t &sql_var = descriptor->sqlvar[fldCnt++];
    memset(&sql_var, 0, sizeof(ifx_sqlvar_t));
    descriptor->sqld = fldCnt;
    int e = 0;
    if (mi.isTime and mi.granularity >= 86400000000) { // nur Datum
      std::stringstream s;
      struct tm ts{};
      mi.toLocalTime(ts);
      LOG(LM_DEBUG, "Informix SqlVar " << mem.name() << ": " << fldCnt -1 << "=" << std::put_time(&ts, "%F"));
//      s << std::put_time(&ts, "%F");  // TODO
      sql_var.sqltype = SQLDATE;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else {
        short mdy[3]; //  { 12, 21, 2007 };
        mdy[0] = mint(ts.tm_mon + 1);
        mdy[1] = mint(ts.tm_mday);
        mdy[2] = mint(ts.tm_year + 1900);
        e = rmdyjul(mdy, (int4 *) sql_var.sqldata);
      }
    }
    else if (mi.isTime) {
      MTime t;
      if (not from_number(mi.i64, t))
        throw std::runtime_error("Time Conversion");
      std::string s = to_string_ansi(t, mobs::MF5);
      LOG(LM_DEBUG, "Informix SqlVar " << mem.name() << ": " << fldCnt -1 << "=" << s);
      sql_var.sqltype = SQLDTIME;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else {
        auto dtp = (dtime_t *) sql_var.sqldata;
        dtp->dt_qual = TU_DTENCODE(TU_YEAR, TU_F5);
        e = dtcvfmtasc(const_cast<char *>(s.c_str()), (char *) "%iY-%m-%d %H:%M:%S.%5F", dtp);
      }
    } else if (mi.isUnsigned) {
      if (increment) {
        if (mi.u64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        if (mem.isNull())
          throw std::runtime_error("VersionElement is null");
        mi.u64++;
      }
      LOG(LM_DEBUG, "Informix SqlVar " << mem.name() << ": " << fldCnt - 1 << "=" << mi.u64);
      if (mi.max > INT32_MAX) {
#ifdef SQLBIGINT
        if (mi.u64 > INT64_MAX)
#endif
          throw std::runtime_error("Number to big");
#ifdef SQLBIGINT
        sql_var.sqltype = SQLBIGINT;
        setBuffer(sql_var);
        *(int64_t *) (sql_var.sqldata) = mi.u64;
#endif
      } else {
        sql_var.sqltype = SQLINT;
        setBuffer(sql_var);
        *(int *) (sql_var.sqldata) = mi.u64;
      }
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
    } else if (mi.isSigned) {
      if (increment) {
        if (mi.i64 == mi.max)
          throw std::runtime_error("VersionElement overflow");
        if (mem.isNull())
          throw std::runtime_error("VersionElement is null");
        mi.i64++;
      }
      if (mi.max > INT32_MAX) {
#ifdef SQLBIGINT
        sql_var.sqltype = SQLBIGINT;
        setBuffer(sql_var);
        *(bigint *) (sql_var.sqldata) = mi.i64;
#else
        throw std::runtime_error("infx: bigint not avilable");
#endif
      } else {
        sql_var.sqltype = SQLINT;
        setBuffer(sql_var);
        *(int *) (sql_var.sqldata) = mi.i64;
      }
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);

//      int e = bigintcvifx_int8((int64_t *)&mi.i64, (bigint *)sql_var.sqldata);
    } else if (mem.toDouble(d)) {
      sql_var.sqltype = SQLFLOAT;
      setBuffer(sql_var);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else
        *(double *) (sql_var.sqldata) = d;
    } else {
      std::string s = mem.toStr(mobs::ConvToStrHint(compact));
      LOG(LM_DEBUG, "Informix SqlVar " << mem.name() << ": " << fldCnt - 1 << "=" << s);
      if (increment)
        throw std::runtime_error("VersionElement is not int");
      // leerstring entspricht NULL daher SQLCHAR mit 1 SPC als leer mit not null
      sql_var.sqltype = SQLVCHAR;
      if (s.empty()) {
        sql_var.sqltype = SQLCHAR;
        s = " ";
      } else if (s.length() >= 2) {
        sql_var.sqltype = SQLLVARCHAR;
      }
      setBuffer(sql_var, s.length() + 1);
      if (mem.isNull())
        e = rsetnull(sql_var.sqltype, sql_var.sqldata);
      else
        stcopy(s.c_str(), sql_var.sqldata);
    }
    if (e)
      throw informix_exception("Conversion error Date", e);

    return "?";
  }

  void readValue(MemberBase &mem, bool compact) override {
    if (pos >= fldCnt)
      throw runtime_error(u8"Result not found " + mem.name());
    auto &col = descriptor->sqlvar[pos++];
    LOG(LM_DEBUG, "Read " << mem.name() << " " << col.sqlname << " " << col.sqllen << " " << rtypname(col.sqltype));

    if (risnull(col.sqltype, col.sqldata)) {
      mem.forceNull();
      return;
    }
    bool ok = true;
    int e = 0;
    MobsMemberInfo mi;
    mem.memInfo(mi);
    switch (col.sqltype) {
      case SQLCHAR:
      case SQLNCHAR:
        // strip trailing blanks
        for (char *cp = col.sqldata + col.sqllen - 2; *cp == ' ' and cp >= col.sqldata; --cp)
          *cp = '\0';
        // fall into
      case SQLLVARCHAR:
      case SQLNVCHAR:
      case SQLVCHAR:
        ok = mem.fromStr(string(col.sqldata),
                         not compact ? ConvFromStrHint::convFromStrHintExplizit : ConvFromStrHint::convFromStrHintDflt);
        if (not ok)
          throw runtime_error(u8"conversion error in " + mem.name() + " Value=" + string(col.sqldata));
        return;
      case SQLDATE: {
        short mdy[3]; //  { 12, 21, 2007 };
        e = rjulmdy(*(int4 *) col.sqldata, mdy);
        if (e)
          throw informix_exception(u8"Date Conversion", e);
        std::tm ts = {};
        ts.tm_mon = mdy[0] - 1;
        ts.tm_mday = mdy[1];
        ts.tm_year = mdy[2] - 1900;
        LOG(LM_INFO, "DATE " << mdy[1] << '.' << mdy[0] << '.' << mdy[2]);
        if (mi.isTime)
          mi.fromLocalTime(ts);
        else
          ok = false;
        break;
      }
      case SQLDTIME: {
        char timebuf[32];
        e = dttofmtasc((dtime_t *) col.sqldata, timebuf, sizeof(timebuf), (char *) "%iY-%m-%d %H:%M:%S.%5F");
        if (e)
          throw informix_exception(u8"DateTime Conversion", e);
        LOG(LM_INFO, "DATETIME " << timebuf);
        MTime t;
        ok = string2x(string(timebuf), t);
        mi.i64 = t.time_since_epoch().count();
        break;
      }
      case SQLBOOL:
        if (mi.isUnsigned)
          mi.u64 = *(int8_t *) (col.sqldata);
        else
          mi.i64 = *(int8_t *) (col.sqldata);
        break;
      case SQLSMINT:
        if (mi.isUnsigned)
          mi.u64 = *(int16_t *) (col.sqldata);
        else
          mi.i64 = *(int16_t *) (col.sqldata);
        break;
      case SQLINT:
      case SQLSERIAL:
        if (mi.isUnsigned)
          mi.u64 = *(int32_t *) (col.sqldata);
        else
          mi.i64 = *(int32_t *) (col.sqldata);
        break;
#ifdef SQLBIGINT
      case SQLBIGINT:
      case SQLSERIAL8:
        if (mi.isUnsigned)
          mi.u64 = *(int64_t *) (col.sqldata);
        else
          mi.i64 = *(int64_t *) (col.sqldata);
        break;
#endif
      case SQLFLOAT:
      {
        double d;
        if (mem.toDouble(d)) {
          ok = mem.fromDouble(*(double *) (col.sqldata));
          if (ok)
            return;
        } else
          ok = false;
        break;
      }
       default:
        throw runtime_error(u8"conversion error in " + mem.name() + " Type=" + to_string(col.sqltype));
    }
//    if (mi.isUnsigned and mi.max == 1) // bool
//      ok = mem.fromUInt64(res == 0 ? 0 : 1);
//    else
    if (not ok)
      ;
    else if (mi.isSigned or mi.isTime)
      ok = mem.fromInt64(mi.i64);
    else if (mi.isUnsigned)
      ok = mem.fromUInt64(mi.u64);
    else  // TODO konvertierung über string versuchen
      ok = false;

    if (not ok)
      throw runtime_error(u8"conversion error in " + mem.name());
  }

  size_t readIndexValue() override {
    if (pos >= fldCnt)
      throw runtime_error(u8"Result not found index");
    auto &col = descriptor->sqlvar[pos++];
    LOG(LM_DEBUG, "Read idx " << col.sqlname << " " << col.sqllen);

    if (risnull(col.sqltype, col.sqldata))
      throw runtime_error(u8"index value is null");

    switch (col.sqltype) {
      case SQLSMINT:
        return *(int16_t *) (col.sqldata);
      case SQLINT:
        return *(int32_t *) (col.sqldata);
#ifdef SQLBIGINT
      case SQLBIGINT:
        return *(int64_t *) (col.sqldata);
#endif
    }
    throw runtime_error(u8"index value is not integer");
  }

  void startReading() override {
    pos = 0;
  }
  void finishReading() override {}

  int fldCnt = 0;
  struct sqlda *descriptor = nullptr;
  char * buf = nullptr; //

private:
  std::string dbPrefix;
  int pos = 0;
};




class CountCursor : public virtual mobs::DbCursor {
  friend class mobs::InformixDatabaseConnection;
public:
  explicit CountCursor(size_t size) { cnt = size; }
  ~CountCursor() override = default;;
  bool eof() override  { return true; }
  bool valid() override { return false; }
  void operator++() override { }
};




class InformixCursor : public virtual mobs::DbCursor {
  friend class mobs::InformixDatabaseConnection;
public:
  explicit InformixCursor(int conNr, std::shared_ptr<DatabaseConnection> dbi,
                       std::string dbName) :
          m_conNr(conNr), dbCon(std::move(dbi)), databaseName(std::move(dbName)) {
    static int n = 0;
    m_cursNr = ++n;
  }
  ~InformixCursor() override {
    if (descPtr)
      close();
  }
  bool eof() override  { return not descPtr; }
  bool valid() override { return not eof(); }
  void operator++() override {
    const int NOMOREROWS=100;
    if (eof())
      return;
    string c = "curs";
    c += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL fetch " << c);
    int e = infx_fetch(c.c_str(), descPtr);
    if (e) {
      close();
      if (e == NOMOREROWS)
        return;
      throw informix_exception(u8"cursor: query row failed", e);
    }
    cnt++;
  }
private:
  void open(const string &stmt) {
    const int NOMOREROWS=100;
    string c = "curs";
    c += std::to_string(m_cursNr);
    string p = "prep";
    p += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL declare " << c << " cursor");
    int e = infx_query(stmt.c_str(), c.c_str(), p.c_str(), &descPtr);
    if (e)
      throw informix_exception(u8"cursor: query row failed", e);
    fldCnt = descPtr->sqld;
    LOG(LM_INFO, "Anz Fields " << fldCnt);

    mint cnt = 0;
    mint pos = 0;
    for(struct sqlvar_struct *col_ptr = descPtr->sqlvar; cnt < fldCnt; cnt++, col_ptr++)
    {
      LOG(LM_INFO, "COL " << cnt << col_ptr->sqltype << " " << col_ptr->sqlname);

      /* Allow for the trailing null character in C character arrays */
      switch (col_ptr->sqltype) {
        case SQLCHAR:
        case SQLNCHAR:
        case SQLNVCHAR:
        case SQLVCHAR:
          col_ptr->sqllen += 1;
          break;
      }

      /* Get next word boundary for column data and assign buffer position to sqldata */
      pos = rtypalign(pos, col_ptr->sqltype);
      col_ptr->sqldata = &buf[pos];

      /* Determine size used by column data and increment buffer position */
      mint size = rtypmsize(col_ptr->sqltype, col_ptr->sqllen);
      pos += size;
      if (pos > sizeof(buf))
        throw runtime_error(u8"informix Buffer overflow");
    }
    LOG(LM_DEBUG, "SQL open " << c);
    e = infx_open_curs(c.c_str());
    if (e)
      throw informix_exception(u8"cursor: open cursor failed", e);
    LOG(LM_DEBUG, "SQL fetch " << c);
    e = infx_fetch(c.c_str(), descPtr);
    if (e) {
      close();
      if (e != NOMOREROWS)
        throw informix_exception(u8"cursor: query row failed", e);
    }
  }
  void close() {
    string c = "curs";
    c += std::to_string(m_cursNr);
    string p = "prep";
    p += std::to_string(m_cursNr);
    LOG(LM_DEBUG, "SQL close " << c);
    infx_remove_curs(c.c_str(), p.c_str());
    free(descPtr);
    descPtr = nullptr;
  }
  std::shared_ptr<DatabaseConnection> dbCon;  // verhindert das Zerstören der Connection
  std::string databaseName;  // unused
  int m_conNr;
  int m_cursNr = 0;
  int fldCnt = 0;
  struct sqlda *descPtr = nullptr;
  char buf[32768];
};

}

namespace mobs {

InformixDatabaseConnection::~InformixDatabaseConnection() {
  if (conNr > 0)
    infx_disconnect(conNr);
}


std::string InformixDatabaseConnection::tableName(const ObjectBase &obj, const DatabaseInterface &dbi) {
  MemVarCfg c = obj.hasFeature(ColNameBase);
  if (c)
    return dbi.database() + "." + obj.getConf(c);
  return dbi.database() + "." + obj.typeName();
}

void InformixDatabaseConnection::open() {
  const int DBLOCALEMISMATCH=-23197;
  if (conNr > 0) {
    infx_set_connection(conNr);
    return;
  }
  size_t pos = m_url.find("//");
  if (pos == string::npos)
    throw runtime_error("informix: error in url");
  size_t pos2 = m_url.find(':', pos);
  string host;
  if (pos2 == string::npos)
    pos2 = m_url.length();

  host = m_url.substr(pos+2, pos2-pos-2);
  const char *dblocale[] = {"de_DE.UTF8", "de_DE.8859-1", nullptr};
  string db = m_database;
  db += "@";
  db += host;
  conNr = infx_connect(db.c_str(), m_user.c_str(), m_password.c_str());
  const char **nextLoc = dblocale;
  while  (conNr == DBLOCALEMISMATCH and *nextLoc) {
    LOG(LM_DEBUG, "infx Locale invalid, try " << *nextLoc);
    setenv("DB_LOCALE", *nextLoc++, 1);
    conNr = infx_connect(db.c_str(), m_user.c_str(), m_password.c_str());
  }
  LOG(LM_DEBUG, "Informix connecting to " << db << " NR = " << conNr);
  if (conNr > 0)
    return;
  if (conNr < 0)
    throw informix_exception(u8"save failed", conNr);
  throw runtime_error("informix: error connecting to db");
}

bool InformixDatabaseConnection::load(DatabaseInterface &dbi, ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  string s = gsql.selectStatementFirst();
  LOG(LM_DEBUG, "SQL: " << s);
  auto cursor = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database());
  cursor->open(s);
  if (cursor->eof()) {
    LOG(LM_DEBUG, "NOW ROWS FOUND");
    return false;
  }

  retrieve(dbi, obj, cursor);
  return true;
}

void InformixDatabaseConnection::save(DatabaseInterface &dbi, const ObjectBase &obj) {
  const int UNIQCONSTRAINT=-268;
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  ifx_sqlda_t descriptor{};
  ifx_sqlvar_t sql_var[100];
  char buffer[32768];
  descriptor.sqlvar = sql_var;
  sd.descriptor = &descriptor;
  sd.buf = buffer;

  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    if (e)
      throw informix_exception(u8"Transaction failed", e);
  }
  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  try {
    string s;
    string upd;
    if (version > 0) // bei unsicher (-1) immer erst insert probieren
      s = gsql.updateStatement(true);
    else
      s = gsql.insertUpdStatement(true, upd);
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_exec_desc(s.c_str(), &descriptor);
    if (version < 0 and e == UNIQCONSTRAINT and not upd.empty()) {  // try update
      LOG(LM_DEBUG, "Uniq Constraint error -> try update");
      LOG(LM_DEBUG, "SQL " << upd);
      e = infx_exec_desc(upd.c_str(), &descriptor);
    }
    if (e)
      throw informix_exception(u8"save failed", e);
    if (version > 0 and infx_processed_rows() != 1)
      throw runtime_error(u8"number of processed rows is " + to_string(infx_processed_rows()) + " should be 1");

    while (not gsql.eof()) {
      sd.fldCnt = 0;
      s = gsql.insertUpdStatement(false, upd);
      LOG(LM_DEBUG, "SQL " << s);
      e = infx_exec_desc(s.c_str(), &descriptor);
      if (e == UNIQCONSTRAINT and not upd.empty()) {  // try update
        LOG(LM_DEBUG, "Uniq Constraint error -> try update");
        LOG(LM_DEBUG, "SQL " << upd);
        e = infx_exec_desc(upd.c_str(), &descriptor);
      }
      if (e)
        throw informix_exception(u8"save failed", e);
    }








  } catch (runtime_error &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  } catch (exception &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
}


bool InformixDatabaseConnection::destroy(DatabaseInterface &dbi, const ObjectBase &obj) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);
  ifx_sqlda_t descriptor{};
  ifx_sqlvar_t sql_var[100];
  char buffer[8096];
  descriptor.sqlvar = sql_var;
  sd.descriptor = &descriptor;
  sd.buf = buffer;
  // Transaktion benutzen zwecks Atomizität
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    // Wenn DBI mit Transaktion, dann in Transaktion bleiben
  }
  else if (currentTransaction != dbi.getTransaction())
    throw std::runtime_error("transaction mismatch");
  else {
    string s = "SAVEPOINT MOBS;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
  }
  int64_t version = gsql.getVersion();
  LOG(LM_DEBUG, "VERSION IS " << version);

  bool found = false;
  try {
    for (bool first = true; first or not gsql.eof(); first = false) {
      sd.fldCnt = 0;
      string s = gsql.deleteStatement(first);
      LOG(LM_DEBUG, "SQL " << s);
      int e = infx_exec_desc(s.c_str(), &descriptor);
      if (e)
        throw informix_exception(u8"destroy failed", e);
      if (first) {
        found = (infx_processed_rows() > 0);
        if (version > 0 and not found)
          throw runtime_error(u8"destroy: Object with appropriate version not found");
      }
    }
  } catch (runtime_error &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  } catch (exception &exc) {
    string s = "ROLLBACK WORK";
    if (currentTransaction)
      s += " TO SAVEPOINT MOBS";
    s += ";";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    throw exc;
  }

  string s;
  if (currentTransaction)
    s = "RELEASE SAVEPOINT MOBS;";
  else
    s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);

  return found;
}

void InformixDatabaseConnection::dropAll(DatabaseInterface &dbi, const ObjectBase &obj) {
  const int EXISTSNOT = -206;
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.dropStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e and e != EXISTSNOT)
      throw informix_exception(u8"dropAll failed", e);
  }
}

void InformixDatabaseConnection::structure(DatabaseInterface &dbi, const ObjectBase &obj) {
  const int EXISTS = -310;
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  for (bool first = true; first or not gsql.eof(); first = false) {
    string s = gsql.createStatement(first);
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e and e != EXISTS)
      throw informix_exception(u8"create failed", e);
  }
}

std::shared_ptr<DbCursor>
InformixDatabaseConnection::query(DatabaseInterface &dbi, ObjectBase &obj, const string &query, bool qbe) {
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  string s;
  if (qbe)
    s = gsql.queryBE(dbi.getCountCursor() ? SqlGenerator::Count : SqlGenerator::Normal);
  else
    s = gsql.query(dbi.getCountCursor() ? SqlGenerator::Count : SqlGenerator::Normal, query);
// TODO  s += " LOCK IN SHARE MODE WAIT 10 "; / NOWAIT
  LOG(LM_INFO, "SQL: " << s);
  if (dbi.getCountCursor()) {
    long cnt = 0;
    int e = infx_count(s.c_str(), &cnt);
    if (e)
      throw informix_exception(u8"dropAll failed", e);
    return std::make_shared<CountCursor>(cnt);
  }

  auto cursor = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database());
  cursor->open(s);
  if (cursor->eof()) {
    LOG(LM_DEBUG, "NOW ROWS FOUND");
  }
  return cursor;
}

void
InformixDatabaseConnection::retrieve(DatabaseInterface &dbi, ObjectBase &obj, std::shared_ptr<mobs::DbCursor> cursor) {
  auto curs = std::dynamic_pointer_cast<InformixCursor>(cursor);
  if (not curs)
    throw runtime_error("InformixDatabaseConnection: invalid cursor");

  if (not curs->descPtr) {
    throw runtime_error("Cursor eof");
  }
  open();
  SQLInformixdescription sd(dbi.database());
  mobs::SqlGenerator gsql(obj, sd);

  obj.clear();
  sd.descriptor = curs->descPtr;
  sd.fldCnt = curs->fldCnt;
  gsql.readObject(obj);

  while (not gsql.eof()) {
    SqlGenerator::DetailInfo di;
    string s = gsql.selectStatementArray(di);
    LOG(LM_DEBUG, "SQL " << s);
    auto curs2 = std::make_shared<InformixCursor>(conNr, dbi.getConnection(), dbi.database());
    curs2->open(s);
    sd.descriptor = curs2->descPtr;
    sd.fldCnt = curs2->fldCnt;
    // Vektor auf leer setzten (wurde wegen Struktur zuvor erweitert)
    di.vecNc->resize(0);
    while (not curs2->eof()) {
      gsql.readObject(di);
      curs2->next();
    }
  }

  LOG(LM_DEBUG, "RESULT " << obj.to_string());
}

void InformixDatabaseConnection::startTransaction(DatabaseInterface &dbi, DbTransaction *transaction,
                                                  shared_ptr<TransactionDbInfo> &tdb) {
  open();
  if (currentTransaction == nullptr) {
    string s = "BEGIN WORK;";
    LOG(LM_DEBUG, "SQL " << s);
    int e = infx_execute(s.c_str());
    if (e)
      throw informix_exception(u8"Transaction failed", e);
    currentTransaction = transaction;
  }
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch"); // hier geht nur eine Transaktion gleichzeitig
}

void InformixDatabaseConnection::endTransaction(DbTransaction *transaction, shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  else if (currentTransaction != transaction)
    throw std::runtime_error("transaction mismatch");
  string s = "COMMIT WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
  currentTransaction = nullptr;
}

void InformixDatabaseConnection::rollbackTransaction(DbTransaction *transaction, shared_ptr<TransactionDbInfo> &tdb) {
  if (currentTransaction == nullptr)
    return;
  string s = "ROLLBACK WORK;";
  LOG(LM_DEBUG, "SQL " << s);
  int e = infx_execute(s.c_str());
  if (e)
    throw informix_exception(u8"Transaction failed", e);
  currentTransaction = nullptr;
}

size_t InformixDatabaseConnection::doSql(const string &sql) {
  LOG(LM_DEBUG, "SQL " << sql);
  int e = infx_execute(sql.c_str());
  if (e)
    throw informix_exception(u8"doSql " + sql + ": ", e);
  return infx_processed_rows();
}


}
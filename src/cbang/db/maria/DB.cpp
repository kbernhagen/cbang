/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "DB.h"

#include <cbang/String.h>
#include <cbang/Exception.h>
#include <cbang/time/Time.h>
#include <cbang/json/JSON.h>
#include <cbang/log/Logger.h>

#include <mysql/mysql.h>

#include <cstring>

#define RAISE_ERROR(msg) raiseError(SSTR(msg), false)
#define RAISE_DB_ERROR(msg) raiseError(SSTR(msg), true)

using namespace std;
using namespace cb;
using namespace cb::MariaDB;


DB::DB(st_mysql *db) :
  db(db ? db : mysql_init(0)), res(0), nonBlocking(false), connected(false),
  stored(false), status(0), continueFunc(0) {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  if (!this->db) RAISE_DB_ERROR("Failed to create MariaDB");
}


DB::~DB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");
  if (db) mysql_close(db);
}


void DB::setInitCommand(const string &cmd) {
  if (mysql_options(db, MYSQL_INIT_COMMAND, cmd.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB init command: " << cmd);
}


void DB::enableCompression() {
  if (mysql_options(db, MYSQL_OPT_COMPRESS, 0))
    RAISE_DB_ERROR("Failed to enable MariaDB compress");
}


void DB::setConnectTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_CONNECT_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB connect timeout to " << secs);
}


void DB::setLocalInFile(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_LOCAL_INFILE, &x))
    RAISE_DB_ERROR("Failed to " << (enable ? "enable" : "disable")
                << " MariaD local infile");
}


void DB::enableNamedPipe() {
  if (mysql_options(db, MYSQL_OPT_NAMED_PIPE, 0))
    RAISE_DB_ERROR("Failed to enable MariaDB named pipe");
}


void DB::setProtocol(protocol_t protocol) {
  mysql_protocol_type type;
  switch (protocol) {
  case PROTOCOL_TCP: type = MYSQL_PROTOCOL_TCP; break;
  case PROTOCOL_SOCKET: type = MYSQL_PROTOCOL_SOCKET; break;
  case PROTOCOL_PIPE: type = MYSQL_PROTOCOL_PIPE; break;
  default: RAISE_ERROR("Invalid protocol " << protocol);
  }

  if (mysql_options(db, MYSQL_OPT_PROTOCOL, &type))
    RAISE_DB_ERROR("Failed to set MariaDB protocol to " << protocol);
}


void DB::setReconnect(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_OPT_RECONNECT, &x))
    RAISE_DB_ERROR("Failed to " << (enable ? "enable" : "disable")
                   << "MariaDB auto reconnect");
}


void DB::setReadTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_READ_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB read timeout to " << secs);
}


void DB::setWriteTimeout(unsigned secs) {
  if (mysql_options(db, MYSQL_OPT_WRITE_TIMEOUT, &secs))
    RAISE_DB_ERROR("Failed to set MariaDB write timeout to " << secs);
}


void DB::setDefaultFile(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_FILE, path.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB default type to " << path);
}


void DB::readDefaultGroup(const string &path) {
  if (mysql_options(db, MYSQL_READ_DEFAULT_GROUP, path.c_str()))
    RAISE_DB_ERROR("Failed to read MariaDB default group file " << path);
}


void DB::setReportDataTruncation(bool enable) {
  my_bool x = enable;
  if (mysql_options(db, MYSQL_REPORT_DATA_TRUNCATION, &x))
    RAISE_DB_ERROR("Failed to" << (enable ? "enable" : "disable")
                << " MariaDB data truncation reporting.");
}


void DB::setCharacterSet(const string &name) {
  if (mysql_options(db, MYSQL_SET_CHARSET_NAME, name.c_str()))
    RAISE_DB_ERROR("Failed to set MariaDB character set to " << name);
}


void DB::enableNonBlocking() {
  if (mysql_options(db, MYSQL_OPT_NONBLOCK, 0))
    RAISE_DB_ERROR("Failed to set MariaDB to non-blocking mode");
  nonBlocking = true;
}


void DB::connect(const string &host, const string &user, const string &password,
                 const string &dbName, unsigned port, const string &socketName,
                 flags_t flags) {
  assertNotPending();
  MYSQL *db = mysql_real_connect(
    this->db, host.c_str(), user.c_str(), password.c_str(), dbName.c_str(),
    port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;
}


bool DB::connectNB(const string &host, const string &user,
                   const string &password, const string &dbName, unsigned port,
                   const string &socketName, flags_t flags) {
  LOG_DEBUG(5, CBANG_FUNC << "(host=" << host << ", user=" << user
    << ", db=" << dbName << ", port=" << port
    << ", socketName=" << socketName << ", flags=" << flags << ")");

  assertNotPending();
  assertNonBlocking();

  MYSQL *db = 0;
  status = mysql_real_connect_start(
    &db, this->db, host.c_str(), user.c_str(), password.c_str(),
    dbName.c_str(), port, socketName.empty() ? 0 : socketName.c_str(), flags);

  if (status) {
    continueFunc = &DB::connectContinue;
    return false;
  }

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;

  return true;
}


void DB::resetConnection() {
  assertNotPending();

  if (mysql_reset_connection(db))
    RAISE_DB_ERROR("Failed to reset DB connection");
}


bool DB::resetConnectionNB() {
  assertNotPending();

  int ret = 0;
  mysql_reset_connection_start(&ret, db);

  if (status) {
    continueFunc = &DB::resetConnectionContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to reset DB connection");

  return true;
}


void DB::changeUser(const string &user, const string &password,
                    const string &dbName) {
  assertNotPending();
  my_bool ret = mysql_change_user(
    db, user.c_str(), password.c_str(), dbName.c_str());

  if (ret) RAISE_DB_ERROR("Failed to change DB user");
}


bool DB::changeUserNB(const string &user, const string &password,
                      const string &dbName) {
  LOG_DEBUG(5, CBANG_FUNC << "(user=" << user << " db=" << dbName << ")");

  assertNotPending();
  assertNonBlocking();

  my_bool ret = false;
  status = mysql_change_user_start(
    &ret, db, user.c_str(), password.c_str(), dbName.c_str());

  if (status) {
    continueFunc = &DB::changeUserContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to change DB user");

  return true;
}


bool DB::ping() {return !mysql_ping(db);}


bool DB::pingNB() {
  int ret = 0;
  status = mysql_ping_start(&ret, db);

  if (status) {
    continueFunc = &DB::pingContinue;
    return false;
  }

  connected = !ret;

  return true;
}


void DB::close() {
  if (!connected) return;

  if (db) {
    mysql_close(db);
    db = mysql_init(0);
  }
  connected = false;
}


bool DB::closeNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  status = mysql_close_start(db);
  if (status) {
    continueFunc = &DB::closeContinue;
    return false;
  }

  db = mysql_init(0);
  connected = false;
  return true;
}


void DB::use(const string &dbName) {
  assertConnected();
  assertNotPending();

  if (mysql_select_db(db, dbName.c_str()))
    RAISE_DB_ERROR("Failed to select DB");
}


bool DB::useNB(const string &dbName) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_select_db_start(&ret, db, dbName.c_str());

  if (status) {
    continueFunc = &DB::useContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Failed to select DB");

  return true;
}


void DB::query(const string &s) {
  assertConnected();
  assertNotPending();

  if (mysql_real_query(db, s.data(), s.length()))
    RAISE_DB_ERROR("Query failed");
}


bool DB::queryNB(const string &s) {
  assertConnected();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_real_query_start(&ret, db, s.data(), s.length());

  LOG_DEBUG(5, CBANG_FUNC << "() status=" << status << " ret=" << ret);

  if (status) {
    continueFunc = &DB::queryContinue;
    return false;
  }

  if (ret) RAISE_DB_ERROR("Query failed");

  return true;
}


void DB::flushResults() {
  while (true) {
    storeResult();
    if (haveResult()) freeResult();
    if (!moreResults()) break;
    nextResult();
  }
}


void DB::useResult() {
  assertConnected();
  assertNotPending();
  assertNotHaveResult();

  res = mysql_use_result(db);
  if (!res) RAISE_DB_ERROR("Failed to use result");

  stored = false;
}


void DB::storeResult() {
  assertConnected();
  assertNotPending();
  assertNotHaveResult();

  res = mysql_store_result(db);

  if (res) stored = true;
  else if (mysql_field_count(db)) RAISE_DB_ERROR("Failed to store result");
}


bool DB::storeResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotPending();
  assertNonBlocking();
  assertNotHaveResult();

  status = mysql_store_result_start(&res, db);
  if (status) {
    continueFunc = &DB::storeResultContinue;
    return false;
  }

  if (res) stored = true;
  else if (hasError()) RAISE_DB_ERROR("Failed to store result");

  return true;
}


bool DB::haveResult() const {return res;}


bool DB::nextResult() {
  assertConnected();
  assertNotHaveResult();
  assertNotPending();

  int ret = mysql_next_result(db);
  if (0 < ret) RAISE_DB_ERROR("Failed to get next result");

  return ret == 0;
}


bool DB::nextResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertConnected();
  assertNotHaveResult();
  assertNotPending();
  assertNonBlocking();

  int ret = 0;
  status = mysql_next_result_start(&ret, db);
  if (status) {
    continueFunc = &DB::nextResultContinue;
    return false;
  }

  if (0 < ret) RAISE_DB_ERROR("Failed to get next result");
  if (ret) LOG_DEBUG(5, "No more results");

  return true;
}


bool DB::moreResults() const {
  assertConnected();
  return mysql_more_results(db);
}


void DB::freeResult() {
  assertNotPending();
  assertHaveResult();
  mysql_free_result(res);
  res = 0;
  stored = false;
}


bool DB::freeResultNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertNotPending();
  assertNonBlocking();
  assertHaveResult();

  status = mysql_free_result_start(res);
  if (status) {
    continueFunc = &DB::freeResultContinue;
    return false;
  }

  res = 0;

  return true;
}


uint64_t DB::getRowCount() const {
  assertHaveResult();
  return mysql_num_rows(res);
}


uint64_t DB::getAffectedRowCount() const {
  assertHaveResult();
  return mysql_affected_rows(db);
}


unsigned DB::getFieldCount() const {
  assertHaveResult();
  return mysql_num_fields(res);
}


bool DB::fetchRow() {
  assertNotPending();
  assertHaveResult();
  return mysql_fetch_row(res);
}


bool DB::fetchRowNB() {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertNotPending();
  assertNonBlocking();
  assertHaveResult();

  MYSQL_ROW row = 0;
  status = mysql_fetch_row_start(&row, res);
  if (status) {
    continueFunc = &DB::fetchRowContinue;
    return false;
  }

  return true;
}


bool DB::haveRow() const {return res && res->current_row;}


void DB::seekRow(uint64_t row) {
  assertHaveResult();
  if (!stored) RAISE_ERROR("Must use storeResult() before seekRow()");
  if (mysql_num_rows(res) <= row)
    RAISE_ERROR("Row seek out of range " << row);
  mysql_data_seek(res, row);
}


void DB::appendRow(JSON::Sink &sink, int first, int count) const {
  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    sink.beginAppend();
    writeField(sink, i);
  }
}


void DB::insertRow(JSON::Sink &sink, int first, int count,
                   bool withNulls) const {

  for (unsigned i = first; i < getFieldCount() && count; i++, count--) {
    if (!withNulls && getNull(i)) continue;
    sink.beginInsert(getField(i).getName());
    writeField(sink, i);
  }
}


void DB::writeRowList(JSON::Sink &sink, int first, int count) const {
  sink.beginList();
  appendRow(sink, first, count);
  sink.endList();
}


void DB::writeRowDict(JSON::Sink &sink, int first, int count,
                      bool withNulls) const {
  sink.beginDict();
  insertRow(sink, first, count, withNulls);
  sink.endDict();
}


SmartPointer<JSON::Value> DB::getRowList(int first, int last) const {
  JSON::Builder builder;
  writeRowList(builder, first, last);
  return builder.getRoot();
}


SmartPointer<JSON::Value> DB::getRowDict(int first, int last,
                                         bool withNulls) const {
  JSON::Builder builder;
  writeRowDict(builder, first, last, withNulls);
  return builder.getRoot();
}


Field DB::getField(unsigned i) const {
  assertInFieldRange(i);
  return &mysql_fetch_fields(res)[i];
}


Field::type_t DB::getType(unsigned i) const {
  return getField(i).getType();
}


unsigned DB::getLength(unsigned i) const {
  assertInFieldRange(i);
  return mysql_fetch_lengths(res)[i];
}


const char *DB::getData(unsigned i) const {
  assertInFieldRange(i);
  return res->current_row[i];
}


void DB::writeField(JSON::Sink &sink, unsigned i) const {
  if (getNull(i)) sink.writeNull();
  else {
    Field field = getField(i);

    if (field.isNumber()) sink.write(getDouble(i));
    else sink.write(getString(i));
  }
}


bool DB::getNull(unsigned i) const {return !res->current_row[i];}


string DB::getString(unsigned i) const {
  unsigned length = getLength(i);
  return string(res->current_row[i], length);
}


bool DB::getBoolean(unsigned i) const {return String::parseBool(getString(i));}


double DB::getDouble(unsigned i) const {
  if (!getField(i).isNumber())
    RAISE_ERROR("Field " << i << " is not a number");
  return String::parseDouble(getString(i));
}


uint32_t DB::getU32(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseU32(getString(i));
}


int32_t DB::getS32(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseS32(getString(i));
}


uint64_t DB::getU64(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseU64(getString(i));
}


int64_t DB::getS64(unsigned i) const {
  if (!getField(i).isInteger())
    RAISE_ERROR("Field " << i << " is not an integer");
  return String::parseS64(getString(i));
}


uint64_t DB::getBit(unsigned i) const {
  if (getType(i) != Field::TYPE_BIT)
    RAISE_ERROR("Field " << i << " is not bit");

  uint64_t x = 0;
  for (const char *ptr = res->current_row[i]; *ptr; ptr++) {
    x <<= 1;
    if (*ptr == '1') x |= 1;
  }

  return x;
}


void DB::getSet(unsigned i, set<string> &s) const {
  if (getType(i) != Field::TYPE_SET)
    RAISE_ERROR("Field " << i << " is not a set");

  const char *start = res->current_row[i];
  const char *end = start;

  while (*end) {
    if (*end == ',') {
      s.insert(string(start, end - start));
      start = end + 1;
    }
    end++;
  }
}


double DB::getTime(unsigned i) const {
  assertInFieldRange(i);

  char *s = res->current_row[i];
  unsigned len = res->lengths[i];

  // Parse decimal part
  double decimal = 0;
  char *ptr = strchr(s, '.');
  if (ptr) {
    decimal = String::parseDouble(ptr);
    len = ptr - s;
  }

  string time = string(s, len);

  switch (getType(i)) {
  case Field::TYPE_YEAR:
    if (len == 2) return decimal + Time::parse(time, "%y");
    return decimal + Time::parse(time, "%Y");

  case Field::TYPE_DATE:
    return decimal + Time::parse(time, "%Y-%m-%d");

  case Field::TYPE_TIME:
    return decimal + Time::parse(time, "%H%M%S");

  case Field::TYPE_TIMESTAMP:
  case Field::TYPE_DATETIME:
    return decimal + Time::parse(time, "%Y-%m-%d %H%M%S");

  default:
    RAISE_ERROR("Invalid time type");
    return 0;
  }
}


string DB::rowToString() const {
  ostringstream str;

  for (unsigned i = 0; i < getFieldCount(); i++) {
    if (i) str << ", ";
    str << getString(i);
  }

  return str.str();
}


string DB::getInfo() const {return mysql_info(db);}
const char *DB::getSQLState() const {return mysql_sqlstate(db);}
bool DB::hasError() const {return mysql_errno(db);}
string DB::getError() const {return mysql_error(db);}
unsigned DB::getErrorNumber() const {return mysql_errno(db);}


void DB::raiseError(const string &msg, bool withDBError) const {
  if (!withDBError || db) THROW("MariaDB: " << msg << ": " << getError());
  else THROW("MariaDB: " << msg);
}


unsigned DB::getWarningCount() const {return mysql_warning_count(db);}


void DB::assertConnected() const {
  if (!connected) RAISE_ERROR("Not connected");
}


void DB::assertPending() const {
  if (!nonBlocking || !status)
    RAISE_ERROR("Non-blocking call not pending");
}


void DB::assertNotPending() const {
  if (status) RAISE_ERROR("Non-blocking call still pending");
}


void DB::assertNonBlocking() const {
  if (!nonBlocking) RAISE_ERROR("Connection is not in nonBlocking mode");
}


void DB::assertHaveResult() const {
  if (!haveResult()) RAISE_ERROR("Don't have result, must call query() and "
                                 "useResult() or storeResult()");
}


void DB::assertNotHaveResult() const {
  if (haveResult())
    RAISE_ERROR("Already have result, must call freeResult()");
}


void DB::assertHaveRow() const {
  if (!haveRow())
    RAISE_ERROR("Don't have row, must call fetchRow()");
}


void DB::assertInFieldRange(unsigned i) const {
  if (getFieldCount() <= 0)
    RAISE_ERROR("Out of field range " << i);
}


bool DB::continueNB(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  assertPending();
  if (!continueFunc) RAISE_ERROR("Continue function not set");
  bool ret = (this->*continueFunc)(ready);
  if (ret) continueFunc = 0;
  return ret;
}


bool DB::waitRead()    const {return status & MYSQL_WAIT_READ;}
bool DB::waitWrite()   const {return status & MYSQL_WAIT_WRITE;}
bool DB::waitExcept()  const {return status & MYSQL_WAIT_EXCEPT;}
bool DB::waitTimeout() const {return status & MYSQL_WAIT_TIMEOUT;}
int DB::getSocket()    const {return mysql_get_socket(db);}


double DB::getTimeout() const {
  return (double)mysql_get_timeout_value_ms(db) / 1000.0; // millisec -> sec
}


string DB::escape(const string &s) {
  string result;
  result.reserve(s.length());

  for (auto c: s)
    switch (c) {
    case 0:    result.append("\\0");  break;
    case '\'': result.append("\\'");  break;
    case '\"': result.append("\\\""); break;
    case '\b': result.append("\\b");  break;
    case '\n': result.append("\\n");  break;
    case '\r': result.append("\\r");  break;
    case '\t': result.append("\\t");  break;
    case 26:   result.append("\\Z");  break;
    case '\\': result.append("\\\\"); break;
    default:   result.push_back(c);   break;
    }

  return result;
}


string DB::toHex(const string &s) {
  SmartPointer<char>::Array to = new char[s.length() * 2 + 1];

  unsigned len = mysql_hex_string(to.get(), s.data(), s.length());

  return string(to.get(), len);
}


string DB::formatBool(bool      value) {return value ? "true" : "false";}
string DB::format(double        value) {return String(value);}
string DB::format(int32_t       value) {return String(value);}
string DB::format(uint32_t      value) {return String(value);}
string DB::format(const string &value) {return "'" + escape(value) + "'";}


string DB::format(const string &s, const JSON::Value &dict) {
  auto cb = [&] (const string &id, const string &spec) -> string {
    auto value = dict.select(id, 0);
    if (value.isNull()) return formatNull();
    if (spec == "s")    return format(value->asString());
    return value->formatAs(spec);
  };

  return String(s).format(cb);
}


void DB::libraryInit(int argc, char *argv[], char *groups[]) {
  if (mysql_library_init(argc, argv, groups)) THROW("Failed to init MariaDB");
}


void DB::libraryEnd() {mysql_library_end();}
const char *DB::getClientInfo() {return mysql_get_client_info();}


void DB::threadInit() {
  if (mysql_thread_init()) THROW("Failed to init MariaDB threads");
}


void DB::threadEnd() {mysql_thread_end();}
bool DB::threadSafe() {return mysql_thread_safe();}


bool DB::closeContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  status = mysql_close_cont(db, ready);
  if (status) return false;

  db = mysql_init(0);
  connected = false;
  return true;
}


bool DB::connectContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  MYSQL *db = 0;
  status = mysql_real_connect_cont(&db, this->db, ready);
  if (status) return false;

  if (!db) RAISE_DB_ERROR("Failed to connect");
  connected = true;

  return true;
}


bool DB::resetConnectionContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_reset_connection_cont(&ret, db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to reset DB connection");

  return true;
}


bool DB::changeUserContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  my_bool ret = false;
  status = mysql_change_user_cont(&ret, db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to change DB user");

  return true;
}


bool DB::pingContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_ping_cont(&ret, db, ready);
  if (status) return false;

  connected = !ret;

  return true;
}


bool DB::useContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_select_db_cont(&ret, this->db, ready);
  if (status) return false;

  if (ret) RAISE_DB_ERROR("Failed to select DB");

  return true;
}


bool DB::queryContinue(unsigned ready) {
  int ret = 0;
  status = mysql_real_query_cont(&ret, this->db, ready);

  LOG_DEBUG(5, CBANG_FUNC << "() ready=" << ready << " status=" << status
            << " ret=" << ret);

  if (status) return false;
  if (ret) RAISE_DB_ERROR("Query failed");

  return true;
}


bool DB::storeResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  status = mysql_store_result_cont(&res, this->db, ready);
  if (status) return false;

  if (res) stored = true;
  else if (hasError()) RAISE_DB_ERROR("Failed to store result");

  return true;
}


bool DB::nextResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  int ret = 0;
  status = mysql_next_result_cont(&ret, this->db, ready);
  if (status) return false;

  if (0 < ret) RAISE_DB_ERROR("Failed to get next result");
  if (ret) LOG_DEBUG(5, "No more results");

  return true;
}


bool DB::freeResultContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  status = mysql_free_result_cont(res, ready);
  if (status) return false;

  res = 0;

  return true;
}


bool DB::fetchRowContinue(unsigned ready) {
  LOG_DEBUG(5, CBANG_FUNC << "()");

  MYSQL_ROW row = 0;
  status = mysql_fetch_row_cont(&row, res, ready);

  return !status;
}

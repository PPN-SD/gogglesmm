/*******************************************************************************
*                         Goggles Music Manager                                *
********************************************************************************
*           Copyright (C) 2006-2014 by Sander Jansen. All Rights Reserved      *
*                               ---                                            *
* This program is free software: you can redistribute it and/or modify         *
* it under the terms of the GNU General Public License as published by         *
* the Free Software Foundation, either version 3 of the License, or            *
* (at your option) any later version.                                          *
*                                                                              *
* This program is distributed in the hope that it will be useful,              *
* but WITHOUT ANY WARRANTY; without even the implied warranty of               *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                *
* GNU General Public License for more details.                                 *
*                                                                              *
* You should have received a copy of the GNU General Public License            *
* along with this program.  If not, see http://www.gnu.org/licenses.           *
********************************************************************************/
#ifndef GMDATABASE_H
#define GMDATABASE_H

struct sqlite3_stmt;
struct sqlite3;

class GMDatabaseException {
  };

class GMDatabase;

class GMQuery{
friend class GMDatabase;
private:
  sqlite3_stmt * statement;
private:
  GMQuery(const GMQuery&);
  GMQuery& operator=(const GMQuery&);
protected:
  void fatal() const;
public:
  /// Empty Statement
  GMQuery();

  /// Compile new query
  GMQuery(GMDatabase *,const FXchar*);

  /// Create from precompiled query
  GMQuery(sqlite3_stmt * statement);

  /// Assign precompiled query
  GMQuery& operator=(sqlite3_stmt* statement);

  /// Destructor
  ~GMQuery();
public: /// Parameter / Column retrieval
  void set(FXint p,FXint v);
  void set(FXint p,FXuint v);
  void set(FXint p,FXlong v);
  void set(FXint p,FXfloat v);
  void set(FXint p,FXdouble v);
  void set(FXint p,const FXString & text);
  void set(FXint p,const void * data,FXuval size);

  void get(FXint p,FXint &);
  void get(FXint p,FXuint &);
  void get(FXint p,FXlong &);
  void get(FXint p,FXfloat &);
  void get(FXint p,FXdouble &);
  void get(FXint p,FXuchar *&,FXuval&);
  void get(FXint p,FXString&);
  const FXchar * get(FXint p);
public:
  /// Reset query for another exection
  void reset();

  /// Clear query
  void clear();

  /// Execute query, fail if rows are returned.
  void execute();

  /// Run query, return true if row is returned
  FXbool row();
public:
  void execute(FXint&);
  void execute(FXString&);
  void execute(const FXint,FXint&);
  void execute(const FXint,FXString&);
  void execute(const FXString&,FXint&);
  void update(const FXint);
  void update(const FXString&);
  FXint insert(const FXString&);
  FXint insert();

  static void makeSelection(const FXIntList & list,FXString & selection);

  };


class GMDatabase {
friend class GMQuery;
private:
  sqlite3 * db;
private:
  static FXMutex     mutex;
  static FXCondition condition;
public:
  static volatile FXbool interrupt;
private:
  void lock();
  void unlock();
  void fatal(const FXchar * q=NULL) const;
protected:
  GMDatabase(const GMDatabase&);
  GMDatabase& operator=(const GMDatabase&);
protected:
public:
  GMDatabase();

  /// Open Database; Return TRUE if succeeds else FALSE
  FXbool open(const FXString & filename);

  /// Close Database
  void close();

  /// Clear the whole database
  void reset();

  /// Compile Query
  sqlite3_stmt * compile(const FXchar * statement);
  sqlite3_stmt * compile(const FXString & statement) { return compile(statement.text()); }

  /// Run
  void execute(const FXchar*);
  void execute(const FXString &);
  void executeFormat(const FXchar * query,...);


  FXint insert(GMQuery&);
  FXint insert(const FXString &);

  /// Results
  void execute(const FXchar * query,FXint &);
  void execute(const FXchar * query,const FXString &,FXint &);
  void execute(const FXchar * query,const FXint,FXString &);





  /// Transactions
  void begin();
  void commit();
  void rollback();

  void beginTask();
  void commitTask();
  void rollbackTask();
  void waitTask();

  FXint rowid() const;
  FXint changes() const;

  /// Set Version
  void setVersion(FXint v);

  /// Get Version
  FXint getVersion();


  static FXbool threadsafe();
  static const FXchar * version();

  /// Close Database
  virtual ~GMDatabase();
  };

#endif


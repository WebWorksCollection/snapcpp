/*
 * Text:
 *      QCassandraQuery.h
 *
 * Description:
 *      Handling of the cassandra::CfDef (Column Family Definition).
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2016 Made to Order Software Corp.
 * 
 *      http://snapwebsites.org/
 *      contact@m2osw.com
 * 
 *      Permission is hereby granted, free of charge, to any person obtaining a
 *      copy of this software and associated documentation files (the
 *      "Software"), to deal in the Software without restriction, including
 *      without limitation the rights to use, copy, modify, merge, publish,
 *      distribute, sublicense, and/or sell copies of the Software, and to
 *      permit persons to whom the Software is furnished to do so, subject to
 *      the following conditions:
 *
 *      The above copyright notice and this permission notice shall be included
 *      in all copies or substantial portions of the Software.
 *
 *      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 *      OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *      IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 *      CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 *      TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 *      SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#pragma once

#include <map>
#include <memory>
#include <string>

#include <QObject>
#include <QString>
#include <QByteArray>

#include "QtCassandra/QCassandraConsistencyLevel.h"
#include "QtCassandra/QCassandraSession.h"


namespace QtCassandra
{


class QCassandraQuery
    : public QObject
    , std::enable_shared_from_this<QCassandraQuery>
{
    Q_OBJECT

public:
    typedef std::shared_ptr<QCassandraQuery>  pointer_t;
    typedef std::map<std::string,std::string> string_map_t;

    QCassandraQuery( QCassandraSession::pointer_t session );
    ~QCassandraQuery();

    consistency_level_t	consistencyLevel() const;
    void                setConsistencyLevel( consistency_level_t level );

    int64_t				timestamp() const;
    void				setTimestamp( int64_t val );

    void       query         ( const QString& query_string, const int bind_count = 0 );
    void       setPagingSize ( const int size );

    void       bindBool      ( const size_t num, const bool          value );
    void       bindInt32     ( const size_t num, const int32_t       value );
    void       bindInt64     ( const size_t num, const int64_t       value );
    void       bindFloat     ( const size_t num, const float         value );
    void       bindDouble    ( const size_t num, const double        value );
    void       bindString    ( const size_t num, const QString&      value );
    void       bindByteArray ( const size_t num, const QByteArray&   value );
    void       bindJsonMap   ( const size_t num, const string_map_t& value );
    void       bindMap       ( const size_t num, const string_map_t& value );

    void       start( const bool block = true );
    bool	   isReady() const;
    void       getQueryResult();
    bool       nextRow();
    bool       nextPage( const bool block = true );
    void       end();

    bool       getBoolColumn      ( const QString& name  ) const;
    bool       getBoolColumn      ( const int      num   ) const;
    int32_t    getInt32Column     ( const QString& name  ) const;
    int32_t    getInt32Column     ( const int      num   ) const;
    int64_t    getInt64Column     ( const QString& name  ) const;
    int64_t    getInt64Column     ( const int      num   ) const;
    float      getFloatColumn     ( const QString& name  ) const;
    float      getFloatColumn     ( const int      num   ) const;
    double     getDoubleColumn    ( const QString& name  ) const;
    double     getDoubleColumn    ( const int      num   ) const;
    QString    getStringColumn    ( const QString& name  ) const;
    QString    getStringColumn    ( const int      num   ) const;
    QByteArray getByteArrayColumn ( const char *   name  ) const;
    QByteArray getByteArrayColumn ( const QString& name  ) const;
    QByteArray getByteArrayColumn ( const int      num   ) const;

    string_map_t getJsonMapColumn ( const QString& name ) const;
    string_map_t getJsonMapColumn ( const int num ) const;
    string_map_t getMapColumn     ( const QString& name ) const;
    string_map_t getMapColumn     ( const int num ) const;

private slots:
    void onQueryFinishedTimer();

signals:
    void queryFinished( QCassandraQuery::pointer_t q );
    void threadQueryFinished( QCassandraQuery::pointer_t q );

private:
    // Current query
    //
    QCassandraSession::pointer_t   f_session;
    QString                        f_queryString;
    CassTools::statement_pointer_t f_queryStmt;
    CassTools::future_pointer_t    f_sessionFuture;
    CassTools::result_pointer_t    f_queryResult;
    CassTools::iterator_pointer_t  f_rowsIterator;
    consistency_level_t			   f_consistencyLevel = CONSISTENCY_LEVEL_DEFAULT;
    int64_t						   f_timestamp = 0;
    int64_t						   f_timeout = 0;

    void 		    setStatementConsistency();
    void 		    setStatementTimestamp();
    bool 		    getBoolFromValue      ( const CassValue* value ) const;
    QByteArray      getByteArrayFromValue ( const CassValue* value ) const;
    string_map_t    getMapFromValue       ( const CassValue* value ) const;
    void            throwIfError          ( const QString& msg     );

    void			fireQueryTimer();
    static void		queryCallbackFunc( CassFuture* future, void *data );
};


}
// namespace QtCassandra

    
// vim: ts=4 sw=4 et

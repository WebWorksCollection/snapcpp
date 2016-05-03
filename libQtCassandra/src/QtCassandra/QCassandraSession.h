/*
 * Text:
 *      QCassandraSession.h
 *
 * Description:
 *      Handling of the connection to the QCassandra database via the cassandra-cpp-driver API.
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

#include <QString>
#include <QByteArray>

typedef struct CassCluster_      CassCluster;
typedef struct CassCollection_   CassCollection;
typedef struct CassColumnMeta_   CassColumnMeta;
typedef struct CassFuture_       CassFuture;
typedef struct CassIterator_     CassIterator;
typedef struct CassKeyspaceMeta_ CassKeyspaceMeta;
typedef struct CassResult_       CassResult;
typedef struct CassSchemaMeta_   CassSchemaMeta;
typedef struct CassSession_      CassSession;
typedef struct CassStatement_    CassStatement;
typedef struct CassTableMeta_    CassTableMeta;
typedef struct CassValue_        CassValue;


namespace QtCassandra
{


namespace CassTools
{
    typedef std::shared_ptr<CassCluster>            cluster_pointer_t;
    typedef std::shared_ptr<CassCollection>         collection_pointer_t;
    typedef std::shared_ptr<const CassColumnMeta>   column_meta_pointer_t;
    typedef std::shared_ptr<CassFuture>             future_pointer_t;
    typedef std::shared_ptr<CassIterator>           iterator_pointer_t;
    typedef std::shared_ptr<const CassKeyspaceMeta> keyspace_meta_pointer_t;
    typedef std::shared_ptr<const CassResult>       result_pointer_t;
    typedef std::shared_ptr<const CassSchemaMeta>   schema_meta_pointer_t;
    typedef std::shared_ptr<const CassTableMeta>    table_meta_pointer_t;
    typedef std::shared_ptr<CassSession>            session_pointer_t;
    typedef std::shared_ptr<CassStatement>          statement_pointer_t;
    typedef std::shared_ptr<const CassValue>        value_pointer_t;
    typedef int64_t                                 timeout_t;
}




class QCassandraSession
        : public std::enable_shared_from_this<QCassandraSession>
{
public:
    typedef std::shared_ptr<QCassandraSession> pointer_t;

    static CassTools::timeout_t const       DEFAULT_TIMEOUT = 12 * 1000; // 12s

    static pointer_t create();
    ~QCassandraSession();

    void connect( const QString& host = "localhost", const int port = 9042 );
    void connect( const QStringList& host_list     , const int port = 9042 );
    void disconnect();
    bool isConnected() const;

    CassTools::cluster_pointer_t cluster()    const;
    CassTools::session_pointer_t session()    const;
    CassTools::future_pointer_t  connection() const;

    CassTools::timeout_t timeout() const;
    CassTools::timeout_t setTimeout(CassTools::timeout_t timeout_ms);

private:
    QCassandraSession();

    CassTools::cluster_pointer_t        f_cluster;
    CassTools::session_pointer_t        f_session;
    CassTools::future_pointer_t         f_connection;
    CassTools::timeout_t                f_timeout = DEFAULT_TIMEOUT; // 12s
};


class QCassandraRequestTimeout
{
public:
    QCassandraRequestTimeout(QCassandraSession::pointer_t session, CassTools::timeout_t timeout_ms)
        : f_session(session)
        , f_old_timeout(f_session->setTimeout(timeout_ms))
    {
    }

    ~QCassandraRequestTimeout()
    {
        f_session->setTimeout(f_old_timeout);
    }

private:
    QCassandraSession::pointer_t    f_session;
    int64_t                         f_old_timeout;
};


}
// namespace QtCassandra
// vim: ts=4 sw=4 et

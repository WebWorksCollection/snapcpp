/*
 * Text:
 *      snapdbproxy_connection.cpp
 *
 * Description:
 *      Each connection is managed by a thread. This file implements that
 *      thread. The thread lasts as long as the connection. Once the connect
 *      gets closed by the client, the thread terminates.
 *
 *      TODO: we certainly want to look into reusing threads in a pool
 *            instead of having a onetime run like we have now.
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
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

// ourselves
//
#include "snapdbproxy.h"

// our lib
//
#include <snapwebsites/log.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>
#include <snapwebsites/dbutils.h>

// advgetopt lib
//
#include <advgetopt/advgetopt.h>

// QtCassandra lib
//
#include <QtCassandra/QCassandra.h>
#include <QtCassandra/QCassandraSchema.h>

// C++ lib
//
#include <algorithm>
#include <iostream>
#include <sstream>

// C lib
//
#include <poll.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>


// a mutex to manage data common to all connections
//
snap::snap_thread::snap_mutex  g_connections_mutex;

// the DESCRIBE CLUSTER is very slow, this is a cached version which
// is reset once in a while when certain orders happen (i.e. create
// remove a context, table, or alter a context, table, column.)
//
QByteArray  g_cluster_description;


void signalfd_deleted(int s)
{
    close(s);
}


int64_t timeofday()
{
    struct timeval tv;

    // we ignore timezone as it can also generate an error
    if(gettimeofday( &tv, NULL ) != 0)
    {
        throw std::runtime_error("gettimeofday() failed.");
    }

    return static_cast<int64_t>( tv.tv_sec ) * 1000000 +
           static_cast<int64_t>( tv.tv_usec );
}


pid_t gettid()
{
    return syscall(SYS_gettid);
}





snapdbproxy_connection::snapdbproxy_connection
    ( snapdbproxy* proxy
    , QtCassandra::QCassandraSession::pointer_t session
    , tcp_client_server::bio_client::pointer_t & client
    , QString const & cassandra_host_list
    , int cassandra_port
    , bool use_ssl
    )
        : snap_runner("snapdbproxy_connection")
        , f_snapdbproxy(proxy)
        //, f_proxy()
        , f_session(session)
        //, f_cursors() -- auto-init
        //, f_client(nullptr) -- auto-init
        , f_socket(client->get_socket())
        , f_cassandra_host_list(cassandra_host_list)
        , f_cassandra_port(cassandra_port)
        , f_use_ssl(use_ssl)
{
    // take ownership of the client's pointer (we are not in the thread
    // yet, so a simple swap is sufficient)
    //
    f_client.swap(client);

    // the parent (main) thread will shutdown the socket if it receives
    // the STOP message from snapcommunicator, see the kill() function
    // for details
}


snapdbproxy_connection::~snapdbproxy_connection()
{
    // Note:
    // The f_client object was likely reset() when we reach this function
    // since the run() loop exists only if the f_client pointer is nullptr.
}


void snapdbproxy_connection::run()
{
    // let the other process push the whole order before moving forward
    // (this made no difference in the potential slowness on various
    // computers so I did not keep it)
    //sched_yield();

    int const socket_on_entry(f_socket);
    SNAP_LOG_TRACE("starting new snapdbproxy connection thread (socket: ")(socket_on_entry)(").");

    try
    {
        while(f_client != nullptr)
        {
            // wait for an order
            //
            QtCassandra::QCassandraOrder order(f_proxy.receiveOrder(*this));
            if(order.validOrder()
            && f_session->isConnected())
            // && !thread->is_stopping()) -- we do not have access to the thread
            //                               and the pthread_kill() should be more
            //                               than enough at this point
            {
                // order can be executed now
                //
// TODO: remark this code off after we have resolved SNAP-493.
//
#if 0
SNAP_LOG_TRACE("got an order: ")
        (static_cast<int>(order.get_type_of_result()))
        (", CQL \"")(order.cql())
        ("\" (")(order.columnCount())
        (" columns).");
//std::cerr << "[" << getpid() << ":" << gettid() << "] got new order \"" << order.cql() << "\" in snapdbproxy at " << timeofday() << "\n";
#endif
                switch(order.get_type_of_result())
                {
                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_CLOSE:
                    close_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_DECLARE:
                    declare_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_DESCRIBE:
                    describe_cluster(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_FETCH:
                    fetch_cursor(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_ROWS:
                    read_data(order);
                    break;

                case QtCassandra::QCassandraOrder::TYPE_OF_RESULT_SUCCESS:
                    execute_command(order);
                    break;

                }

                // the order may include the flag telling us that the
                // cluster schema may have changed and if so we have
                // to clear our memory cache
                //
                if(order.clearClusterDescription())
                {
                    clear_cluster_description();
                }
            }
            else
            {
                // in most cases if the order is not valid the connect
                // was hung up; it could also be an invalid protocol
                // or some transmission error (although really, with
                // TCP/IP transmission errors rarely happen.)
                //
                if(order.validOrder())
                {
                    SNAP_LOG_TRACE("snapdbproxy connection socket is gone (")(f_client != nullptr ? f_client->get_socket() : -1)(").");
                }
                else
                {
                    SNAP_LOG_TRACE("snapdbproxy received an invalid order (")(f_client != nullptr ? f_client->get_socket() : -1)(").");
                }

                close();
            }
        }
    }
    catch( QtCassandra::QCassandraQuery::query_exception_t const& e )
    {
        if( e.getCode() == 16777226 )
        {
            SNAP_LOG_ERROR("thread received QCassandraQuery::query_exception \"")(e.what())("\", reconnecting to Cassandra server!");

            // No hosts available! We must have lost the connection.
            // Tell the parent proxy object we need to reset.
            //
            f_snapdbproxy->no_cassandra();
        }
        else
        {
            SNAP_LOG_WARNING("thread received QCassandraQuery::query_exception \"")(e.what())("\"");
        }

        close();
    }
    catch(std::exception const & e)
    {
        SNAP_LOG_WARNING("thread received std::exception \"")(e.what())("\"");

        close();
    }
    catch(...)
    {
        SNAP_LOG_WARNING("thread received an unknown exception");

        close();
    }
    // exit thread normally

    SNAP_LOG_TRACE("ending snapdbproxy connection thread (")(socket_on_entry)(").");
}


/** \brief Read count bytes to the specified buffer.
 *
 * This function reads \p count bytes from the socket managed by
 * this connection. The bytes are saved in the specified \p buf
 * buffer.
 *
 * If an error occurs before any data was read, the function returns
 * -1.
 *
 * If data was already read and an error occurs, the function returns
 * the number of bytes already read.
 *
 * If no error occurs, the function reads all the data from the socket
 * and returns \p count.
 *
 * \note
 * If the thread receives the SIGUSR1 signal, it is considered to be an
 * error. The function will stop immediately returning the number of
 * bytes already read. If nothing was read, then the function
 * returns -1 and sets errno to ECANCEL.
 *
 * \exception std::logic_error
 * This function raises this error if somehow it detects a size
 * mistmatch. This exception should never happen.
 *
 * \param[in] buf  A pointer to a buffer of data to send to the client.
 * \param[in] count  The number of bytes to write to the client.
 *
 * \return The number of bytes written or -1 if no bytes get written and
 *         an error occurs.
 */
ssize_t snapdbproxy_connection::read(void * buf, size_t count)
{
    if(f_client == nullptr)
    {
        return -1L;
    }

    if(count == 0)
    {
        return 0;
    }

    // we are supposed to have a blocking socket, but with large amounts
    // of data the read() may return less than count bytes, for this
    // reason we have to have a loop
    //
    size_t size(0);
    for(;;)
    {
        ssize_t const r(f_client->read(reinterpret_cast<char *>(buf), count));
        if(r < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapdbproxy_connection::read() returned with ")(r)(", errno ")(e)(", ")(strerror(e));
            // TBD: we could "return size > 0 ? size : -1L;" too...
            return -1L;
        }
        if(r > 0)
        {
            count -= r;
            size += r;
            if(count == 0)
            {
                return size;
            }
            buf = reinterpret_cast<char *>(buf) + r;
            SNAP_LOG_TRACE("snapdbproxy_connection::read() needs more than one call (")(count)("/")(size)(").");
        }
        else
        {
            // wait a bit and try again
            // (this is when we receive a return value of 0)
            //
            struct pollfd fd;
            fd.fd = f_client->get_socket();
            fd.events = POLLIN | POLLPRI | POLLRDHUP | POLLHUP;
            snap::NOTUSED(poll(&fd, 1, 0));
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                // this happens all the time so we just use a trace on it
                // (at first it was an error)
                //
                SNAP_LOG_TRACE("snapdbproxy_connection::read() attempted to read from a socket that is closed.");
                return -1L;
            }
        }
    }
}


/** \brief Write the count bytes of the specified buffer.
 *
 * This function writes the specified buffer to the socket managed by
 * this connection. The number of bytes written to the socket is
 * specified by count.
 *
 * If an error occurs before any data was written, the function returns
 * -1.
 *
 * If data was already written and an error occurs, the function returns
 * the number of bytes already written.
 *
 * If no error occurs, the function writes all the data to the socket
 * and returns \p count.
 *
 * \note
 * If the thread receives the SIGUSR1 signal, it is considered to be an
 * error. The function will stop immediately returning the number of
 * bytes already written. If nothing was written, then the function
 * returns -1 and sets errno to ECANCEL.
 *
 * \exception std::logic_error
 * This function raises this error if somehow it detects a size
 * mistmatch. This exception should never happen.
 *
 * \param[in] buf  A pointer to a buffer of data to send to the client.
 * \param[in] count  The number of bytes to write to the client.
 *
 * \return The number of bytes written or -1 if no bytes get written and
 *         an error occurs.
 */
ssize_t snapdbproxy_connection::write(void const * buf, size_t count)
{
    // make sure the client is valid
    //
    if(f_client == nullptr)
    {
        return -1L;
    }
    int const socket(f_client->get_socket());
    if(socket < 0)
    {
        return -1L;
    }

    // anything to write?
    //
    if(count == 0)
    {
        return 0;
    }

    // we are supposed to have a blocking socket, but with large amounts
    // of data the write() may accept less than count bytes, for this
    // reason we have to have a loop
    //
    size_t size(0);
    for(;;)
    {
        ssize_t const r(::write(socket, buf, count));
        if(r < 0)
        {
            int const e(errno);
            SNAP_LOG_ERROR("snapdbproxy_connection::write() returned with ")(r)(", errno ")(e)(", ")(strerror(e));
            // TBD: we could "return size > 0 ? size : -1L;" too...
            return -1L;
        }
        if(r > 0)
        {
            count -= r;
            size += r;
            if(count == 0)
            {
                return size;
            }
            buf = reinterpret_cast<char const *>(buf) + r;
            SNAP_LOG_TRACE("snapdbproxy_connection::write() needs more than one call (")(count)("/")(size)(").");
        }
        else
        {
            // wait a bit and try again
            // (this is when we receive a return value of 0)
            //
            struct pollfd fd;
            fd.fd = socket;
            fd.events = POLLOUT | POLLRDHUP | POLLHUP;
            snap::NOTUSED(poll(&fd, 1, 0));
            if((fd.revents & (POLLHUP | POLLRDHUP)) != 0)
            {
                SNAP_LOG_ERROR("snapdbproxy_connection::write() attempted to write to a socket that is closed.");
                return -1L;
            }
        }
    }
}


void snapdbproxy_connection::close()
{
    {
        snap::snap_thread::snap_lock lock(f_mutex);
        f_socket = -1;
    }

    // the client is only in the thread runner so it is safe to do
    // that outside of the lock
    //
    f_client.reset();
}


void snapdbproxy_connection::kill()
{
    snap::snap_thread::snap_lock lock(f_mutex);

    // parent thread wants to quit, tell the child to exit ASAP
    // by partially shutting down the socket
    //
    if(f_socket != -1)
    {
        // Note: when we reach this function the socket may have been closed
        //       already, the shutdown will just fail (note however that we
        //       are safe from shutting down another socket since we just
        //       checked whether it was not -1 so we did not accept another
        //       socket in between and f_socket will either still be opened
        //       or -1)
        //
        snap::NOTUSED(::shutdown(f_socket, SHUT_RD));
    }
}


void snapdbproxy_connection::send_order(QtCassandra::QCassandraQuery::pointer_t q, QtCassandra::QCassandraOrder const & order)
{
    size_t const count(order.parameterCount());

    // This generates way too many logs. It's like all the orders that
    // should not have ONE do have ONE instead of QUORUM. For now we
    // force QUORUM in the QCassandraQuery class instead of worrying of
    // where the bug really is. This will work just fine for us, which is
    // why we do it this way.
    //if( order.consistencyLevel() != QtCassandra::CONSISTENCY_LEVEL_QUORUM)
    //{
    //    SNAP_LOG_WARNING("Consistency ")
    //                    (order.consistencyLevel())
    //                    (" instead of the usually expected QUORUM for [")
    //                    (order.cql())
    //                    ("]");
    //}

    // CQL order
    q->query( order.cql(), count );

    // Parameters
    for(size_t idx(0); idx < count; ++idx)
    {
        q->bindByteArray( idx, order.parameter(idx) );
    }

    // Consistency Level
    q->setConsistencyLevel( order.consistencyLevel() );

    // Timestamp
    q->setTimestamp(order.timestamp());

    // Paging Size
    int32_t const paging_size(order.pagingSize());
    if(paging_size > 0)
    {
        q->setPagingSize(paging_size);
    }

    // run the CQL order
    q->start();
}


void snapdbproxy_connection::declare_cursor(QtCassandra::QCassandraOrder const & order)
{
    cursor_t cursor;
    cursor.f_query = QtCassandra::QCassandraQuery::create(f_session);
    cursor.f_column_count = order.columnCount();

    // in this case we have to keep the query so we allocate it
    //
    send_order(cursor.f_query, order);

    QtCassandra::QCassandraOrderResult result;
    QByteArray cursor_index;
    QtCassandra::appendUInt32Value(cursor_index, f_cursors.size());
    result.addResult(cursor_index);

    while(cursor.f_query->nextRow())
    {
        for(int idx(0); idx < cursor.f_column_count; ++idx)
        {
            result.addResult( cursor.f_query->getByteArrayColumn( idx ) );
        }
    }

    f_cursors.push_back(cursor);

    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        close();
    }
}


void snapdbproxy_connection::describe_cluster(QtCassandra::QCassandraOrder const & order)
{
    snap::NOTUSED(order);

    QtCassandra::QCassandraOrderResult result;

    {
        snap::snap_thread::snap_lock lock(g_connections_mutex);

        if(g_cluster_description.isEmpty())
        {
            // load the meta data
            QtCassandra::QCassandraSchema::SessionMeta::pointer_t session_meta( QtCassandra::QCassandraSchema::SessionMeta::create(f_session) );
            session_meta->loadSchema();
            g_cluster_description = session_meta->encodeSessionMeta();
        }

        // convert the meta data to a blob and send it over the wire
        result.addResult( g_cluster_description );
    }

    result.setSucceeded(true);

    if(!f_proxy.sendResult(*this, result))
    {
        close();
    }
}


void snapdbproxy_connection::clear_cluster_description()
{
    snap::snap_thread::snap_lock lock(g_connections_mutex);

    g_cluster_description.clear();
}


void snapdbproxy_connection::fetch_cursor(QtCassandra::QCassandraOrder const & order)
{
    int const cursor_index(order.cursorIndex());
    if(static_cast<size_t>(cursor_index) >= f_cursors.size())
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor index is out of bounds, it may already have been closed.");
    }
    QtCassandra::QCassandraQuery::pointer_t q(f_cursors[cursor_index].f_query);
    if(!q)
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor was already closed.");
    }

    QtCassandra::QCassandraOrderResult result;

    if(q->nextPage())
    {
        // TBD: add the cursor_index on a fetch? probably not required...
        //result.addResult(...);

        int const column_count(f_cursors[cursor_index].f_column_count);
        while(q->nextRow())
        {
            for(int idx(0); idx < column_count; ++idx)
            {
                result.addResult( q->getByteArrayColumn( idx ) );
            }
        }
    }

    // send the following or an empty set (an empty set means we reached
    // the last page!)
    //
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        close();
    }
}


void snapdbproxy_connection::close_cursor(QtCassandra::QCassandraOrder const & order)
{
    // verify that the specified index is considered valid on this side
    //
    int const cursor_index(order.cursorIndex());
    if(static_cast<size_t>(cursor_index) >= f_cursors.size())
    {
        throw snap::snapwebsites_exception_invalid_parameters("cursor index is out of bounds.");
    }

    // send an empty, successful reply in this case
    //
    QtCassandra::QCassandraOrderResult result;
    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        close();
    }

    // now actually do the clean up
    // (we can do that after we sent the reply since we are one separate
    // process, yet the process is fully synchronized on the TCP/IP socket)
    //
    f_cursors[cursor_index].f_query.reset();

    // remove all the cursors that were closed if possible so the
    // vector does not grow indefinitly
    //
    while(!f_cursors.empty() && !f_cursors.rbegin()->f_query)
    {
        f_cursors.pop_back();
    }
}


void snapdbproxy_connection::read_data(QtCassandra::QCassandraOrder const & order)
{
    auto q( QtCassandra::QCassandraQuery::create( f_session ) );
    send_order(q, order);

    QtCassandra::QCassandraOrderResult result;

    if( q->nextRow() )
    {
        // the list of columns may vary so we get the count
        int const max_columns(order.columnCount());
        for(int idx(0); idx < max_columns; ++idx)
        {
            result.addResult(q->getByteArrayColumn( idx ));
        }
    }

    result.setSucceeded(true);
    if(!f_proxy.sendResult(*this, result))
    {
        close();
    }
}


void snapdbproxy_connection::execute_command(QtCassandra::QCassandraOrder const & order)
{
    QtCassandra::QCassandraSession::pointer_t order_session;

SNAP_LOG_WARNING("------- got a simple command: ")(order.cql());
    if(order.timeout() > 0)
    {
        // unfortunately, the request timeout cannot be changed in an
        // existing session (a connected session, to be precise); the
        // only way to get that to work is to change the timeout (in
        // the cluster config_) and then create a new session connection...
        //
        // see: https://datastax-oss.atlassian.net/browse/CPP-362
        //      https://datastax-oss.atlassian.net/browse/CPP-300
        //
        //SNAP_LOG_INFO("creating sub-Cassandra-session for this slow order (")(order.cql())(")");
        order_session = QtCassandra::QCassandraSession::create();
        {
            snap::snap_thread::snap_lock lock(g_connections_mutex);

            QtCassandra::QCassandraRequestTimeout request_timeout(order_session, order.timeout());
            order_session->connect( f_cassandra_host_list, f_cassandra_port, f_use_ssl ); // throws on failure!
        }
SNAP_LOG_WARNING("created sub-session with timeout: ")(order.timeout());
    }
    else
    {
        order_session = f_session;
    }

    auto q( QtCassandra::QCassandraQuery::create( order_session ) );
SNAP_LOG_WARNING("send order now...");
    send_order(q, order);
SNAP_LOG_WARNING("order done...");

    // success
    QtCassandra::QCassandraOrderResult result;
    result.setSucceeded(true);
SNAP_LOG_WARNING("reply with success...");
    if(!f_proxy.sendResult(*this, result))
    {
SNAP_LOG_WARNING("reply failed, close connection...");
        close();
    }
else SNAP_LOG_WARNING("result sent successfully...");
}


// vim: ts=4 sw=4 et

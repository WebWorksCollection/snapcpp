/*
 * Text:
 *      QCassandraProxy.cpp
 *
 * Description:
 *      Handle sending CQL orders to the snapproxy and receiving the
 *      Cassandra results.
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

// ourselves
//
#include "QtCassandra/QCassandraProxy.h"
#include "QtCassandra/QCassandra.h"

// 3rd party lib
//
#include <QtCore>

// C++ lib
//
#include <iostream>
#include <sstream>

// OS libs
//
#include <openssl/err.h>
#include <unistd.h>


namespace QtCassandra
{


namespace
{


/** \brief Release a BIO object.
 *
 * This function is used with the bio shared pointer to make sure we
 * can automatically remove the bio.
 */
void bio_deleter(BIO * bio)
{
    BIO_free_all(bio);
}


bool g_bio_initialized = false;
void bio_initialize()
{
    // already initialized?
    if(g_bio_initialized)
    {
        return;
    }
    g_bio_initialized = true;

    // Make sure the SSL library gets initialized
    //SSL_library_init();

    // TBD: should we call the load string functions only when we
    //      are about to generate an error?
    ERR_load_crypto_strings();
    //ERR_load_SSL_strings();

    // TODO: define a way to only define safe algorithm
    //       (it looks like we can force TLSv1 below at least)
    //OpenSSL_add_all_algorithms();

    // TBD: need a PRNG seeding before creating a new SSL context?
}

// this fast_buffer implements a very fast "lets use the stack if
// we do not need to much memory allocated" so that way we can
// avoid many malloc()'s
//
// many times, the reply will be very small (less than 1Kb)
// so using the stack for that purpose is an incredible saver!
// (i.e. in assembly, the stack allocation is one SUB SSP, 4096
// and to release the memory, ADD SSP, 4096, so 2 instructions
// which are anyway done at the start and end of the function
// call.)
//
class fast_buffer
{
public:
    fast_buffer(size_t size)
        : f_ptr(size > sizeof(f_fast_alloc) ? new char[size] : f_fast_alloc)
    {
    }

    ~fast_buffer()
    {
        if(f_ptr != f_fast_alloc)
        {
            delete [] f_ptr;
        }
    }

    char * get()
    {
        return f_ptr;
    }

private:
    char    f_fast_alloc[4096]; // default
    char *  f_ptr = nullptr;
};

void bufferDeleter(char * ptr)
{
    delete [] ptr;
}

}


/** \brief Creating a QCassandraProxy from the daemon.
 *
 * This constructor is used whenever we build the QCassandraProxy
 * object from the daemon side. In that case the daemon calls
 * the receiveOrder() and sendResult() with a socket as one of
 * the parameters.
 */
QCassandraProxy::QCassandraProxy()
{
}


QCassandraProxy::QCassandraProxy(QString const & host, int port)
    : f_host(host)
    , f_port(port)
{
}


QCassandraOrderResult QCassandraProxy::sendOrder(QCassandraOrder const & order)
{
    // Note: by default result is marked as "failed"
    //
    QCassandraOrderResult result;

    QByteArray const encoded(order.encodeOrder());

    // send the encoded buffer in one write
    //
    if(static_cast<int>(bio_write(encoded.data(), encoded.size())) != encoded.size())
    {
        return result;
    }

    // then we want to wait for the result
    //
    if(order.blocking())
    {
        // results are very similar to what we send: 4 bytes of
        // what we are receiving, a size, and the result buffer
        // of data encoded as per the QCassandraOrderResult scheme
        //
        unsigned char buf[8];
        if(bio_read(buf, 8) != 8) // 4 letters + 4 bytes for size
        {
            return result;
        }

        std::string command(reinterpret_cast<char const *>(buf), 4);

        uint32_t const reply_size(
                      (buf[4] << 24)
                    | (buf[5] << 16)
                    | (buf[6] <<  8)
                    | (buf[7] <<  0));

        fast_buffer reply(reply_size);
        if(reply_size > 0)
        {
            if(static_cast<uint32_t>(bio_read(reply.get(), reply_size)) != reply_size)
            {
                return result;
            }
        }

        if(result.decodeResult(reinterpret_cast<unsigned char const *>(reply.get()), reply_size))
        {
            // right now we expect either SUCS or EROR
            result.setSucceeded(command == "SUCS");
        }
    }
    else
    {
        result.setSucceeded(true);
    }

    return result;
}



/** \brief Read the next incoming order.
 *
 * This function is called by snapdbproxy to listen for further data
 * store orders to forward to Cassandra.
 *
 * The function blocks reading on the input \p reader. The result of
 * the function is exactly one order. snapdbproxy takes care of
 * the rest which is in generate to send the order to Cassandra,
 * wait for the answer, encode the answer and reply to client
 * with an encoded result (unless the order says it is "non-blocking"
 * in which case no reply are expected.)
 *
 * \todo
 * Look into whether we should instead use a poll() on all the
 * sockets, but right now we expect the snapdbproxy to use one
 * thread per socket...
 *
 * \param[in] reader  A QCassandraProxyIO implementation that can
 *                    read data from somewhere.
 *
 * \return The order just read, or an invalid order.
 */
QCassandraOrder QCassandraProxy::receiveOrder(QCassandraProxyIO & io)
{
    if(!f_host.isEmpty())
    {
        throw std::runtime_error("QCassandraProxy::receiveOrder() called from the client...");
    }

    // create an invalid order by default
    //
    QCassandraOrder order;
    order.setValidOrder(false);

    // each order starts with a 4 letter command
    //
    unsigned char buf[8];
    if(io.read(buf, 8) != 8)
    {
        return order;
    }

    uint32_t const order_size(
                  (buf[4] << 24)
                | (buf[5] << 16)
                | (buf[6] <<  8)
                | (buf[7] <<  0));

    std::string const command(reinterpret_cast<char const *>(buf), 4);
    if(command != "CQLP")
    {
        return order;
    }

    // now we want to read the order itself, so we need a buffer
    //
    fast_buffer order_data(order_size);
    if(static_cast<uint32_t>(io.read(order_data.get(), order_size)) != order_size)
    {
        return order;
    }

    if(!order.decodeOrder(reinterpret_cast<unsigned char const *>(order_data.get()), order_size))
    {
        return order;
    }

    // it worked, the order is valid
    //
    order.setValidOrder(true);

    return order;
}


/** \brief Send a result back to a client.
 *
 * This function is used to send the specified \p result back to
 * the client that sent an order ealier.
 *
 * While writing to a socket, if the client closes the socket, it is
 * likely that the write() function will return an invalid size. As
 * a result, this function returns false. On a false, you should end
 * your loop immediately.
 *
 * \param[in] io  An object capable of writing the result to a socket.
 * \param[in] result  The result to be sent to the client.
 *
 * \return true if the message is sent as expected, false otherwise.
 */
bool QCassandraProxy::sendResult(QCassandraProxyIO & io, QCassandraOrderResult const & result)
{
    if(!f_host.isEmpty())
    {
        throw std::runtime_error("QCassandraProxy::sendResult() called from the client...");
    }

    QByteArray const encoded(result.encodeResult());

    // now send the encoded buffer all at once
    //
    if(io.write(encoded.data(), encoded.size()) != encoded.size())
    {
        return false;
    }

    // result sent successfully
    //
    return true;
}


bool QCassandraProxy::isConnected() const
{
    return !!f_bio;
}


void QCassandraProxy::bio_get()
{
    if(!f_bio)
    {
        if(f_host.isEmpty())
        {
            throw std::runtime_error("QCassandraProxy::bio_get(): server cannot call bio_get()...");
        }

        bio_initialize();

        // create a plain BIO connection
        std::shared_ptr<BIO> bio(BIO_new(BIO_s_connect()), bio_deleter);
        if(!bio)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("QCassandraProxy::bio_get(): failed initializing a BIO object");
        }

        // it is supposed to be non-blocking by default, but just in case,
        // mark that one as such; although even that does not prevent the
        // BIO_read() and BIO_write() functions from returning early!
        //
        BIO_set_nbio(bio.get(), 0);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        BIO_set_conn_hostname(bio.get(), const_cast<char *>(f_host.toUtf8().data()));
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
        BIO_set_conn_int_port(bio.get(), const_cast<int *>(&f_port));
#pragma GCC diagnostic pop

        // connect to the server (open the socket)
        if(BIO_do_connect(bio.get()) <= 0)
        {
            ERR_print_errors_fp(stderr);
            throw std::runtime_error("QCassandraProxy::bio_get(): failed connecting BIO object to server");
        }

        // it worked, save the results
        f_bio = bio;
    }
}


void QCassandraProxy::bio_reset()
{
    f_bio.reset();
}


int QCassandraProxy::bio_read(void * buf, size_t size)
{
    if(size == 0)
    {
        return 0;
    }

    if(!f_bio)
    {
        bio_get();
    }

    // even though we open the BIO as blocking, somehow, we can get
    // the BIO_read() to return too soon... so here we loop until
    // all of the expected data got transferred
    //
    int count(0);
    int r(static_cast<int>(BIO_read(f_bio.get(), buf, size)));
    while(r >= -1)
    {
        if(r <= 0)
        {
            if(!BIO_should_retry(f_bio.get()))
            {
                break;
            }
        }
        else
        {
            count += r;
            size -= r;
            if(size == 0)
            {
                return count;
            }
            buf = reinterpret_cast<char *>(buf) + r;
        }
        r = static_cast<int>(BIO_read(f_bio.get(), buf, size));
    }

    // the BIO generated an error (TBD should we check BIO_eof() too?)
    // or the BIO is not implemented
    // XXX: do we have to set errno?
    ERR_print_errors_fp(stderr);
    return -1;
}


int QCassandraProxy::bio_write(void const * buf, size_t size)
{
    if(size == 0)
    {
        return 0;
    }

    if(!f_bio)
    {
        bio_get();
    }

    // even though we open the BIO as blocking, somehow, we can get
    // the BIO_read() to return too soon... so here we loop until
    // all of the expected data got transferred
    //
    int count(0);
    int r(static_cast<int>(BIO_write(f_bio.get(), buf, size)));
    while(r >= -1)
    {
        if(r == -1 || r == 0)
        {
            if(!BIO_should_retry(f_bio.get()))
            {
                break;
            }
        }
        else
        {
            count += r;
            size -= r;
            if(size == 0)
            {
                BIO_flush(f_bio.get());
                return count;
            }
            buf = reinterpret_cast<char const *>(buf) + r;
            r = static_cast<int>(BIO_write(f_bio.get(), buf, size));
        }
    }

    // the BIO generated an error (TBD should we check BIO_eof() too?)
    // or the BIO is not implemented
    // XXX: do we have to set errno?
    ERR_print_errors_fp(stderr);
    return -1;
}


} // namespace QtCassandra
// vim: ts=4 sw=4 et
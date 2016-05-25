/*
 * Text:
 *      snapbackup.h
 *
 * Description:
 * 	    Dumps and restores the "snap_websites" context to/from a SQLite database.
 *
 * License:
 *      Copyright (c) 2012-2016 Made to Order Software Corp.
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

// Snap suppot libs
//
#include <advgetopt/advgetopt.h>
#include <QtCassandra/QCassandraQuery.h>

// 3rd party libs
//
#include <QString>
#include <memory>

typedef std::shared_ptr<advgetopt::getopt>    getopt_ptr_t;

class snapbackup
{
public:
    snapbackup( getopt_ptr_t opt );

    void connectToCassandra();

    void dumpContext();
    void restoreContext();

private:
    QtCassandra::QCassandraSession::pointer_t f_session;
    getopt_ptr_t							  f_opt;

    void storeSchema( const QString& context_name );
    void dropContext( const QString& context_name );
    void storeTables( const int count, const QString& context_name );
    void restoreTables( const QString& context_name );
};

// vim: ts=4 sw=4 et

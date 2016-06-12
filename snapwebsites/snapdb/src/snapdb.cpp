/*
 * Text:
 *      snapdb.cpp
 *
 * Description:
 *      Reads and describes a Snap database. This ease checking out the
 *      current content of the database as the cassandra-cli tends to
 *      show everything in hexadecimal number which is quite unpractical.
 *      Now we do it that way for runtime speed which is much more important
 *      than readability by humans, but we still want to see the data in an
 *      easy practical way which this tool offers.
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

#include "snapdb.h"

// snapwebsites library
//
#include "dbutils.h"
#include "qstring_stream.h"
#include "snapwebsites.h"

// 3rd party libs
//
#include <QtCore>
#include <QtSql>
#include <QtCassandra/QCassandraSchema.h>
#include <controlled_vars/controlled_vars_need_init.h>
#include <advgetopt/advgetopt.h>

// system
//
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <sstream>

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdb_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>] [table [row] [cell] [value]]",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "context",
            nullptr,
            "name of the context from which to read",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "count",
            nullptr,
            "specify the number of rows to display",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            0,
            "create-row",
            nullptr,
            "allows the creation of a row when writing a value",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "drop-cell",
            nullptr,
            "drop the specified cell (specify row and cell)",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "drop-row",
            nullptr,
            "drop the specified row (specify row)",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "full-cell",
            nullptr,
            "show all the data from that cell, by default large binary cells get truncated for display",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "yes-i-know-what-im-doing",
            nullptr,
            "Force the dropping of tables, without warning and stdin prompt. Only use this if you know what you're doing!",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "host",
            nullptr,
            "host IP address or name (defaults to localhost)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "port",
            nullptr,
            "port on the host to connect to (defaults to 9042)",
            advgetopt::getopt::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "info",
            nullptr,
            "print out the cluster name and protocol version",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "no-types",
            nullptr,
            "supress the output of the column type",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of the snapdb executable",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "[table [row] [cell] [value]]",
            advgetopt::getopt::default_multiple_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::end_of_options
        }
    };
}
//namespace

using namespace QtCassandra;
using namespace QCassandraSchema;

snapdb::snapdb(int argc, char * argv[])
    : f_session( QCassandraSession::create() )
    , f_host("localhost") // default
    , f_port(9042) //default to connect to snapdbproxy
    , f_count(100)
    , f_context("snap_websites")
    //, f_table("") -- auto-init
    //, f_row("") -- auto-init
    , f_opt( new advgetopt::getopt( argc, argv, g_snapdb_options, g_configuration_files, nullptr ) )
{
    if(f_opt->is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
    }

    // first check options
    if( f_opt->is_defined( "count" ) )
    {
        f_count = f_opt->get_long( "count" );
    }
    if( f_opt->is_defined( "host" ) )
    {
        f_host = f_opt->get_string( "host" ).c_str();
    }
    if( f_opt->is_defined( "port" ) )
    {
        f_port = f_opt->get_long( "port" );
    }
    if( f_opt->is_defined( "context" ) )
    {
        f_context = f_opt->get_string( "context" ).c_str();
    }

    // then check commands
    if( f_opt->is_defined( "help" ) )
    {
        usage(advgetopt::getopt::no_error);
    }

    try
    {
        if( f_opt->is_defined( "info" ) )
        {
            info();
            exit(0);
        }
    }
    catch( const std::exception& except )
    {
        std::cerr << "Error connecting to the cassandra server! Reason=[" << except.what() << "]" << std::endl;
        exit( 1 );
    }
    catch( ... )
    {
        std::cerr << "Unknown error connecting to the cassandra server!" << std::endl;
        exit( 1 );
    }

    // finally check for parameters
    if( f_opt->is_defined( "--" ) )
    {
        const int arg_count = f_opt->size( "--" );
        if( arg_count > 4 )
        {
            std::cerr << "error: only four parameters (table, row, cell and value) can be specified on the command line." << std::endl;
            usage(advgetopt::getopt::error);
        }
        for( int idx = 0; idx < arg_count; ++idx )
        {
            if( idx == 0 )
            {
                f_table = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 1 )
            {
                f_row = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 2 )
            {
                f_cell = f_opt->get_string( "--", idx ).c_str();
            }
            else if( idx == 3 )
            {
                f_value = f_opt->get_string( "--", idx ).c_str();
            }
        }
    }
}

void snapdb::usage(advgetopt::getopt::status_t status)
{
    f_opt->usage( status, "snapdb" );
    exit(1);
}

void snapdb::info()
{
    try
    {
        f_session->connect(f_host, f_port);
        if(f_session->isConnected())
        {
            // read and display the Cassandra information
            auto q = QCassandraQuery::create( f_session );
            q->query( "SELECT cluster_name,native_protocol_version,partitioner FROM system.local" );
            q->start();
            std::cout << "Working on Cassandra Cluster Named \""    << q->getStringColumn("cluster_name")            << "\"." << std::endl;
            std::cout << "Working on Cassandra Protocol Version \"" << q->getStringColumn("native_protocol_version") << "\"." << std::endl;
            std::cout << "Using Cassandra Partitioner \""           << q->getStringColumn("partitioner")             << "\"." << std::endl;
            q->end();

            // At this time the following does not work, we will need CQL support first
            //const QCassandraClusterInformation& cluster_info(f_cassandra->clusterInformation());
            //int max(cluster_info.size());
            //std::cout << "With " << max << " nodes running." << std::endl;
            //for(int idx(0); idx < max; ++idx)
            //{
            //    const QCassandraNode& node(cluster_info.node(idx));
            //    std::cout << "H:" << node.nodeHost() << " R:" << node.nodeRack() << " DC:" << node.nodeDataCenter() << std::endl;
            //}
            exit(0);
        }
        else
        {
            std::cerr << "The connection failed!" << std::endl;
            exit(1);
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "The connection failed! what=" << ex.what() << std::endl;
        exit(1);
    }
}


void snapdb::drop_row() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("DELETE FROM %1.%2 WHERE key = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        int bind = 0;
        q->bindString( bind++, row_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove theme QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::drop_cell() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        QByteArray col_key;
        du.set_column_name( col_key, f_cell );
        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("DELETE FROM %1.%2 WHERE key = ? and column1 = ?;")
            .arg(f_context)
            .arg(f_table)
            );
        int bind = 0;
        q->bindByteArray( bind++, row_key );
        q->bindByteArray( bind++, col_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove theme QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


bool snapdb::row_exists() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        const QByteArray row_key( du.get_row_key() );
        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("SELECT column1 FROM %1.%2 WHERE key = ?")
            .arg(f_context)
            .arg(f_table)
            );
        int bind = 0;
        q->bindByteArray( bind++, row_key );
        q->start();
        return q->rowCount() > 0;
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Remove theme QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    return false;
}


void snapdb::display_tables() const
{
    try
    {
        SessionMeta::pointer_t sm( SessionMeta::create(f_session) );
        sm->loadSchema();
        const auto& keyspaces( sm->getKeyspaces() );
        auto snap_iter = keyspaces.find(f_context);
        if( snap_iter == keyspaces.end() )
        {
            throw std::runtime_error(
                    QString("Context '%1' does not exist! Aborting!")
                    .arg(f_context).toUtf8().data()
                    );
        }

        auto kys( snap_iter->second );
        for( auto table : kys->getTables() )
        {
            std::cout << table.first << std::endl;
        }
    }
    catch( const std::exception& ex )
    {
        std::cerr << "Exception caught! what=" << ex.what() << std::endl;
        exit(1);
    }
}


void snapdb::display_rows() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("SELECT DISTINCT key FROM %1.%2;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->setPagingSize(f_count);
        q->start();
        do
        {
            while( q->nextRow() )
            {
                std::cout << du.get_row_name( q->getByteArrayColumn(0) ) << std::endl;
            }
        }
        while( q->nextPage() );
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::display_rows_wildcard() const
{
    try
    {
        snap::dbutils du( f_table, f_row );
        QString const row_start(f_row.left(f_row.length() - 1));
        std::stringstream ss;

        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("SELECT DISTINCT key FROM %1.%2;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->setPagingSize(f_count);
        q->start();
        do
        {
            while( q->nextRow() )
            {
                const QString name(du.get_row_name(q->getByteArrayColumn(0)));
                if( name.length() >= row_start.length()
                    && row_start == name.mid(0, row_start.length()))
                {
                    ss << name << std::endl;
                }
            }
        }
        while( q->nextPage() );
        q->end();

        std::cout << ss.str();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::display_columns() const
{
    if( f_opt->is_defined("drop-row") )
    {
        drop_row();
        return;
    }

    try
    {
        snap::dbutils du( f_table, f_row );

        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("SELECT column1, value FROM %1.%2 WHERE key = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
        q->bindByteArray( 0, du.get_row_key() );
        q->setPagingSize(f_count);
        q->start();
        QStringList keys;
        QStringList types;
        QStringList values;
        do
        {
            while( q->nextRow() )
            {
                const auto column_key( q->getByteArrayColumn("column1") );
                const auto column_val( q->getByteArrayColumn("value") );
                keys   << du.get_column_name      ( column_key );
                types  << "[" + du.get_column_type_name ( column_key ) + "]";
                values << du.get_column_value     ( column_key, column_val, true /*display_only*/ );
            }
        }
        while( q->nextPage() );
        q->end();

        int max_key_len = 0;
        std::for_each( keys.begin(), keys.end(),
            [&max_key_len]( const QString& key )
            {
                max_key_len = std::max( key.size(), max_key_len );
            }
        );

        int max_value_len = 0;
        std::for_each( values.begin(), values.end(),
            [&max_value_len]( const QString& key )
            {
                max_value_len = std::max( key.size(), max_value_len );
            }
        );

        auto old_flags = std::cout.flags();
        std::cout.flags(std::ios::left);
        for( int idx = 0; idx < keys.size(); ++idx )
        {
            std::cout
                    << std::setw(max_key_len)   << keys[idx]   << " = "
                    << std::setw(max_value_len) << values[idx]
                    ;
            if( !f_opt->is_defined("no-types") )
            {
                std::cout << " " << types[idx];
            }
            std::cout << std::endl;
        }
        std::cout.flags(old_flags);
    }
    catch( std::exception const& e )
    {
        // in most cases we get here because of something invalid in
        // the database
        std::cerr   << "error: could not properly read row \""
                    << f_row
                    << "\" in table \""
                    << f_table
                    << "\". It may not exist or its key is not defined as expected (i.e. not a valid md5sum)"
                    << std::endl
                    << "what=" << e.what()
                    << std::endl;
    }
}


void snapdb::display_cell() const
{
    if(f_opt->is_defined("drop-cell"))
    {
        drop_cell();
    }
    else
    {
        snap::dbutils du( f_table, f_row );

        QByteArray value;
        try
        {
            const QByteArray row_key( du.get_row_key() );
            QByteArray col_key;
            du.set_column_name( col_key, f_cell );
            auto q( QCassandraQuery::create(f_session) );
            q->query( QString("SELECT value FROM %1.%2 WHERE key = ? and column1 = ?;")
                    .arg(f_context)
                    .arg(f_table)
                    );
            int num = 0;
            q->bindByteArray( num++, row_key );
            q->bindByteArray( num++, col_key );
            q->start();
            if( !q->nextRow() )
            {
                throw std::runtime_error( "Row/cell NOT FOUND!" );
            }
            value = q->getByteArrayColumn("value");
            q->end();
        }
        catch( const std::exception& ex )
        {
            std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
            exit(1);
            snap::NOTREACHED();
        }

        if(f_opt->is_defined("save-cell"))
        {
            std::fstream out;
            out.open(f_opt->get_string( "save-cell" ), std::fstream::out );
            if( !out.is_open() )
            {
                std::cerr << "error:display_cell(): could not open \"" << f_opt->get_string( "save-cell" )
                    << "\" to output content of cell \"" << f_cell
                    << "\" in table \"" << f_table
                    << "\" and row \"" << f_row
                    << "\"." << std::endl;
                exit(1);
                snap::NOTREACHED();
            }
            out.write( value.data(), value.size() );
        }
        else
        {
            std::cout << du.get_column_value
                ( f_cell.toUtf8(), value
                , !f_opt->is_defined("full-cell") /*display_only*/
                );
            if( !f_opt->is_defined("no-types") )
            {
                std::cout << " [" << du.get_column_type_name( f_cell.toUtf8() ) << "]";
            }
            std::cout << std::endl;
        }
    }
}


void snapdb::set_cell() const
{
    if( !f_opt->is_defined("create-row") )
    {
        if( !row_exists() )
        {
            std::cerr << "error:set_cell(): row \""
                      << f_row
                      << "\" not found in table \""
                      << f_table
                      << "\"."
                      << std::endl;
            exit(1);
            snap::NOTREACHED();
        }
    }

    try
    {
        snap::dbutils du( f_table, f_row );
        QByteArray const row_key( du.get_row_key() );
        QByteArray col_key;
        du.set_column_name( col_key, f_cell );
        QByteArray value;
        du.set_column_value( f_cell.toUtf8(), value, f_value );

        auto q( QCassandraQuery::create(f_session) );
        q->query( QString("UPDATE %1.%2 SET value = ? WHERE key = ? and column1 = ?;")
                .arg(f_context)
                .arg(f_table)
                );
        int bind_num = 0;
        q->bindByteArray( bind_num++, value   );
        q->bindByteArray( bind_num++, row_key );
        q->bindByteArray( bind_num++, col_key );
        q->start();
        q->end();
    }
    catch( const std::exception& ex )
    {
        std::cerr << "QCassandraQuery exception caught! what=" << ex.what() << std::endl;
        exit(1);
        snap::NOTREACHED();
    }
}


void snapdb::exec()
{
    f_session->connect( f_host, f_port );

    if(f_table.isEmpty())
    {
        display_tables();
    }
    else if(f_row.isEmpty())
    {
        display_rows();
    }
    else if(f_row.endsWith("%"))
    {
        display_rows_wildcard();
    }
    else if(f_cell.isEmpty())
    {
        display_columns();
    }
    else if(f_value.isEmpty())
    {
        display_cell();
    }
    else
    {
        set_cell();
    }
}

// vim: ts=4 sw=4 et

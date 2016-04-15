/*
 * Text:
 *      context_management.cpp
 *
 * Description:
 *      Create contexts, check that they exist, drop contexts, check that
 *      they were removed.
 *
 * Documentation:
 *      Run with no options, although supports the -h to define
 *      Cassandra's host.
 *      Fails if the test cannot find the expected contexts or can find
 *      the non-expected contexts.
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

#include <QtCassandra/QCassandra.h>
#include <QtCore/QDebug>

int main(int argc, char *argv[])
{
    QtCassandra::QCassandra::pointer_t cassandra( QtCassandra::QCassandra::create() );

    const char *host("localhost");
    for(int i(1); i < argc; ++i)
    {
        if(strcmp(argv[i], "--help") == 0)
        {
            qDebug() << "Usage:" << argv[0] << "[-h <hostname>]";
            exit(1);
        }
        if(strcmp(argv[i], "-h") == 0)
        {
            ++i;
            if(i >= argc)
            {
                qDebug() << "error: -h must be followed by a hostname.";
                exit(1);
            }
            host = argv[i];
        }
    }

    cassandra->connect(host);
    qDebug() << "Working on Cassandra Cluster Named" << cassandra->clusterName();

    QtCassandra::QCassandraContext::pointer_t context(cassandra->context("qt_cassandra_test_context"));
    //
    QtCassandra::QCassandraSchema::Value replication;
    auto& replication_map(replication.map());
    replication_map["class"]              = QtCassandra::QCassandraSchema::Value("SimpleStrategy");
    replication_map["replication_factor"] = QtCassandra::QCassandraSchema::Value(1);
    //
    auto& fields(context->fields());
    fields["replication"]    = replication;
    fields["durable_writes"] = QVariant(true);
    //
    try
    {
        context->drop();
        cassandra->synchronizeSchemaVersions();
    }
    catch(...)
    {
        // ignore errors, this happens when the context doesn't exist yet
    }

    QtCassandra::QCassandraTable::pointer_t table(context->table("qt_cassandra_test_table"));
    auto& table_fields( table->fields() );
    table_fields["comment"]                     = QVariant("Our test table.");
    table_fields["memtable_flush_period_in_ms"] = QVariant(60);
    table_fields["gc_grace_seconds"]            = QVariant(86400);
    //
    auto& compaction_value_map(table_fields["compaction"].map());
    compaction_value_map["class"]         = QVariant("SizeTieredCompactionStrategy");
    compaction_value_map["min_threshold"] = QVariant(4);
    compaction_value_map["max_threshold"] = QVariant(22);

    try
    {
        context->create();
        cassandra->synchronizeSchemaVersions();
        qDebug() << "Done!";
    }
    catch( const std::exception& e )
    {
        qDebug() << "Exception is [" << e.what() << "]";
    }

    context->drop();
    cassandra->synchronizeSchemaVersions();

    return 0;
}

// vim: ts=4 sw=4 et

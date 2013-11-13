/*
 * Text:
 *      cassandra_lock.cpp
 *
 * Description:
 *      Test the QCassandraLock object to make sure that the lock works
 *      as expected when running this test on any number of computers.
 *
 * Documentation:
 *      Start the first instance with the -h option to define the
 *      Cassandra's host (defaults to 127.0.0.1 if undefined) and
 *      the -i to tell the first instance how many instances you
 *      want to run simultaneously. For example, if you have 4
 *      processors, you may want to use -i 4 or -i 8. It also
 *      accepts the number of times the processes will attempt the
 *      lock with -n. By default that count is 60 (1 minute). You
 *      can set it to 86400 for about 1 day test and a multiple
 *      thereof to run the test for multiple days.
 *
 * License:
 *      Copyright (c) 2013 Made to Order Software Corp.
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
#include <QtCassandra/QCassandraLock.h>
#include <QtCassandra/QCassandraValue.h>
#include <QtCore/QDebug>
#include <unistd.h>

int main(int argc, char *argv[])
{
    QtCassandra::QCassandra     cassandra;
    qDebug() << "+ libQtCassandra version" << cassandra.version();

    int process_count(0);
    int repeat(0);
    int replication_factor(0);
    int mode(0);
    int check_result(0);
    const char *host("localhost");
    const char *computer_name(NULL);
    QtCassandra::consistency_level_t consistency_level(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    for(int i(1); i < argc; ++i) {
        if(strcmp(argv[i], "--help") == 0) {
            qDebug() << "Usage:" << argv[0] << "[--help] [-a | -r | -o <computer-name>] [-h <hostname>] [-i <count>] [-n <repeat>] [-c <replication-factor>] [-V] [-t] [-l <consistency level>]";
            qDebug() << "  where -h indicates the Cassandra IP address";
            qDebug() << "  where -i indicates the number of process to spawn total";
            qDebug() << "  where -n indicates the number of time each process will create a unique row";
            qDebug() << "  where -o indicates the name of this computer";
            qDebug() << "  where -a indicates the name of a computer to add to the database";
            qDebug() << "  where -r indicates the name of a computer to remove from the database";
            qDebug() << "  where -c indicates that the call is used to create the context with the specified replication factor; ignore -i and -n";
            qDebug() << "  where -V indicates you want to verify the database after a run";
            qDebug() << "  where -t indicates you want to truncate the test table (usually before a new test)";
            qDebug() << "  where -l indicates the consistency level (one, quorum [default], local-quorum, each-quorum, all, two, three)";
            exit(1);
        }
        if(strcmp(argv[i], "-h") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -h must be followed by a hostname.";
                exit(1);
            }
            host = argv[i];
        }
        else if(strcmp(argv[i], "-i") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -i must be followed by a number.";
                exit(1);
            }
            process_count = atol(argv[i]);
        }
        else if(strcmp(argv[i], "-n") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -n must be followed by a number.";
                exit(1);
            }
            repeat = atol(argv[i]);
        }
        else if(strcmp(argv[i], "-o") == 0 || strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "-r") == 0) {
            mode = argv[i][1] == 'a' ? 1 : (argv[i][1] == 'r' ? 2 : 0);
            ++i;
            if(i >= argc) {
                qDebug() << "error: -o, -a, and -r must be followed by a computer name.";
                exit(1);
            }
            computer_name = argv[i];
        }
        else if(strcmp(argv[i], "-c") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -c must be followed by a number.";
                exit(1);
            }
            replication_factor = atol(argv[i]);
            if(replication_factor <= 0) {
                qDebug() << "error: replication factor (-c) must be positive.";
                exit(1);
            }
        }
        else if(strcmp(argv[i], "-V") == 0) {
            check_result = 1;
        }
        else if(strcmp(argv[i], "-t") == 0) {
            check_result = 2;
        }
        else if(strcmp(argv[i], "-l") == 0) {
            ++i;
            if(i >= argc) {
                qDebug() << "error: -l must be followed by a consistency level.";
                exit(1);
            }
            if(strcmp(argv[i], "one") == 0 || strcmp(argv[i], "1") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_ONE;
            }
            else if(strcmp(argv[i], "quorum") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_QUORUM;
            }
            else if(strcmp(argv[i], "local-quorum") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_LOCAL_QUORUM;
            }
            else if(strcmp(argv[i], "each-quorum") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_EACH_QUORUM;
            }
            else if(strcmp(argv[i], "all") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_ALL;
            }
            else if(strcmp(argv[i], "two") == 0 || strcmp(argv[i], "2") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_TWO;
            }
            else if(strcmp(argv[i], "three") == 0 || strcmp(argv[i], "3") == 0) {
                consistency_level = QtCassandra::CONSISTENCY_LEVEL_THREE;
            }
            else {
                qDebug() << "error: " << argv[i] << " is not a valid consistency level.";
                exit(1);
            }
        }
    }

    if(replication_factor > 0) {
        // each child must have a separate connection, so we have a specific
        // connection for the context handling
        cassandra.connect(host);
        QString name = cassandra.clusterName();
        qDebug() << "+ Cassandra Cluster Name is" << name;
        qDebug() << "+ Creating context with replication factor set to" << replication_factor;

        QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_lock"));
        try {
            context->drop();
            cassandra.synchronizeSchemaVersions();
        }
        catch(...) {
            // ignore error, the context probably doesn't exist yet
        }
        context->setStrategyClass("SimpleStrategy"); // default is LocalStrategy
        //context->setDurableWrites(false); // by default this is 'true'
        context->setReplicationFactor(replication_factor); // by default this is undefined

        QSharedPointer<QtCassandra::QCassandraTable> table(context->table("qt_cassandra_test_table"));
        //table->setComment("Our test table.");
        table->setColumnType("Standard"); // Standard or Super
        table->setKeyValidationClass("BytesType");
        table->setDefaultValidationClass("BytesType");
        table->setComparatorType("BytesType");
        table->setKeyCacheSavePeriodInSeconds(14400);
        table->setMemtableFlushAfterMins(60);
        //table->setMemtableThroughputInMb(247);
        //table->setMemtableOperationsInMillions(1.1578125);
        //table->setGcGraceSeconds(864000); // 10 days (default)
        table->setGcGraceSeconds(3600); // 1h.
        table->setMinCompactionThreshold(4);
        table->setMaxCompactionThreshold(22);
        table->setReplicateOnWrite(1);

        try {
            context->create();
        }
        catch(...) {
            qDebug() << "error: could not create the context, an exception occured.";
            throw;
        }
        // attempt a synchronization so when we quit we can immediately use the context
        cassandra.synchronizeSchemaVersions();
        exit(0);
    }

    if(check_result > 0) {
        if(check_result == 1) {
            // check the whole database for unique entries
            cassandra.connect(host);
            QString name = cassandra.clusterName();
            qDebug() << "+ Cassandra Cluster Name is" << name;
            qDebug() << "+ Verifying test table" << replication_factor;

            QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_lock"));
            if(!context) {
                qDebug() << "warning: could not find the context, did you run the test yet?";
                exit(1);
            }
            QSharedPointer<QtCassandra::QCassandraTable> table(context->table("qt_cassandra_test_table"));
            QSharedPointer<QtCassandra::QCassandraColumnRangePredicate> col_predicate(new QtCassandra::QCassandraColumnRangePredicate);
            col_predicate->setStartColumnName("unique");
            col_predicate->setEndColumnName("uo");
            QtCassandra::QCassandraRowPredicate row_predicate;
            row_predicate.setColumnPredicate(col_predicate);
            int row_count(0);
            int err(0);
            for(;;) {
                table->clearCache();
                uint32_t max(table->readRows(row_predicate));
                if(max == 0) {
                    // we expect to exit here on success
                    break;
                }
                const QtCassandra::QCassandraRows& r(table->rows());
                for(QtCassandra::QCassandraRows::const_iterator o(r.begin()); o != r.end(); ++o) {
                    QtCassandra::QCassandraColumnRangePredicate inner_col_predicate;
                    inner_col_predicate.setStartColumnName("unique");
                    inner_col_predicate.setEndColumnName("uo");
                    // TODO XXX What?!
                    // Having a predicate only here does not work, it needs
                    // to be on the readRows()!!!
                    (*o)->readCells(inner_col_predicate);
                    const QtCassandra::QCassandraCells& c(o.value()->cells());
                    if(c.size() > 1) {
                        QByteArray key((*o)->rowKey());
                        int64_t v(QtCassandra::int64Value(key));
                        qDebug() << "error: row" << v << "has" << c.size() << "'unique' columns.";
                        ++err;
                        for(QtCassandra::QCassandraCells::const_iterator p(c.begin()); p != c.end(); ++p) {
                            qDebug() << "error: cell" << (*p)->columnName();
                        }
                    }
                    ++row_count;
                }
            }
            qDebug() << "info: found" << row_count << "rows.";
            if(err > 0) {
                qDebug() << "warning: " << err << " errors occured.";
            }
        }
        else if(check_result == 2) {
            // truncate the table so we can start a new clean test
            // without having to delete everything
            cassandra.connect(host);
            QString name = cassandra.clusterName();
            qDebug() << "+ Cassandra Cluster Name is" << name;
            qDebug() << "+ Truncating the test table";

            QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_lock"));
            if(!context) {
                qDebug() << "error: could not retreive the qt_cassandra_test_lock context, did you run once with -c?";
                exit(1);
            }
            QSharedPointer<QtCassandra::QCassandraTable> table(context->table("qt_cassandra_test_table"));
            table->truncate();
        }
        exit(0);
    }

    if(mode != 0) {
        if(computer_name == NULL) {
            qDebug() << "error: -o is required to add or remove the host name from the cluster";
            exit(1);
        }

        cassandra.connect(host);
        QString name = cassandra.clusterName();
        qDebug() << "+ Cassandra Cluster Name is" << name;
        qDebug() << "+" << (mode == 1 ? "Adding" : "Removing") << computer_name << "to the lock table";

        QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_lock"));
        if(!context) {
            qDebug() << "error: could not retrive the qt_cassandra_test_lock context, did you run once with -c?";
            exit(1);
        }
        if(mode == 1) {
            context->addLockHost(computer_name);
            printf("%s added to the database.\n", computer_name);
        }
        else {
            context->removeLockHost(computer_name);
            printf("%s removed from the database.\n", computer_name);
        }
        exit(0);
    }

    if(process_count < 1) {
        qDebug() << "error: -i must be followed by a valid decimal number larger than 0";
        exit(1);
    }
    if(process_count > 100) {
        qDebug() << "error: -i must be followed by a valid decimal number up to 100";
        exit(1);
    }

    if(repeat < 1) {
        qDebug() << "error: -n must be followed by a valid decimal number larger than 0";
        exit(1);
    }

    qDebug() << "+ Starting test with" << process_count << "processes and repeat the lock" << repeat << "times";

    for(int i(1); i < process_count; ++i) {
        if(fork() == 0) {
            // the children don't create other processes
            break;
        }
    }

    // the child connects to Cassandra
    cassandra.connect(host);
    QString name = cassandra.clusterName();
    qDebug() << "+ Cassandra Cluster Name is" << name << "for child" << getpid();
    QSharedPointer<QtCassandra::QCassandraContext> context(cassandra.context("qt_cassandra_test_lock"));
    if(!context) {
        qDebug() << "error: could not retrive the qt_cassandra_test_lock context, did you run once with -c?";
        exit(1);
    }
    if(computer_name != NULL) {
        context->setHostName(computer_name);
    }

    QSharedPointer<QtCassandra::QCassandraTable> table(context->table("qt_cassandra_test_table"));
    if(!table) {
        qDebug() << "error: could not retrive the qt_cassandra_test_table, did you run once with -c?";
        exit(1);
    }

    try {
        for(int i(0); i < repeat; ++i) {
            sleep(1);
            // define a common key
            time_t now(time(NULL));
            QByteArray key;
            QtCassandra::appendUInt64Value(key, now);
            // acquire the lock; if it fails it will throw
            QtCassandra::QCassandraLock lock(context, key, consistency_level);
            QSharedPointer<QtCassandra::QCassandraCell> cell(table->row(key)->cell("winner"));
            cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
            QtCassandra::QCassandraValue winner(cell->value());
            if(winner.nullValue()) {
                // we're the first to lock that row!
                QtCassandra::QCassandraValue win(getpid());
                win.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
                table->row(key)->cell("winner")->setValue(win);
                QtCassandra::QCassandraValue unique(true);
                // unique with a consistency of ONE would also work, but in a real
                // world situation you probably would want to use QUORUM anyway
                unique.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
                table->row(key)->cell(QString("unique%1").arg(getpid()))->setValue(unique);
            }
            else {
                // if we're not the winner still show that we were working on that row
                QtCassandra::QCassandraValue loser(true);
                loser.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
                table->row(key)->cell(QString("loser%1").arg(getpid()))->setValue(loser);
            }
        }
    }
    catch(const std::exception& e) {
        fprintf(stderr, "!!! exception [%d]: %s !!!\n", getpid(), e.what());
    }
}

// vim: ts=4 sw=4 et

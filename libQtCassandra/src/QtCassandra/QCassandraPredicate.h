/*
 * Header:
 *      QCassandraPredicate.h
 *
 * Description:
 *      Handling of the cassandra::SlicePredicate to retrieve a set of columns
 *      all at once.
 *
 * Documentation:
 *      See the corresponding .cpp file.
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

#include "QtCassandra/QCassandraConsistencyLevel.h"
#include "QtCassandra/QCassandraQuery.h"

#include <controlled_vars/controlled_vars_auto_enum_init.h>
#include <controlled_vars/controlled_vars_limited_auto_init.h>

#include <QByteArray>
#include <QRegExp>
#include <QString>

#include <memory>

namespace QtCassandra
{

typedef controlled_vars::limited_auto_init<int32_t, 1, INT_MAX, 100> cassandra_count_t;

class QCassandraPredicate
{
public:
    typedef std::shared_ptr<QCassandraPredicate> pointer_t;

    QCassandraPredicate() : f_count(100), f_consistencyLevel(CONSISTENCY_LEVEL_DEFAULT) {}
    virtual ~QCassandraPredicate() {}

    int32_t count() const                       { return f_count; }
    void    setCount( const int32_t val = 100 ) { f_count = val;  }

    consistency_level_t	consistencyLevel() const							{ return f_consistencyLevel;  }
    void                setConsistencyLevel( consistency_level_t level )	{ f_consistencyLevel = level; }

protected:
    cassandra_count_t   f_count;
    consistency_level_t	f_consistencyLevel;

    virtual void appendQuery( QString& query, int& bind_count ) = 0;
    virtual void bindQuery( QCassandraQuery::pointer_t query, int& bind_num ) = 0;
};


class QCassandraCellPredicate : public QCassandraPredicate
{
public:
    typedef std::shared_ptr<QCassandraCellPredicate> pointer_t;

    // The name predicates can have any character from \0 to \uFFFD
    // (although in full Unicode, you may want to use \U10FFFD but at this
    // point I limit the code to \uFFFD because QChar uses a ushort)
    //
    // Note: Qt strings use UTF-16, but in a QChar, I'm not too sure we
    //       can put a value more than 0xFFFF... so we'd need the last_char
    //       to be a QString to support the max. character in Unicode!
    static const QChar first_char;
    static const QChar last_char;

    QCassandraCellPredicate() {}
    virtual ~QCassandraCellPredicate() {}

protected:
    friend class QCassandraRowPredicate;
    friend class QCassandraRowKeyPredicate;
    friend class QCassandraRowRangePredicate;

    virtual void appendQuery( QString& /*query*/, int& /*bind_count*/             ) {}
    virtual void bindQuery( QCassandraQuery::pointer_t /*query*/, int& /*bind_num*/ ) {}
};


class QCassandraCellKeyPredicate : public QCassandraCellPredicate
{
public:
    typedef std::shared_ptr<QCassandraCellKeyPredicate> pointer_t;

    QCassandraCellKeyPredicate() {}

    const QByteArray& cellKey() const                        { return f_cellKey; }
    void              setCellKey(const QByteArray& cell_key) { f_cellKey = cell_key; }

protected:
    QByteArray  f_cellKey;

    virtual void appendQuery( QString& query, int& bind_count );
    virtual void bindQuery( QCassandraQuery::pointer_t query, int& bind_num );
};


class QCassandraCellRangePredicate : public QCassandraCellPredicate
{
public:
    typedef std::shared_ptr<QCassandraCellRangePredicate> pointer_t;

    QCassandraCellRangePredicate() {}

    const QByteArray& startCellKey() const                        { return f_startCellKey;     }
    void              setStartCellKey(const char* cell_key)       { setStartCellKey(QByteArray(cell_key,qstrlen(cell_key))); }
    void              setStartCellKey(const QString& cell_key)    { setStartCellKey(cell_key.toUtf8()); }
    void              setStartCellKey(const QByteArray& cell_key) { f_startCellKey = cell_key; }

    const QByteArray& endCellKey() const                          { return f_endCellKey;       }
    void              setEndCellKey(const char* cell_key)         { setEndCellKey(QByteArray(cell_key,qstrlen(cell_key))); }
    void              setEndCellKey(const QString& cell_key)      { setEndCellKey(cell_key.toUtf8()); }
    void              setEndCellKey(const QByteArray& cell_key)   { f_endCellKey = cell_key;   }

    bool reversed() const                                         { return f_reversed; }
    void setReversed( bool val = true )                           { f_reversed = val;  }

    bool index() const                                            { return f_index;    }
    void setIndex( bool val = true )                              { f_index = val;     }

protected:
    QByteArray                  f_startCellKey;
    QByteArray                  f_endCellKey;
    controlled_vars::fbool_t    f_reversed;
    controlled_vars::fbool_t    f_index; // whether predicate is used as an index

    virtual void appendQuery( QString& query, int& bind_count );
    virtual void bindQuery( QCassandraQuery::pointer_t query, int& bind_num );
};


class QCassandraRowPredicate : public QCassandraPredicate
{
public:
    typedef std::shared_ptr<QCassandraRowPredicate> pointer_t;

    QCassandraRowPredicate() : f_cellPred( new QCassandraCellPredicate ) {}
    virtual ~QCassandraRowPredicate() {}

    QRegExp rowNameMatch() const { return f_row_name_match; }
    void    setRowNameMatch(QRegExp const& re) { f_row_name_match = re; }

    QCassandraCellPredicate::pointer_t  cellPredicate() const { return f_cellPred; }
    void                                setCellPredicate( QCassandraCellPredicate::pointer_t pred ) { f_cellPred = pred; }

    virtual void appendQuery( QString& /*query*/, int& /*bind_count*/             ) {}
    virtual void bindQuery( QCassandraQuery::pointer_t /*query*/, int& /*bind_num*/ ) {}

protected:
    QCassandraCellPredicate::pointer_t      f_cellPred;
    QRegExp                                 f_row_name_match;
};


class QCassandraRowKeyPredicate : public QCassandraRowPredicate
{
public:
    typedef std::shared_ptr<QCassandraRowKeyPredicate> pointer_t;

    QCassandraRowKeyPredicate() {}

    const QByteArray& rowKey() const                       { return f_rowKey;   }
    void              setRowKey(const QByteArray& row_key) { f_rowKey= row_key; }

    virtual void appendQuery( QString& query, int& bind_count );
    virtual void bindQuery( QCassandraQuery::pointer_t query, int& bind_num );

protected:
    QByteArray  f_rowKey;
};


class QCassandraRowRangePredicate : public QCassandraRowPredicate
{
public:
    typedef std::shared_ptr<QCassandraRowRangePredicate> pointer_t;

    QCassandraRowRangePredicate() {}

    const QByteArray& startRowKey() const                       { return f_startRowKey;    }
    void              setStartRowKey(const QByteArray& row_key) { f_startRowKey = row_key; }

    const QByteArray& endRowKey() const                         { return f_endRowKey;      }
    void              setEndRowKey(const QByteArray& row_key)   { f_endRowKey = row_key;   }

    virtual void appendQuery( QString& query, int& bind_count );
    virtual void bindQuery( QCassandraQuery::pointer_t query, int& bind_num );

protected:
    QByteArray  f_startRowKey;
    QByteArray  f_endRowKey;
};


} // namespace QtCassandra

// vim: ts=4 sw=4 et

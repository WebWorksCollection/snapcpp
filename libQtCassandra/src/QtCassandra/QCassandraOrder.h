/*
 * Header:
 *      QCassandraOrder.h
 *
 * Description:
 *      Manager an order to be sent to the snapdbproxy daemon.
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

//#include <controlled_vars/controlled_vars_auto_init.h>
//#include <controlled_vars/controlled_vars_limited_auto_init.h>
//#include <controlled_vars/controlled_vars_limited_auto_enum_init.h>
//#include <QString>
//#include <QByteArray>
//#include <memory>
//#include <stdint.h>


namespace QtCassandra
{

class QCassandraOrder
{
public:
    enum type_of_result_t
    {
        TYPE_OF_RESULT_CLOSE,       // i.e. close a cursor
        TYPE_OF_RESULT_DECLARE,     // i.e. create a cursor (SELECT)
        TYPE_OF_RESULT_DESCRIBE,    // i.e. just whether it worked or not
        TYPE_OF_RESULT_FETCH,       // i.e. read next page from cursor (nextPage)
        TYPE_OF_RESULT_ROWS,        // i.e. one SELECT
        TYPE_OF_RESULT_SUCCESS      // i.e. just whether it worked or not
    };

    type_of_result_t    get_type_of_result() const;
    QString             cql() const;
    void                setCql(QString const & cql_string, type_of_result_t const result_type = type_of_result_t::TYPE_OF_RESULT_SUCCESS);

    bool                validOrder() const;
    void                setValidOrder(bool const valid);

    cassandra_consistency_level_t consistencyLevel() const;
    void                setConsistencyLevel(cassandra_consistency_level_t consistency_level);

    int64_t             timestamp() const;
    void                setTimestamp(int64_t const user_timestamp);

    int32_t             timeout() const;
    void                setTimeout(int32_t const statement_timeout_ms);

    int8_t              columnCount() const;
    void                setColumnCount(int8_t const paging_size);

    int32_t             pagingSize() const;
    void                setPagingSize(int32_t const paging_size);

    int32_t             cursorIndex() const;
    void                setCursorIndex(int32_t const cursor_index);

    bool                blocking() const;
    void                setBlocking(bool const block = true);

    size_t              parameterCount() const;
    QByteArray const &  parameter(int index) const;
    void                addParameter(QByteArray const & data);

    QByteArray          encodeOrder() const;
    bool                decodeOrder(unsigned char const * encoded_order, size_t size);

private:
    QString                         f_cql;
    bool                            f_valid = true;
    bool                            f_blocking = true;
    type_of_result_t                f_type_of_result = type_of_result_t::TYPE_OF_RESULT_SUCCESS;
    cassandra_consistency_level_t   f_consistency_level = CONSISTENCY_LEVEL_ONE; // TBD: can we get the QCassandra default automatically?
    int64_t                         f_timestamp = 0;
    int32_t                         f_timeout_ms = 0;
    int8_t                          f_column_count = 1;
    int32_t                         f_paging_size = 0;
    int32_t                         f_cursor_index = -1;
    std::vector<QByteArray>         f_parameter;
};


} // namespace QtCassandra
// vim: ts=4 sw=4 et

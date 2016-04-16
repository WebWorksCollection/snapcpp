/*
 * Text:
 *      QCassandraSchemaValue.h
 *
 * Description:
 *      Handling of the CQL interface.
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

#include "QtCassandra/QCassandraQuery.h"

#include <map>
#include <memory>
#include <vector>

#include <QString>
#include <QVariant>


namespace QtCassandra
{


namespace QCassandraSchema
{


class Value
{
public:
    typedef enum { TypeUnknown, TypeVariant, TypeMap, TypeList } type_t;
    //typedef std::shared_ptr<Value>            pointer_t;
    typedef std::vector<Value>            list_t;
    typedef std::map<QString,Value>       map_t;

    Value();
    Value( const QVariant& var );

    //static pointer_t create();

    void    readValue( CassTools::iterator_pointer_t iter );
    void    readValue( CassTools::value_pointer_t iter );
    type_t  type() const { return f_type; }

    const QVariant&     variant()    const { return f_variant;   }
    QVariant&     		variant()          { f_type = TypeVariant; return f_variant;   }
    const list_t&       list()       const { return f_list;      }
    list_t&       		list()             { f_type = TypeList; return f_list;      }
    const map_t&        map()        const { return f_map;       }
    map_t&              map()              { f_type = TypeMap; return f_map;       }

    const QString& output() const;

private:
    CassTools::value_pointer_t f_value;
    type_t                     f_type;
    QVariant                   f_variant;
    list_t                     f_list;
    map_t                      f_map;

    mutable QString f_stringOutput;

    void    parseValue();
    void    parseMap();
    void    parseList();
    void    parseTuple();
    void    parseVariant();
};


}
// namespace QCassandraSchema


}
//namespace QtCassandra

// vim: ts=4 sw=4 et

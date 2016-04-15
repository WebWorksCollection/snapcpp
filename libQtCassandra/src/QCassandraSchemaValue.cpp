/*
 * Text:
 *      QCassandraSchemaValue.cpp
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

#include "QtCassandra/QCassandraQuery.h"
#include "QtCassandra/QCassandraSchemaValue.h"
#include "CassTools.h"

namespace QtCassandra
{

using namespace CassTools;

namespace QCassandraSchema
{


Value::Value()
    : f_type(TypeUnknown)
    , f_cassType(CASS_VALUE_TYPE_UNKNOWN)
{
}


Value::Value( const QVariant& var )
    : f_type(TypeUnknown)
    , f_cassType(CASS_VALUE_TYPE_UNKNOWN)
    , f_variant(var)
{
}


#if 0
Value::pointer_t Value::create()
{
    return std::make_shared<Value>();
}
#endif


void Value::readValue( iterator_pointer_t iter )
{
    value_pointer_t value
        ( cass_iterator_get_meta_field_value(iter.get())
        , valueDeleter()
        );
    readValue( value );
}


void Value::readValue( value_pointer_t val )
{
    f_value = val;
    f_cassType = cass_value_type( f_value.get() );

    parseValue();
}


void Value::parseValue()
{
    f_map.clear();
    f_list.clear();
    f_variant.clear();
    f_stringOutput.clear();

    switch( f_cassType )
    {
        case CASS_VALUE_TYPE_UNKNOWN    :
        case CASS_VALUE_TYPE_CUSTOM     :
        case CASS_VALUE_TYPE_DECIMAL    :
        case CASS_VALUE_TYPE_LAST_ENTRY :
        case CASS_VALUE_TYPE_UDT       :
            f_type = TypeUnknown;
            break;

        case CASS_VALUE_TYPE_LIST      :
        case CASS_VALUE_TYPE_SET       :
            f_type = TypeList;
            parseList();
            break;

        case CASS_VALUE_TYPE_TUPLE     :
            f_type = TypeList;
            parseTuple();
            break;

        case CASS_VALUE_TYPE_MAP       :
            f_type = TypeMap;
            parseMap();
            break;

        case CASS_VALUE_TYPE_BLOB       :
        case CASS_VALUE_TYPE_BOOLEAN    :
        case CASS_VALUE_TYPE_FLOAT      :
        case CASS_VALUE_TYPE_DOUBLE     :
        case CASS_VALUE_TYPE_TINY_INT  :
        case CASS_VALUE_TYPE_SMALL_INT :
        case CASS_VALUE_TYPE_INT       :
        case CASS_VALUE_TYPE_VARINT    :
        case CASS_VALUE_TYPE_BIGINT     :
        case CASS_VALUE_TYPE_COUNTER    :
        case CASS_VALUE_TYPE_ASCII     :
        case CASS_VALUE_TYPE_DATE      :
        case CASS_VALUE_TYPE_TEXT      :
        case CASS_VALUE_TYPE_TIME      :
        case CASS_VALUE_TYPE_TIMESTAMP :
        case CASS_VALUE_TYPE_VARCHAR   :
        case CASS_VALUE_TYPE_UUID      :
        case CASS_VALUE_TYPE_TIMEUUID  :
        case CASS_VALUE_TYPE_INET      :
            f_type = TypeVariant;
            parseVariant();
            break;
    }
}


void Value::parseMap()
{
    iterator_pointer_t iter
        ( cass_iterator_from_map(f_value.get())
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        value_pointer_t key
            ( cass_iterator_get_map_key(iter.get())
              , valueDeleter()
            );
        const char* str;
        size_t len = 0;
        CassError rc = cass_value_get_string( key.get(), &str, &len );
        if( rc != CASS_OK )
        {
            throw std::runtime_error( "Can't extract the map key!" );
        }
        const QString key_str( QString::fromUtf8( str, len ) );

        value_pointer_t value
            ( cass_iterator_get_map_value(iter.get())
            , valueDeleter()
            );

        Value val;
        val.readValue( value );
        //
        f_map[key_str] = val;
    }
}


void Value::parseList()
{
    iterator_pointer_t iter
        ( cass_iterator_from_collection(f_value.get())
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        value_pointer_t p_val
                ( cass_iterator_get_value(iter.get())
                , valueDeleter()
                );
        Value val;
        val.readValue( p_val );
        f_list.push_back( val );
    }
}


void Value::parseTuple()
{
    iterator_pointer_t iter
        ( cass_iterator_from_tuple(f_value.get())
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        Value val;
        val.readValue( iter );
        f_list.push_back( val );
    }
}


void Value::parseVariant()
{
    CassError rc = CASS_OK;
    switch( f_cassType )
    {
        case CASS_VALUE_TYPE_BLOB       :
            {
                const cass_byte_t* buff;
                size_t len = 0;
                rc = cass_value_get_bytes( f_value.get(), &buff, &len );
                f_variant = QByteArray::fromRawData( reinterpret_cast<const char*>(buff), len );
            }
            break;

        case CASS_VALUE_TYPE_BOOLEAN    :
            {
                cass_bool_t b;
                rc = cass_value_get_bool( f_value.get(), &b );
                f_variant = (b == cass_true);
            }
            break;

        case CASS_VALUE_TYPE_FLOAT      :
            {
                cass_float_t f;
                rc = cass_value_get_float( f_value.get(), &f );
                f_variant = static_cast<float>(f);
            }
            break;

        case CASS_VALUE_TYPE_DOUBLE     :
            {
                cass_double_t d;
                rc = cass_value_get_double( f_value.get(), &d );
                f_variant = static_cast<double>(d);
            }
            break;

        case CASS_VALUE_TYPE_TINY_INT  :
            {
                cass_int8_t i;
                rc = cass_value_get_int8( f_value.get(), &i );
                f_variant = static_cast<int8_t>(i);
            }
            break;

        case CASS_VALUE_TYPE_SMALL_INT :
            {
                cass_int16_t i;
                rc = cass_value_get_int16( f_value.get(), &i );
                f_variant = static_cast<int16_t>(i);
            }
            break;

        case CASS_VALUE_TYPE_INT       :
        case CASS_VALUE_TYPE_VARINT    :
            {
                cass_int32_t i;
                rc = cass_value_get_int32( f_value.get(), &i );
                f_variant = static_cast<int32_t>(i);
            }
            break;

        case CASS_VALUE_TYPE_BIGINT     :
        case CASS_VALUE_TYPE_COUNTER    :
            {
                cass_int64_t i;
                rc = cass_value_get_int64( f_value.get(), &i );
                f_variant = static_cast<qlonglong>(i);
            }
            break;

        case CASS_VALUE_TYPE_ASCII     :
        case CASS_VALUE_TYPE_DATE      :
        case CASS_VALUE_TYPE_TEXT      :
        case CASS_VALUE_TYPE_TIME      :
        case CASS_VALUE_TYPE_TIMESTAMP :
        case CASS_VALUE_TYPE_VARCHAR   :
            {
                const char* str;
                size_t len = 0;
                rc = cass_value_get_string( f_value.get(), &str, &len );
                f_variant = QString::fromUtf8( str, len );
            }
            break;

        case CASS_VALUE_TYPE_UUID      :
            {
                CassUuid uuid;
                rc = cass_value_get_uuid( f_value.get(), &uuid );
                if( rc == CASS_OK )
                {
                    char str[CASS_UUID_STRING_LENGTH+1];
                    cass_uuid_string( uuid, str );
                    f_variant = QString(str);
                }
            }
            break;

        case CASS_VALUE_TYPE_TIMEUUID  :
            {
                CassUuid uuid;
                rc = cass_value_get_uuid( f_value.get(), &uuid );
                if( rc == CASS_OK )
                {
                    f_variant = static_cast<qulonglong>(cass_uuid_timestamp( uuid ));
                }
            }
            break;

        case CASS_VALUE_TYPE_INET      :
            {
                CassInet inet;
                rc = cass_value_get_inet( f_value.get(), &inet );
                if( rc == CASS_OK )
                {
                    char str[CASS_UUID_STRING_LENGTH+1];
                    cass_inet_string( inet, str );
                    f_variant = QString(str);
                }
            }
            break;

        default:
            throw std::runtime_error( "This type is not a bare type!" );
    }

    if( rc != CASS_OK )
    {
        throw std::runtime_error( "You cannot extract this value!" );
    }
}


const QString& Value::output() const
{
    if( f_stringOutput.isEmpty() )
    {
        switch( f_type )
        {
            case TypeUnknown:
                throw std::runtime_error( "Uninitialized type!" );

            case TypeVariant:
                if( f_variant.type() == QVariant::String )
                {
                    f_stringOutput = QString("'%1'").arg(f_variant.toString());
                }
                else
                {
                    f_stringOutput = f_variant.toString();
                }
                break;

            case TypeMap:
                {
                    QString content;
                    for( const auto& pair : f_map )
                    {
                        if( !content.isEmpty() )
                        {
                            content += ", ";
                        }
                        content += QString("'%1': %2")
                            .arg(pair.first)
                            .arg(pair.second.output())
                            ;
                    }
                    f_stringOutput = QString("{%1}").arg(content);
                }
                break;

            case TypeList:
                {
                    QString content;
                    for( const auto& entry : f_list )
                    {
                        if( !content.isEmpty() )
                        {
                            content += ", ";
                        }
                        content += entry.output();
                    }
                    f_stringOutput = QString("{%1}").arg(content);
                }
                break;
        }
    }

    return f_stringOutput;
}


}
// namespace QCassandraSchema

}
//namespace QtCassandra

// vim: ts=4 sw=4 et

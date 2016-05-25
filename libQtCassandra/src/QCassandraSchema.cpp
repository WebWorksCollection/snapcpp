/*
 * Text:
 *      QCassandraSchema.h
 *
 * Description:
 *      Database schema metadata.
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

#include "QtCassandra/QCassandraSchema.h"
#include "QtCassandra/QCassandraQuery.h"

#include "cassandra.h"

#include "CassTools.h"

#include <memory>
#include <map>
#include <QtCore>


namespace QtCassandra
{


using namespace CassTools;


namespace QCassandraSchema
{


//================================================================/
// SessionMeta
//
SessionMeta::SessionMeta( QCassandraSession::pointer_t s )
    : f_session(s)
{
}


SessionMeta::~SessionMeta()
{
}


SessionMeta::pointer_t SessionMeta::create( QCassandraSession::pointer_t s )
{
    return std::make_shared<SessionMeta>(s);
}


void SessionMeta::loadSchema()
{
    schema_meta_pointer_t schema_meta
        ( cass_session_get_schema_meta(f_session->session().get())
        , schemaMetaDeleter()
        );

    iterator_pointer_t iter
        ( cass_iterator_keyspaces_from_schema_meta( schema_meta.get() )
        , iteratorDeleter()
        );

    while( cass_iterator_next( iter.get() ) )
    {
        keyspace_meta_pointer_t p_keyspace( cass_iterator_get_keyspace_meta( iter.get() ), keyspaceMetaDeleter() );
        const char * name;
        size_t len;
        cass_keyspace_meta_name( p_keyspace.get(), &name, &len );
        KeyspaceMeta::pointer_t keyspace( std::make_shared<KeyspaceMeta>(shared_from_this()) );
        keyspace->f_name = QString::fromUtf8(name,len);
        f_keyspaces[keyspace->f_name] = keyspace;

        iterator_pointer_t fields_iter
            ( cass_iterator_fields_from_keyspace_meta( p_keyspace.get() )
            , iteratorDeleter()
            );
        while( cass_iterator_next( fields_iter.get() ) )
        {
            CassError rc = cass_iterator_get_meta_field_name( fields_iter.get(), &name, &len );
            if( rc != CASS_OK )
            {
                throw std::runtime_error( "Cannot get field name from iterator!" );
            }

            const QString field_name( QString::fromUtf8(name,len) );
            Value val;
            val.readValue(fields_iter);
            keyspace->f_fields[field_name] = val;
        }

        iterator_pointer_t tables_iter
            ( cass_iterator_tables_from_keyspace_meta(p_keyspace.get())
            , iteratorDeleter()
            );
        while( cass_iterator_next(tables_iter.get()) )
        {
            table_meta_pointer_t p_table
                ( cass_iterator_get_table_meta( tables_iter.get() )
                , tableMetaDeleter()
                );
            cass_table_meta_name( p_table.get(), &name, &len );
            using TableMeta = KeyspaceMeta::TableMeta;
            TableMeta::pointer_t table
                    ( std::make_shared<TableMeta>(keyspace) );
            table->f_keyspace = keyspace;
            table->f_name     = QString::fromUtf8(name,len);
            keyspace->f_tables[table->f_name] = table;

            iterator_pointer_t table_fields_iter
                ( cass_iterator_fields_from_table_meta( p_table.get() )
                , iteratorDeleter()
                );
            while( cass_iterator_next( table_fields_iter.get() ) )
            {
                CassError rc = cass_iterator_get_meta_field_name( table_fields_iter.get(), &name, &len );
                if( rc != CASS_OK )
                {
                    throw std::runtime_error( "Cannot get field name from iterator!" );
                }

                const QString field_name( QString::fromUtf8(name,len) );
                Value val;
                val.readValue(table_fields_iter);
                table->f_fields[field_name] = val;
            }

            iterator_pointer_t columns_iter
                ( cass_iterator_columns_from_table_meta( p_table.get() )
                , iteratorDeleter()
                );
            while( cass_iterator_next( columns_iter.get() ) )
            {
                column_meta_pointer_t p_col
                    ( cass_iterator_get_column_meta( columns_iter.get() )
                    , columnMetaDeleter()
                    );
                cass_column_meta_name( p_col.get(), &name, &len );

                using ColumnMeta = TableMeta::ColumnMeta;
                ColumnMeta::pointer_t column( std::make_shared<ColumnMeta>(table) );
                column->f_table = table;
                column->f_name  = QString::fromUtf8(name,len);
                table->f_columns[column->f_name] = column;

                CassColumnType type = cass_column_meta_type( p_col.get() );
                switch( type )
                {
                    case CASS_COLUMN_TYPE_REGULAR        : column->f_type = ColumnMeta::type_t::TypeRegular;        break;
                    case CASS_COLUMN_TYPE_PARTITION_KEY  : column->f_type = ColumnMeta::type_t::TypePartitionKey;   break;
                    case CASS_COLUMN_TYPE_CLUSTERING_KEY : column->f_type = ColumnMeta::type_t::TypeClusteringKey;  break;
                    case CASS_COLUMN_TYPE_STATIC         : column->f_type = ColumnMeta::type_t::TypeStatic;         break;
                    case CASS_COLUMN_TYPE_COMPACT_VALUE  : column->f_type = ColumnMeta::type_t::TypeCompactValue;   break;
                }

                CassValueType vt = cass_data_type_type( cass_column_meta_data_type(p_col.get()) );
                switch( vt )
                {
                    case CASS_VALUE_TYPE_UNKNOWN    :   column->f_columnType = column_type_t::TypeUnknown    ; break;
                    case CASS_VALUE_TYPE_CUSTOM     :   column->f_columnType = column_type_t::TypeCustom     ; break;
                    case CASS_VALUE_TYPE_DECIMAL    :   column->f_columnType = column_type_t::TypeDecimal    ; break;
                    case CASS_VALUE_TYPE_LAST_ENTRY :   column->f_columnType = column_type_t::TypeLast_entry ; break;
                    case CASS_VALUE_TYPE_UDT        :   column->f_columnType = column_type_t::TypeUdt        ; break;
                    case CASS_VALUE_TYPE_LIST       :   column->f_columnType = column_type_t::TypeList       ; break;
                    case CASS_VALUE_TYPE_SET        :   column->f_columnType = column_type_t::TypeSet        ; break;
                    case CASS_VALUE_TYPE_TUPLE      :   column->f_columnType = column_type_t::TypeTuple      ; break;
                    case CASS_VALUE_TYPE_MAP        :   column->f_columnType = column_type_t::TypeMap        ; break;
                    case CASS_VALUE_TYPE_BLOB       :   column->f_columnType = column_type_t::TypeBlob       ; break;
                    case CASS_VALUE_TYPE_BOOLEAN    :   column->f_columnType = column_type_t::TypeBoolean    ; break;
                    case CASS_VALUE_TYPE_FLOAT      :   column->f_columnType = column_type_t::TypeFloat      ; break;
                    case CASS_VALUE_TYPE_DOUBLE     :   column->f_columnType = column_type_t::TypeDouble     ; break;
                    case CASS_VALUE_TYPE_TINY_INT   :   column->f_columnType = column_type_t::TypeTinyInt    ; break;
                    case CASS_VALUE_TYPE_SMALL_INT  :   column->f_columnType = column_type_t::TypeSmallInt   ; break;
                    case CASS_VALUE_TYPE_INT        :   column->f_columnType = column_type_t::TypeInt        ; break;
                    case CASS_VALUE_TYPE_VARINT     :   column->f_columnType = column_type_t::TypeVarint     ; break;
                    case CASS_VALUE_TYPE_BIGINT     :   column->f_columnType = column_type_t::TypeBigint     ; break;
                    case CASS_VALUE_TYPE_COUNTER    :   column->f_columnType = column_type_t::TypeCounter    ; break;
                    case CASS_VALUE_TYPE_ASCII      :   column->f_columnType = column_type_t::TypeAscii      ; break;
                    case CASS_VALUE_TYPE_DATE       :   column->f_columnType = column_type_t::TypeDate       ; break;
                    case CASS_VALUE_TYPE_TEXT       :   column->f_columnType = column_type_t::TypeText       ; break;
                    case CASS_VALUE_TYPE_TIME       :   column->f_columnType = column_type_t::TypeTime       ; break;
                    case CASS_VALUE_TYPE_TIMESTAMP  :   column->f_columnType = column_type_t::TypeTimestamp  ; break;
                    case CASS_VALUE_TYPE_VARCHAR    :   column->f_columnType = column_type_t::TypeVarchar    ; break;
                    case CASS_VALUE_TYPE_UUID       :   column->f_columnType = column_type_t::TypeUuid       ; break;
                    case CASS_VALUE_TYPE_TIMEUUID   :   column->f_columnType = column_type_t::TypeTimeuuid   ; break;
                    case CASS_VALUE_TYPE_INET       :   column->f_columnType = column_type_t::TypeInet       ; break;
                }

                iterator_pointer_t meta_iter
                    ( cass_iterator_fields_from_column_meta( p_col.get() )
                    , iteratorDeleter()
                    );
                while( cass_iterator_next( meta_iter.get() ) )
                {
                    CassError rc = cass_iterator_get_meta_field_name( meta_iter.get(), &name, &len );
                    if( rc != CASS_OK )
                    {
                        throw std::runtime_error( "Cannot read field from set!" );
                    }
                    const QString field_name( QString::fromUtf8(name,len) );
                    Value val;
                    val.readValue(meta_iter);
                    column->f_fields[field_name] = val;
                }
            }
        }
    }
}


QCassandraSession::pointer_t SessionMeta::session() const
{
    return f_session;
}


const SessionMeta::KeyspaceMeta::map_t& SessionMeta::getKeyspaces()
{
    return f_keyspaces;
}


/** \brief Transform a SessionMeta in a blob.
 *
 * This function transforms a SessionMeta object into a blob that can be
 * transfered from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
QByteArray SessionMeta::encodeSessionMeta() const
{
    // at this time ours is nearly 120Kb... so reserve one block
    // 200Kb from the get go
    //
    QCassandraEncoder encoder(200 * 1024);

    // save the number of keyspaces
    //
    encoder.appendUInt16Value(f_keyspaces.size());
    for(auto keyspace : f_keyspaces)
    {
        keyspace.second->encodeKeyspaceMeta(encoder);
    }

    return encoder.result();
}


void SessionMeta::decodeSessionMeta(const QByteArray& encoded)
{
    QCassandraDecoder const decoder(encoded);

    size_t const keyspace_max(decoder.uint16Value());
    for(size_t idx(0); idx < keyspace_max; ++idx)
    {
        KeyspaceMeta::pointer_t keyspace(std::make_shared<KeyspaceMeta>(shared_from_this()));
        keyspace->decodeKeyspaceMeta(decoder);
        f_keyspaces[keyspace->getName()] = keyspace;
    }
}


//================================================================/
// KeyspaceMeta
//
SessionMeta::KeyspaceMeta::KeyspaceMeta( SessionMeta::pointer_t session_meta )
    : f_session(session_meta)
{
    // TODO add sub-fields
}


/** \brief Generate CQL string to create the keyspace
 */
QString SessionMeta::KeyspaceMeta::getCqlString() const
{
    QStringList cql;
    cql << QString("CREATE KEYSPACE IF NOT EXISTS %1").arg(f_name);
    cql << "WITH replication =";
    cql << f_fields.at("replication").output();
    cql << "AND durable_writes = " + f_fields.at("durable_writes").output();
    cql << "";

    for( auto table : f_tables )
    {
        cql << table.second->getCqlString();
    }

    return cql.join('\n');
}


const QString& SessionMeta::KeyspaceMeta::getName() const
{
    return f_name;
}


const Value::map_t&
    SessionMeta::KeyspaceMeta::getFields() const
{
    return f_fields;
}


Value::map_t&
    SessionMeta::KeyspaceMeta::getFields()
{
    return f_fields;
}


Value& SessionMeta::KeyspaceMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


const SessionMeta::KeyspaceMeta::TableMeta::map_t&
    SessionMeta::KeyspaceMeta::getTables() const
{
    return f_tables;
}


/** \brief Transform a KeyspaceMeta in a blob.
 *
 * This function transforms a KeyspaceMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void SessionMeta::KeyspaceMeta::encodeKeyspaceMeta(QCassandraEncoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the keyspace fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the tables
    // first the size on 2 bytes then each table
    encoder.appendUInt16Value(f_tables.size());
    for( auto t : f_tables )
    {
        t.second->encodeTableMeta(encoder);
    }
}


/** \brief Decode a KeyspaceMeta object from a blob.
 *
 */
void SessionMeta::KeyspaceMeta::decodeKeyspaceMeta(const QCassandraDecoder& decoder)
{
    // retrieve the keyspace name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the tables
    //
    size_t const table_max(decoder.uint16Value());
    for(size_t idx(0); idx < table_max; ++idx)
    {
        SessionMeta::KeyspaceMeta::TableMeta::pointer_t table(std::make_shared<SessionMeta::KeyspaceMeta::TableMeta>(shared_from_this()));
        table->decodeTableMeta(decoder);
        f_tables[table->getName()] = table;
    }
}



//================================================================/
// TableMeta
//
SessionMeta::KeyspaceMeta::TableMeta::TableMeta( KeyspaceMeta::pointer_t kysp )
    : f_keyspace(kysp)
{
}


const QString& SessionMeta::KeyspaceMeta::TableMeta::getName() const
{
    return f_name;
}


const Value::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::getFields() const
{
    return f_fields;
}


Value::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::getFields()
{
    return f_fields;
}


Value& SessionMeta::KeyspaceMeta::TableMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


const SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::getColumns() const
{
    return f_columns;
}


/** \brief Transform a TableMeta in a blob.
 *
 * This function transforms a TableMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void SessionMeta::KeyspaceMeta::TableMeta::encodeTableMeta(QCassandraEncoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the table fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    //for( auto f(f_fields.begin()); f != f_fields.end(); ++f )
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the columns
    // first the size on 2 bytes then each table
    encoder.appendUInt16Value(f_columns.size());
    for( auto c : f_columns )
    {
        c.second->encodeColumnMeta(encoder);
    }
}


/** \brief Decode a TableMeta object from a blob.
 *
 */
void SessionMeta::KeyspaceMeta::TableMeta::decodeTableMeta(const QCassandraDecoder& decoder)
{
    // retrieve the table name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the columns
    //
    size_t const column_max(decoder.uint16Value());
    for(size_t idx(0); idx < column_max; ++idx)
    {
        SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::pointer_t column(std::make_shared<SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta>(shared_from_this()));
        column->decodeColumnMeta(decoder);
        f_columns[column->getName()] = column;
    }
}


/** \brief Generate CQL string to create the table
 */
QString SessionMeta::KeyspaceMeta::TableMeta::getCqlString() const
{
    QStringList cql;
    cql << QString("CREATE TABLE IF NOT EXISTS %1.%2")
           .arg(f_keyspace->getName())
           .arg(f_name);

    for( auto column : f_columns )
    {
        cql << column.second->getCqlString();
    }

#if 0
    cql << "WITH replication =";
    cql << f_fields["replication"].output();
    cql << "AND durable_writes = " + f_fields["durable_writes"].output();
    cql << "";
#endif
    return cql.join('\n');
}


//================================================================/
// ColumnMeta
//
SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::ColumnMeta( SessionMeta::KeyspaceMeta::TableMeta::pointer_t table )
    : f_table(table)
{
}


const QString&
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getName() const
{
    return f_name;
}


QString SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getCqlString() const
{
    // TODO
    return QString();
}


SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::type_t
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getType() const
{
    return f_type;
}


const Value::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getFields() const
{
    return f_fields;
}


Value::map_t&
    SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::getFields()
{
    return f_fields;
}


Value& SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::operator[]( const QString& name )
{
    return f_fields[name];
}


/** \brief Transform a ColumnMeta in a blob.
 *
 * This function transforms a ColumnMeta object into a blob that can be
 * transferred from snapdbproxy to a client.
 *
 * \return The meta data in a blob.
 */
void SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::encodeColumnMeta(QCassandraEncoder& encoder) const
{
    // save the name as a PSTR with a size on 2 bytes
    //
    encoder.appendP16StringValue(f_name);

    // save the table fields
    // first the size on 2 bytes then each field
    encoder.appendUInt16Value(f_fields.size());
    for( auto f : f_fields )
    {
        // field name
        encoder.appendP16StringValue(f.first);

        // field value
        f.second.encodeValue(encoder);
    }

    // save the column type
    // there are only a very few types so one char is enough
    encoder.appendUnsignedCharValue(static_cast<unsigned char>(f_type));
}


/** \brief Decode a ColumnMeta object from a blob.
 *
 */
void SessionMeta::KeyspaceMeta::TableMeta::ColumnMeta::decodeColumnMeta(const QCassandraDecoder& decoder)
{
    // retrieve the column name
    //
    f_name = decoder.p16StringValue();

    // read field values
    //
    size_t const field_max(decoder.uint16Value());
    for(size_t idx(0); idx < field_max; ++idx)
    {
        // field name
        QString const name(decoder.p16StringValue());

        // field value
        Value field;
        field.decodeValue(decoder);

        // save field in our map
        f_fields[name] = field;
    }

    // retrieve the column type
    //
    f_type = static_cast<type_t>(decoder.unsignedCharValue());
}




} //namespace QCassandraSchema
} // namespace QtCassandra
// vim: ts=4 sw=4 et

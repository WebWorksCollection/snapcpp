//===============================================================================
// Copyright (c) 2005-2016 by Made to Order Software Corporation
// 
// All Rights Reserved.
// 
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#include "RowModel.h"

#include <snapwebsites/dbutils.h>
#include <snapwebsites/not_used.h>
#include <snapwebsites/qstring_stream.h>

#include <QtCore>

#include "poison.h"

using namespace QtCassandra;


RowModel::RowModel()
{
}


const QByteArray& RowModel::rowKey() const
{
    return f_rowKey;
}


void RowModel::setRowKey( const QByteArray& val )
{
    f_rowKey = val;
}


void RowModel::doQuery()
{
    auto query = std::make_shared<QCassandraQuery>(f_session);
    query->query(
        QString("SELECT column1,value FROM %1.%2 WHERE key = ?")
            .arg(f_keyspaceName)
            .arg(f_tableName)
        , 1
        );
    query->setPagingSize( 10 );
    query->bindByteArray( 0, f_rowKey );

    QueryModel::doQuery( query );
}


void RowModel::fetchCustomData( QCassandraQuery::pointer_t q )
{
    f_columns.push_back( q->getByteArrayColumn(1) );
}


Qt::ItemFlags RowModel::flags( const QModelIndex & idx ) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if( idx.column() == 1 )
    {
        f |= Qt::ItemIsEditable;
    }
    return f;
}


QVariant RowModel::data( QModelIndex const & idx, int role ) const
{
    if( role == Qt::UserRole )
    {
        return QueryModel::data( idx, role );
    }

    if( role != Qt::DisplayRole && role != Qt::EditRole )
    {
        return QVariant();
    }

    if( idx.column() < 0 || idx.column() > 1 )
    {
        Q_ASSERT(false);
        return QVariant();
    }

    try
    {
        auto const column_name(  *(f_rows.begin()    + idx.row()) );
        auto const column_value( *(f_columns.begin() + idx.row()) );
        snap::dbutils du( f_tableName, QString::fromUtf8(f_rowKey.data()) );
        switch( idx.column() )
        {
            case 0:
                du.set_display_len( 24 );
                return du.get_column_name( column_name );

            case 1:
                du.set_display_len( 64 );
                return du.get_column_value( column_name, column_value, role == Qt::DisplayRole /*display_only*/ );

            default:
                throw std::runtime_error( "We should never get here!" );
        }
    }
    catch( std::exception const& except )
    {
        displayError( except, tr("Cannot read data from database.") );
    }

    return QueryModel::data( idx, role );
}


int RowModel::columnCount( const QModelIndex & /*parent*/ ) const
{
    return 2;
}


bool RowModel::setData( const QModelIndex & idx, const QVariant & value, int role )
{
    if( role != Qt::EditRole )
    {
        return false;
    }

    try
    {
        QByteArray result;
        snap::dbutils du( f_tableName, f_rowKey );
        const QByteArray& key( f_rows[idx.row()] );
        du.set_column_value( key, result, value.toString() );

        QCassandraQuery q( f_session );
        q.query(
                    QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                    , 3
                    );
        q.bindByteArray( 0, f_rowKey );
        q.bindByteArray( 1, key      );
        q.bindByteArray( 2, result   );
        q.start();
        q.end();

        Q_EMIT dataChanged( idx, idx );

        return true;
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot write data to database.") );
    }

    return false;
}


bool RowModel::setHeaderData( int /*section*/, Qt::Orientation /*orientation*/, const QVariant & /*value*/, int /*role*/ )
{
    return false;
}


bool RowModel::insertRows ( int row, int count, const QModelIndex & parent_index )
{
    beginInsertRows( parent_index, row, count );

    bool retval( true );
    try
    {
        for( int i = 0; i < count; ++i )
        {
            const QByteArray newcol( QString("New column %1").arg(i).toUtf8() );
            const QByteArray newval( QString("New value %1").arg(i).toUtf8() );
            f_rows.insert    ( f_rows.begin()    + (row+i), newcol );
            f_columns.insert ( f_columns.begin() + (row+i), newval );

            QByteArray result;
            snap::dbutils du( f_tableName, f_rowKey );
            du.set_column_value( newcol, result, newval );

            QCassandraQuery q( f_session );
            q.query(
                        QString("INSERT INTO %1.%2 (key,column1,value) VALUES (?,?,?)")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                        , 3
                        );
            q.bindByteArray( 0, f_rowKey );
            q.bindByteArray( 1, newcol   );
            q.bindByteArray( 2, newval   );
            q.start();
            q.end();
        }
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot add rows to database.") );
        retval = false;
    }

    endInsertRows();

    return retval;
}


bool RowModel::removeRows ( int row, int count, const QModelIndex & )
{
    // Make a list of the keys we will drop
    //
    QList<QByteArray> key_list;
    for( int idx = 0; idx < count; ++idx )
    {
        const QByteArray key(f_rows[idx + row]);
        key_list << key;
    }

    try
    {
        // Drop each key
        //
        for( auto key : key_list )
        {
            // TODO: this might be pretty slow. I need to utilize the "prepared query" API.
            //
            QCassandraQuery q( f_session );
            q.query(
                        QString("DELETE FROM %1.%2 WHERE key = ? AND column1 = ?")
                        .arg(f_keyspaceName)
                        .arg(f_tableName)
                        , 2
                        );
            q.bindByteArray( 0, f_rowKey );
            q.bindByteArray( 1, key 	 );
            q.start();
            q.end();
        }

        // Remove the columns from the model
        //
        beginRemoveRows( QModelIndex(), row, row+count );
        f_rows.erase( f_rows.begin()+row, f_rows.begin()+row+count );
        endRemoveRows();
    }
    catch( const std::exception& except )
    {
        displayError( except, tr("Cannot remove rows to database.") );
        return false;
    }

    return true;
}


// vim: ts=4 sw=4 et

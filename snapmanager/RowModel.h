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

#pragma once

#include <QtCassandra/QueryModel.h>

class RowModel
    : public QtCassandra::QueryModel
{
    Q_OBJECT

public:
    RowModel();

    // Read access
    //
    virtual Qt::ItemFlags   flags           ( const QModelIndex & index ) const;
    virtual QVariant        data            ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
    virtual int             columnCount     ( const QModelIndex & parent = QModelIndex() ) const;

    virtual bool            fetchCustomData ( QtCassandra::QCassandraQuery::pointer_t q );

    // Write access
    //
    virtual bool            setData         ( const QModelIndex & index, const QVariant & value, int role = Qt::EditRole );
    virtual bool            setHeaderData   ( int section, Qt::Orientation orientation, const QVariant & value, int role = Qt::EditRole );

    // Resizable methods
    //
    virtual bool            insertRows      ( int row, int count, const QModelIndex & parent = QModelIndex() );
    virtual bool            removeRows      ( int row, int count, const QModelIndex & parent = QModelIndex() );

private:
    std::vector<QByteArray> f_columns;
    QByteArray              f_rowKey;
};


// vim: ts=4 sw=4 et

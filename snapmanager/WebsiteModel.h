// Copyright (C) 2015-2016  Made to Order Software Corp.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#pragma once

#include <QtCassandra/QueryModel.h>

#include <QModelIndex>

class WebsiteModel
    : public QtCassandra::QueryModel
{
    Q_OBJECT

public:
    WebsiteModel();

    void setDomainOrgName( const QString& val ) { f_domain_org_name = val; }

    // Read only access
    //
    virtual bool     fetchFilter( const QByteArray& key );
    virtual QVariant data       ( QModelIndex const & index, int role = Qt::DisplayRole ) const;

    void doQuery();

private:
    QString f_domain_org_name;
};

// vim: ts=4 sw=4 et

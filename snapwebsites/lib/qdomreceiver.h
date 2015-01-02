// Snap Websites Servers -- generate a DOM from the output of an XML Query
// Copyright (C) 2011-2015  Made to Order Software Corp.
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
#ifndef _QXMLRECEIVER_H
#define _QXMLRECEIVER_H

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QAbstractXmlReceiver>
#include <QDomDocument>
#include <QDomElement>
#include <QXmlNamePool>
#pragma GCC diagnostic pop

class QDomReceiver : public QAbstractXmlReceiver
{
public:
                    QDomReceiver(QXmlNamePool namepool, QDomDocument doc);
    virtual         ~QDomReceiver();
    virtual void    atomicValue(const QVariant& value);
    virtual void    attribute(const QXmlName& name, const QStringRef& value);
    virtual void    characters(const QStringRef& value);
    virtual void    comment(const QString& value);
    virtual void    endDocument();
    virtual void    endElement();
    virtual void    endOfSequence();
    virtual void    namespaceBinding(const QXmlName& name);
    virtual void    processingInstruction(const QXmlName& target, const QString& value );
    virtual void    startDocument();
    virtual void    startElement(const QXmlName& name);
    virtual void    startOfSequence();

private:
    QXmlNamePool    f_namepool;
    QDomDocument    f_doc;
    QDomElement     f_element;
};

#endif
// _QXMLRECEIVER_H
// vim: ts=4 sw=4 et

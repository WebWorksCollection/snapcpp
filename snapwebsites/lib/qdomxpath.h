// Snap Websites Servers -- generate a DOM from the output of an XML Query
// Copyright (C) 2013-2014  Made to Order Software Corp.
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
#ifndef _QXMLXPATH_H
#define _QXMLXPATH_H

#include <stdexcept>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#include <QDomNode>
#include <QString>
#include <QMap>
#include <QVector>
#pragma GCC diagnostic pop


class QDomXPathException : public std::runtime_error
{
public:
    QDomXPathException(std::string const& err)
        : runtime_error(err)
    {
    }
};

class QDomXPathException_InternalError : public QDomXPathException
{
public:
    QDomXPathException_InternalError(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_UndefinedInstructionError : public QDomXPathException
{
public:
    QDomXPathException_UndefinedInstructionError(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_UnknownFunctionError : public QDomXPathException
{
public:
    QDomXPathException_UnknownFunctionError(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidMagic : public QDomXPathException
{
public:
    QDomXPathException_InvalidMagic(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidError : public QDomXPathException
{
public:
    QDomXPathException_InvalidError(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidCharacter : public QDomXPathException
{
public:
    QDomXPathException_InvalidCharacter(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_InvalidString : public QDomXPathException
{
public:
    QDomXPathException_InvalidString(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_TooManyUnget : public QDomXPathException
{
public:
    QDomXPathException_TooManyUnget(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_SyntaxError : public QDomXPathException
{
public:
    QDomXPathException_SyntaxError(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_ExecutionTime : public QDomXPathException
{
public:
    QDomXPathException_ExecutionTime(std::string const& err)
        : QDomXPathException(err)
    {
    }
};

class QDomXPathException_NotImplemented : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_NotImplemented(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_OutOfRange : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_OutOfRange(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_EmptyStack : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_EmptyStack(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_EmptyContext : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_EmptyContext(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_WrongType : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_WrongType(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};

class QDomXPathException_UndefinedVariable : public QDomXPathException_ExecutionTime
{
public:
    QDomXPathException_UndefinedVariable(std::string const& err)
        : QDomXPathException_ExecutionTime(err)
    {
    }
};



class QDomXPath
{
public:
    typedef QVector<QDomNode>       node_vector_t;
    typedef QMap<QString, QString>  bind_vector_t;
    typedef uint8_t                 instruction_t;
    typedef QVector<instruction_t>  program_t;

    static char const *             MAGIC;
    static instruction_t const      VERSION_MAJOR = 1;
    static instruction_t const      VERSION_MINOR = 0;

                                    QDomXPath();
                                    ~QDomXPath();

    bool                            setXPath(QString const& xpath, bool show_commands = false);
    QString                         getXPath() const;
    void                            setProgram(program_t const& program, bool show_commands = false);
    program_t const&                getProgram() const;

    void                            bindVariable(QString const& name, QString const& value);
    bool                            hasVariable(QString const& name);
    QString                         getVariable(QString const& name);

    node_vector_t                   apply(QDomNode node) const;
    node_vector_t                   apply(node_vector_t node) const;
    void                            disassemble() const;

private:
    class QDomXPathImpl;
    friend class QDomXPathImpl;

    QString                         f_xpath;
    QDomXPathImpl *                 f_impl;
    bind_vector_t                   f_variables;
};

#endif
// _QXMLXPATH_H
// vim: ts=4 sw=4 et

/*
 * Text:
 *      QCassandraColumnDefinition.cpp
 *
 * Description:
 *      Handling of the cassandra::ColumnDef.
 *
 * Documentation:
 *      See each function below.
 *
 * License:
 *      Copyright (c) 2011-2013 Made to Order Software Corp.
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

#include "QtCassandra/QCassandraColumnDefinition.h"
#include "legacy/cassandra_types.h"

#include <stdexcept>
#include <QtCassandra/QCassandraTable.h>

namespace QtCassandra
{

/** \class QCassandraColumnDefinition
 * \brief Hold a Cassandra column definition.
 *
 * This class is used to read and create column definitions to use with
 * the Cassandra database system.
 */

/** \typedef QCassandraColumnDefinition::QCassandraIndexOptions
 * \brief A set of options.
 *
 * Columns support any number of options. These can be defined in a map
 * and passed to the QCassandraColumnDefinition via its setIndexOptions()
 * function.
 *
 * The current options can be read using the indexOptions() function.
 *
 * \sa setIndexOptions()
 */

/** \var QCassandraColumnDefinition::f_private
 * \brief Pointer to the private data of the column definition.
 *
 * A pointer to the private part of this column definition. This means
 * a pointer to a ColumnDef structure.
 */

/** \var QCassandraColumnDefinition::f_table
 * \brief Pointer back to the table that created this column definition.
 *
 * A pointer back to the table that created this column definition.
 *
 * This is a bare pointer since a table has a shared pointer to this
 * column definition (i.e. when the table goes, the column definition
 * is gone.)
 */

/** \var QCassandraColumnDefinition::f_index_options
 * \brief The current set of name/value defining this column.
 *
 * This variable holds the set of options defining this column parameters.
 *
 * Note that some parameters of columns are defined using other
 * parameters defined in the f_private variable. For example, the
 * column data class is defined as a separate parameter.
 */

/** \enum QCassandraColumnDefinition::index_type_t
 * \brief Define the type of index.
 *
 * This enumeration defines the type of index used by Cassandra.
 *
 * At this time only KEYS is defined as an index type. The other two
 * values are used when the type cannot be determined or is defined
 * by not known (i.e. it's not KEYS.)
 */

/** \var QCassandraColumnDefinition::INDEX_TYPE_UNKNOWN
 *
 * When an index type is defined but the value is not recognized by
 * the libQtCassandra, then this value is used instead.
 */

/** \var QCassandraColumnDefinition::INDEX_TYPE_UNDEFINED
 *
 * When an index type is not defined (__isset is false) then this
 * value is returned.
 */

/** \var QCassandraColumnDefinition::INDEX_TYPE_KEYS
 *
 * The index type is KEYS.
 */



/** \brief Initialize a QCassandraColumnDefinition object.
 *
 * This function initializes a QCassandraColumnDefinition object.
 *
 * All the parameters are set to the defaults as defined in the Cassandra
 * definition of the ColumnDef message. You can use the different functions of
 * this class to change the default values.
 *
 * \param[in] table  The table this column is part of.
 * \param[in] column_name  The name of the column.
 */
QCassandraColumnDefinition::QCassandraColumnDefinition(QCassandraTable::pointer_t table, const QString& column_name)
    : f_private(new ColumnDef)
    , f_table(table)
{
    // we save the name and at this point we prevent it from being changed.
    f_private->__set_name(column_name.toStdString());
}

/** \brief Clean up the QCassandraColumnDefinition object.
 *
 * This function ensures that all resources allocated by the
 * QCassandraColumnDefinition are released.
 */
QCassandraColumnDefinition::~QCassandraColumnDefinition()
{
}

/** \brief Retrieve the name of the column.
 *
 * This function returns the name of the column as specified in the
 * constructor.
 *
 * The name cannot be changed.
 *
 * \return A string with the column name.
 */
QString QCassandraColumnDefinition::columnName() const
{
    return f_private->name.c_str();
}

/** \brief Set the validation class of this column.
 *
 * Note that the validation class is a mandatory parameter although it
 * can be set to "" to get the default class.
 *
 * \param[in] name  The name of the new validation class to use with this column.
 */
void QCassandraColumnDefinition::setValidationClass(const QString& name)
{
    f_private->__set_validation_class(name.toUtf8().data());
}

/** \brief Retrieve the validation class.
 *
 * This function returns the name of the validation class of this column.
 *
 * \return The validation class name.
 */
QString QCassandraColumnDefinition::validationClass() const
{
    return f_private->validation_class.c_str();
}

/** \brief Set the type of index used on this column.
 *
 * Cassandra offers the ability to build an index over a column. This parameter
 * defines the type of that index.
 *
 * The currently valid values are:
 *
 * \li INDEX_TYPE_KEYS -- create an index with the value as a key
 *
 * \exception runtime_error
 * If the index_type value does not correspond to one of the valid types as
 * defined in the QCassandraColumnDefinition, then this exception is raised.
 *
 * \param[in] index_type  The type of index.
 */
void QCassandraColumnDefinition::setIndexType(index_type_t index_type)
{
    switch(index_type)
    {
    case INDEX_TYPE_KEYS:
        f_private->__set_index_type(IndexType::KEYS);
        break;

    default:
        throw std::runtime_error("unrecognized index_type value in QCassandraColumnDefinition::set_index_type()");
    }
}

/** \brief Remove the index type definition.
 *
 * The index type parameter is optional and can be cleared. This does not mean
 * that the index is removed from the column, just that the information is not
 * sent over the network. (TBD)
 */
void QCassandraColumnDefinition::unsetIndexType()
{
    f_private->__isset.index_type = false;
}

/** \brief Check whether the index type is defined.
 *
 * This function retrieves the current status of the index type parameter.
 *
 * \return True if the index type parameter is defined.
 */
bool QCassandraColumnDefinition::hasIndexType() const
{
    return f_private->__isset.index_type;
}

/** \brief Retrieve the type of index.
 *
 * This function returns the current type of index or INDEX_TYPE_UNDEFINED
 * if the index is not currently defined.
 *
 * If the index type is defined but not known by the QtCassandra library,
 * then INDEX_TYPE_UNKNOWN is returned.
 *
 * \return The index type or UNDEFINED.
 */
QCassandraColumnDefinition::index_type_t QCassandraColumnDefinition::indexType() const
{
    if(!f_private->__isset.index_type)
    {
        return INDEX_TYPE_UNDEFINED;
    }
    switch(f_private->index_type)
    {
    case IndexType::KEYS:
        return INDEX_TYPE_KEYS;

    default:
        return INDEX_TYPE_UNKNOWN;
    }
}

/** \brief Set the name of the index table.
 *
 * Define the name that the system gives the index table.
 *
 * \param[in] name  The index table name.
 */
void QCassandraColumnDefinition::setIndexName(const QString& name)
{
    f_private->__set_index_name(name.toUtf8().data());
}

/** \brief Unset the index name.
 *
 * This function marks the index name as not set.
 */
void QCassandraColumnDefinition::unsetIndexName()
{
    f_private->__isset.index_name = false;
}

/** \brief Check whether the index name is defined.
 *
 * This function retrieves the current status of the index name parameter.
 *
 * \return True if the index name parameter is defined.
 */
bool QCassandraColumnDefinition::hasIndexName() const
{
    return f_private->__isset.index_name;
}

/** \brief Retrieve the current name of this column index.
 *
 * This function returns the name of this column index.
 *
 * When the name is not set the function returns an empty string.
 *
 * \return The name of the index.
 */
QString QCassandraColumnDefinition::indexName() const
{
    if(!f_private->__isset.index_name)
    {
        return "";
    }
    return f_private->index_name.c_str();
}

/** \brief Replace all the index options.
 *
 * This function overwrites all the index options with the ones
 * specified in the input parameter.
 *
 * This function can be used to clear all the options by passing an
 * empty options parameter.
 *
 * \since Cassandra version 1.0.0
 *
 * \param[in] options  The replacing options.
 */
void QCassandraColumnDefinition::setIndexOptions(const QCassandraIndexOptions& options)
{
    f_index_options = options;
}

/** \brief Get the map of all index options.
 *
 * The column definition maintains a map indexed by option name of all the
 * index options of the column definition. This function retreives a constant
 * reference to that list.
 *
 * If you keep the returned reference as such (i.e. a reference) then make sure
 * you do not modify the options (calling one of the setIndexOptions()
 * or setIndexOption() functions.) Otherwise the reference may become
 * invalid. If you are going to modify the options, make a copy of the map.
 *
 * \return A reference to the map of index options.
 */
const QCassandraColumnDefinition::QCassandraIndexOptions& QCassandraColumnDefinition::indexOptions() const
{
    return f_index_options;
}

/** \brief Add or replace one of the index options.
 *
 * This function sets the specified \p option to the specified \p value.
 *
 * \param[in] option  The option to set.
 * \param[in] value  The new value of the option to set.
 */
void QCassandraColumnDefinition::setIndexOption(const QString& option, const QString& value)
{
    f_index_options[option] = value;
}

/** \brief Retrieve an index option.
 *
 * This function retrieves the index option named by the \p option
 * parameter.
 *
 * \param[in] option  The name of the index option to retrieve.
 *
 * \return The value of the named index option or an empty string.
 */
QString QCassandraColumnDefinition::indexOption(const QString& option) const
{
    if(!f_index_options.contains(option))
    {
        return "";
    }
    return f_index_options[option];
}

/** \brief Delete an index option from the current list of index options.
 *
 * This function deletes the specified index option from the list of index
 * options in memory. If the option was not defined, the function has no
 * effect.
 *
 * \note
 * This is useful to manage the list of index options, however, erasing an
 * option here only tells the system to use the default value. It does
 * not prevent the system from having that option defined.
 *
 * \param[in] option  The name of the index option to delete.
 */
void QCassandraColumnDefinition::eraseIndexOption(const QString& option)
{
    f_index_options.erase(f_index_options.find(option));
}

/** \brief This is an internal function used to parse a ColumnDef structure.
 *
 * This function is called internally to parse a ColumnDef object.
 *
 * \param[in] data  The pointer to the ColumnDef object.
 */
void QCassandraColumnDefinition::parseColumnDefinition(const ColumnDef *col)
{
    // column name
    if(col->name != f_private->name)
    {
        // what do we do here?
        throw std::logic_error("ColumnDef names don't match");
    }

    // validation class
    f_private->__set_validation_class(col->validation_class);

    // index type
    if(col->__isset.index_type) {
        f_private->__set_index_type(col->index_type);
    }
    else {
        f_private->__isset.index_type = false;
    }

    // index name
    if(col->__isset.index_name) {
        f_private->__set_index_name(col->index_name);
    }
    else {
        f_private->__isset.index_name = false;
    }

    // list of index options
    f_index_options.clear();
    if(col->__isset.index_options)
    {
        for( auto o : col->index_options )
        {
            f_index_options.insert( o.first.c_str(), o.second.c_str() );
        }
    }
}

/** \brief Prepare a column definition.
 *
 * This function copies the private ColumnDef in the specified
 * \p col parameter.
 *
 * \param[in] col  The destination buffer for the column definition.
 */
void QCassandraColumnDefinition::prepareColumnDefinition( ColumnDef *col ) const
{
    // Copy the structure up the food chain
    //
    *col = *f_private;

    // Clear the existing options
    //
    col->index_options.clear();
    col->__isset.index_options = false;

    // Copy the index options if non-empty
    //
    if( !f_index_options.isEmpty() )
    {
        std::map<std::string,std::string> map;
        for( const auto& key : f_index_options.keys() )
        {
            map[key.toStdString()] = f_index_options[key].toStdString();
        }
        col->__set_index_options( map );
    }
}

} // namespace QtCassandra
// vim: ts=4 sw=4 et

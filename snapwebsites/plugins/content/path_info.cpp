// Snap Websites Server -- all the user content and much of the system content
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


/** \file
 * \brief The implementation of the content plugin path_info class.
 *
 * This file contains the path_info class implementation.
 */

#include "content.h"

#include "log.h"

#include "poison.h"


SNAP_PLUGIN_EXTENSION_START(content)


/** \brief Initialize the path_info_t object to an empty object.
 *
 * The consturctor sets all the values to an empty string or undefined
 * version.
 *
 * You must call the set_path() function at least once to setup this
 * object properly and then use it. Until then, errors will ensue if
 * you attempt to use the object.
 *
 * If you have a path that is already canonicalized (you have to be
 * 100% sure that it is indeed canonicalized to be secure) you may
 * use the set_real_path() function. It is optimized and will skip
 * on the canonicalization of the path.
 *
 * \sa set_path()
 * \sa set_real_path()
 */
path_info_t::path_info_t()
    : f_content_plugin(content::content::instance())
    , f_snap(f_content_plugin->get_snap())
    //, f_key("") -- auto-init
    //, f_real_key("") -- auto-init
    //, f_cpath("") -- auto-init
    //, f_real_cpath("") -- auto-init
    //, f_main_page(false) -- auto-init
    //, f_parameters() -- auto-init
    //, f_branch(snap_version::SPECIAL_VERSION_UNDEFINED) -- auto-init
    //, f_revision(snap_version::SPECIAL_VERSION_UNDEFINED) -- auto-init
    //, f_revision_string("") -- auto-init
    //, f_branch_key("") -- auto-init
    //, f_revision_key("") -- auto-init
    //, f_locale("") -- auto-init
{
}


/** \brief Set the path of this path_info_t structure.
 *
 * This function takes a full path or relative path to an existing page,
 * a page to be created, or simply a page to be checked out.
 *
 * A relative path is expected to start with a root path name (i.e. no
 * slash), although a slash is legal and will work as well.
 *
 * If you have a path which you know for sure is already canonicalized,
 * you may instead call the set_real_path() function. That way you will
 * skip on the canonicalization. However, it is a security risk because
 * without proper canonicalization, you may end up with the wrong path
 * (i.e. it could be a path to another website data!)
 *
 * \param[in] path  The new path for this path_infot_t structure.
 *
 * \sa set_real_path()
 */
void path_info_t::set_path(QString const & path)
{
    if(!f_initialized
    || (path != f_cpath && path != f_key))
    {
        f_initialized = true;

        QString const & site_key(f_snap->get_site_key_with_slash());
        if(path.startsWith(site_key))
        {
            // already canonicalized
            f_key = path;
            f_cpath = path.mid(site_key.length());
        }
        else
        {
            // TODO: check whether the path starts with http[s] or some
            //       other protocol; if so, forget it because we do not
            //       allow such in the path anyway! This could catch some
            //       security problems along the way too.
            //
            // may require canonicalization
            f_cpath = path;
            f_snap->canonicalize_path(f_cpath);
            f_key = site_key + f_cpath;
        }

        // retrieve the action from this path
        // (note that in case of the main page the action is NOT included)
        // f_action will be an empty string if no action was specified
        try
        {
            snap_uri const uri(f_key);
            QString const action(uri.query_option(server::server::instance()->get_parameter("qs_action")));
            if(!action.isEmpty())
            {
                set_parameter("action", action);
            }
        }
        catch(snap_uri_exception_invalid_uri const &)
        {
            // add an error so we can get more information about the full key
            // that created an exception
            SNAP_LOG_ERROR("URI \"")(f_key)("\" was not accepted.");
            throw;
        }

        // the other info becomes invalid
        clear();
    }
}


/** \brief Set the path of this path_info_t structure.
 *
 * This function takes a canonicalized path, which may be a full path
 * or a relative path to an existing page, a page to be created, or
 * simply a page to be checked out.
 *
 * A relative path is expected to start with a root path name (i.e. no
 * slash), although a slash is legal and will work as well.
 *
 * If you have a path which you know for sure is already canonicalized,
 * you may this function. Otherwise, it is generally preferable to
 * call the set_path() function as it verifies whether the path is valid
 * for this website.
 *
 * \warning
 * This function may introduce a security risk if the path you pass
 * to it is not properly canonicalized.
 *
 * \param[in] path  The new path for this path_infot_t structure.
 *
 * \sa set_path()
 */
void path_info_t::set_real_path(QString const & path)
{
    if(!f_initialized
    || (path != f_real_cpath && path != f_real_key))
    {
        f_initialized = true;

        QString const& site_key(f_snap->get_site_key_with_slash());
        if(path.startsWith(site_key))
        {
            // already canonicalized
            f_real_key = path;
            f_real_cpath = path.mid(site_key.length());
        }
        else
        {
            // may require canonicalization
            f_real_cpath = path;
            f_snap->canonicalize_path(f_real_cpath);
            f_real_key = f_snap->get_site_key_with_slash() + f_real_cpath;
        }

        // the other info becomes invalid
        // except for the parameters which we keep in place
        clear(true);
    }
}


/** \brief Set whether the path represents the main page or not.
 *
 * This function is used to mark a path as the one representing the
 * main page being generated. This makes a huge difference, for
 * example if the user does not have permissions to access the
 * main page, then the system generates a 503. If the page is
 * not the main page, it is simply dropped (i.e. its content cannot
 * be shown to the current user so it does not get included in
 * the final output.)
 *
 * \param[in] main_page  Whether this path represents the main page.
 */
void path_info_t::set_main_page(bool main_page)
{
    // Note: we could check with f_snap->get_uri() except that in some
    //       situations we may want to have main_page set to true even
    //       though the path is not the URI path used to access the site
    if(f_main_page != main_page)
    {
        clear();
        f_main_page = main_page;
    }
}


/** \brief Attach a parameter to this path_info_t object.
 *
 * This function is used to save a named parameter to this path_info_t
 * object.
 *
 * This function is seldom used, yet at times you pass a path_info_t
 * objects to many different functions, some of which need to know
 * your status.
 *
 * \param[in] name  The name of the parameter.
 * \param[in] value  The value of this parameter.
 */
void path_info_t::set_parameter(QString const & name, QString const & value)
{
    f_parameters[name] = value;
}


/** \brief Force the branch number to the specified branch.
 *
 * By default, the system allocates a branch number as required. Either
 * zero (0) for a system branch, or one (1) for a user branch, or it
 * reads the branch number from the database.
 *
 * This function can be used to force the branch to a specific value.
 *
 * \param[in] branch  The branch you want to use with this page.
 */
void path_info_t::force_branch(snap_version::version_number_t branch)
{
    f_branch = branch;
    f_branch_key.clear();
}


void path_info_t::force_revision(snap_version::version_number_t revision)
{
    f_revision = revision;
    f_revision_key.clear();
}


void path_info_t::force_extended_revision(QString const & revision, QString const & filename)
{
    snap_version::version v;
    if(!v.set_version_string(revision))
    {
        throw snap_logic_exception(QString("invalid version string (%1) in \"%2\" (force_extended_revision).").arg(revision).arg(filename));
    }
    snap_version::version_numbers_vector_t const & version_numbers(v.get_version());
    if(version_numbers.size() < 1)
    {
        throw snap_logic_exception(QString("invalid version string (%1) in \"%2\" (force_extended_revision): not enough numbers (at least 1 required).").arg(revision).arg(filename));
    }
    force_branch(version_numbers[0]);
    force_revision(snap_version::SPECIAL_VERSION_EXTENDED);

    // WARNING: the revision string includes the branch
    f_revision_string = v.get_version_string();
}


/** \brief Set the locale to use in the revision key.
 *
 * Whenever you save a revision entry, it includes a specific language.
 * The locale defines the content of the revision as being in that
 * language.
 *
 * We support two special locales:
 *
 * \li "" -- the empty string represents a language agnostic revision.
 *     This could be a photo which no lettering.
 * \li "xx" -- the special "xx" language represents a neutral language.
 *     This means translation can still be created and will properly
 *     be distinguished. The "xx" revision is used when no other
 *     languages match the user's language.
 *
 * \param[in] locale  The locale two letter abbreviation.
 */
void path_info_t::force_locale(QString const & locale)
{
    // TBD: not too sure how valid this is...
    f_locale = locale;
}


/** \brief Defines the parent of an info path.
 *
 * The \p parent_ipath is set to the parent of 'this' path_info_t object.
 *
 * The parent of the root path is itself.
 *
 * \param[out] parent_ipath  The info path that will represent the parent.
 */
void path_info_t::get_parent(path_info_t & parent_ipath) const
{
    int const pos(f_cpath.lastIndexOf("/"));
    if(pos <= 0)
    {
        parent_ipath.set_path("");
    }
    else
    {
        // f_cpath is canonicalized so we can be sure there
        // aren't two // one after another; also cpath does
        // not include the domain name
        parent_ipath.set_path(f_cpath.mid(0, pos));
    }
}


/** \brief Create a path representing a child of this path.
 *
 * 'this' path is viewed as the parent path. This function uses the
 * parent path and appends the \p child string and saves the result
 * in the \p child_ipath parameter.
 *
 * \note
 * Notice that the function does not return anything because a path_info_t
 * cannot be copied. Instead we change the content of a parameter the user
 * passes in.
 *
 * \param[out] child_ipath  The resulting child path.
 * \param[in] child  The child to append to this path.
 */
void path_info_t::get_child(path_info_t & child_ipath, QString const & child) const
{
    // since the path will not include the domain name, it will get
    // canonicalized automatically
    child_ipath.set_path(f_cpath + "/" + child);
}


snap_child * path_info_t::get_snap() const
{
    return f_snap;
}


QString path_info_t::get_key() const
{
    return f_key;
}


QString path_info_t::get_real_key() const
{
    return f_real_key;
}


QString path_info_t::get_cpath() const
{
    return f_cpath;
}


snap_string_list path_info_t::get_segments() const
{
    if(!f_cpath.isEmpty()
    && f_segments.isEmpty())
    {
        f_segments = f_cpath.split('/');
    }
    return f_segments;
}


QString path_info_t::get_real_cpath() const
{
    return f_real_cpath;
}


bool path_info_t::is_main_page() const
{
    return f_main_page;
}


QString path_info_t::get_parameter(QString const & name) const
{
    return f_parameters.contains(name) ? f_parameters[name] : "";
}


/** \brief Retrieve the current status of this page.
 *
 * This function reads the raw status of the page. This is important when
 * more than one person access a website to avoid a certain amount of
 * conflicting processes (i.e. creating a page at the same time as you
 * delete that very page). It also very much helps the backend processes
 * which would otherwise attempt updates too early or too late.
 *
 * The status returned is any one of the status_t values, although the
 * general and working numbers may be mixed together (i.e. a page can
 * at the same time be hidden and updated.)
 *
 * The function may return a status with the path_info_t::status_t::UNDEFINED
 * error in which case the page does not exist at all. Note that this
 * function will not lie to you and say that the page does not exist just
 * because it is marked as deleted or some other similar valid status.
 * In that very case, the page simply is not defined in the
 * Cassandra database.
 *
 * The function may returned the special status named
 * SNAP_CONTENT_STATUS_UNSUPPORTED. When that happens, you cannot know what
 * to do with that very page because a more advanced Snap version is running
 * and marked the page with a status that you do not yet understand... In
 * that case, the best is for your function to return and not process the
 * page in any way.
 *
 * \important
 * Access to the status values make use the QUORUM consistency instead
 * of the default of ONE. This is to ensure that all instances see the
 * same/latest value saved in the database. This does NOT ensure 100%
 * consistency between various instances, however, it is not that likely
 * that two people would apply status changes to a page so simultaneously
 * that it would fail consistently (i.e. we do not use a lock to update
 * the status.) Note that if a Cassandra node is down, it is likely to
 * block the Snap! server as it has to wait on that one node (forever).
 * It will eventually time out, but most certainly after Apache already
 * said that the request could not be satisfied.
 *
 * \note
 * The status is not cached in the path_info_t object because (1) we could
 * have multiple path_info_t objects, each with its own status; and (2) the
 * libQtCassandra library has its own cache which is common to all the
 * path_info_t objects.
 *
 * \return The current raw status of the page.
 */
path_info_t::status_t path_info_t::get_status() const
{
    status_t    result;

    // verify that the page (row) exists, if not it was eradicated or
    // not yet created...
    QtCassandra::QCassandraTable::pointer_t content_table(f_content_plugin->get_content_table());
    if(!content_table->exists(f_key))
    {
        // the page does not exist
        result.set_error(status_t::error_t::UNDEFINED);
        return result;
    }

    // we set the consistency of the cell to QUORUM to make sure
    // we read the last written value
    QtCassandra::QCassandraCell::pointer_t cell(content_table->row(f_key)->cell(get_name(name_t::SNAP_NAME_CONTENT_STATUS)));
    cell->setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    QtCassandra::QCassandraValue value(cell->value());
    if(value.size() != sizeof(uint32_t))
    {
        // this case is legal, it happens when creating a new page
        result.set_error(status_t::error_t::UNDEFINED);
        return result;
    }

    // we have a status
    result.set_status(static_cast<status_t::status_type>(value.uint32Value()));

    return result;
}


/** \brief Change the current status of the page.
 *
 * This function can be used to change the status of the page from its
 * current status to a new status.
 *
 * The function re-reads the status first to make sure we can indeed
 * change the value. Then it verifies that the status can go from the
 * existing status to the new status. If not, we assume that the code
 * is wrong an thus raise an exception.
 *
 * The path_info_t object is not using RAII to handle the status cleanly
 * because these variables can be copied or duplicated and the status of
 * one variable could be misinterpreted. Therefore, any function that
 * changes the the status to a temporary state (SNAP_CONTENT_STATUS_CREATING,
 * SNAP_CONTENT_STATUS_CLONING, or SNAP_CONTENT_STATUS_REMOVING at time of
 * writing) should use RAII to make sure that the object gets a valid status
 * once the function is done dealing with the page.
 *
 * \important
 * Status values are using the QUORUM consistency instead of the default of
 * ONE. This is to ensure that all instances see the same/latest value
 * saved in the database. However, it blocks the Snap! server until the
 * write returns and that could be a problem, especially if a node is down.
 * Such a write will eventually time out.
 *
 * \bug
 * At this point the function expects the status to be properly managed
 * from the outside. That being said, status changes should only be handled
 * by functions defined in the content plugin and not functions from other
 * plugins. Yet, there is a problem where a page status may be set to a
 * value and not properly restored as expected later. When that occurs, the
 * database will "remember" the wrong status. We will need to have a way to
 * fix a website by going through all of the pages and making sure their
 * status is a currently working status. This is probably a job for the
 * content backend that also handles things like the trashcan.
 *
 * \exception content_exception_invalid_sequence
 * This exception is raised if the change in status is not valid. (i.e.
 * changing from status A to status B is not allowed.)
 *
 * \param[in] status  The new status for this page.
 */
void path_info_t::set_status(status_t const & status)
{
    // make sure it is not an error
    if(status.is_error())
    {
        throw content_exception_invalid_sequence(QString("changing page status to error %1 is not allowed, page \"%2\"")
                        .arg(static_cast<int>(status.get_status()))
                        .arg(f_key));
    }

    status_t now(get_status());

    if(!now.valid_transition(status))
    {
        throw content_exception_invalid_sequence(QString("changing page status from %1/%2 to %3/%4 is not supported, page \"%5\"")
                        .arg(static_cast<int>(now.get_state()))
                        .arg(static_cast<int>(now.get_working()))
                        .arg(static_cast<int>(status.get_state()))
                        .arg(static_cast<int>(status.get_working()))
                        .arg(f_key));
    }

    if(status.is_working())
    {
        QtCassandra::QCassandraTable::pointer_t processing_table(f_content_plugin->get_processing_table());
        signed char const one_byte(1);
        processing_table->row(f_key)->cell(get_name(name_t::SNAP_NAME_CONTENT_STATUS_CHANGED))->setValue(one_byte);
    }
    QtCassandra::QCassandraTable::pointer_t content_table(f_content_plugin->get_content_table());

    // we use QUORUM in the consistency level to make sure that information
    // is available on all Cassandra nodes all at once
    //
    // we save the date when we changed the status so that way we know whether
    // the process when to lala land or is still working on the status; a
    // backend is responsible for fixing "invalid" status (i.e. after 10 min.
    // a status is reset back to something like DELETED or HIDDEN if not
    // otherwise considered valid.)
    QtCassandra::QCassandraValue changed;
    int64_t const start_date(f_snap->get_start_date());
    changed.setInt64Value(start_date);
    changed.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    content_table->row(f_key)->cell(get_name(name_t::SNAP_NAME_CONTENT_STATUS_CHANGED))->setValue(changed);

    QtCassandra::QCassandraValue value;
    value.setUInt32Value(status.get_status());
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    content_table->row(f_key)->cell(get_name(name_t::SNAP_NAME_CONTENT_STATUS))->setValue(value);
}


bool path_info_t::get_working_branch() const
{
    return f_main_page ? f_snap->get_working_branch() : false;
}


snap_version::version_number_t path_info_t::get_branch(bool create_new_if_required, QString const & locale) const
{
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
    {
        f_branch = f_main_page
                    ? f_snap->get_branch()
                    : static_cast<snap_version::version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED);

        if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
        {
            QString const & key(f_real_key.isEmpty() ? f_key : f_real_key);
            f_branch = f_content_plugin->get_current_branch(key, get_working_branch());
            if(create_new_if_required
            && snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
            {
                f_locale = locale;
                f_branch = f_content_plugin->get_new_branch(key, f_locale);
            }
        }
    }

    return f_branch;
}


bool path_info_t::has_branch() const
{
    return snap_version::SPECIAL_VERSION_UNDEFINED != get_branch();
}


snap_version::version_number_t path_info_t::get_revision() const
{
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision
    || snap_version::SPECIAL_VERSION_INVALID == f_revision)
    {
        // check all available revisions and return the first valid one,
        // however, if the user specified a revision (as we get with the
        // f_snap->get_revision() function) then we use that one no matter
        // what... if f_revision is defined and f_revision_key is empty
        // that means we have an invalid user revision and it will get caught
        // at some point.

        // make sure the branch is defined
        if(!has_branch())
        {
            // no branch implies no revision...
            return f_revision;
        }

        // reset values
        f_revision = f_main_page
                    ? f_snap->get_revision()
                    : static_cast<snap_version::version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED);

        // TODO if user did not specify the locale, we still have a chance
        //      to find out which locale to use -- at this point the following
        //      does not properly handle the case where the locale was not
        //      specified in the URI
        f_locale = f_snap->get_language_key();
        QString default_language(f_locale);
        f_revision_key.clear();

        if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision)
        {
            QString const& key(f_real_key.isEmpty() ? f_key : f_real_key);

            // try with the full locale
            f_revision = f_content_plugin->get_current_revision(key, f_branch, f_locale, get_working_branch());
            if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision && f_locale.length() == 5)
            {
                // try without the country
                f_locale = f_locale.left(2);
                f_revision = f_content_plugin->get_current_revision(key, f_branch, f_locale, get_working_branch());
            }
            if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision)
            {
                // try with the neutral language
                f_locale = "xx";
                f_revision = f_content_plugin->get_current_revision(key, f_branch, f_locale, get_working_branch());
            }
            if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision)
            {
                // try without a language
                f_locale.clear();
                f_revision = f_content_plugin->get_current_revision(key, f_branch, f_locale, get_working_branch());
            }
            if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision
            && default_language.left(2) != "en")
            {
                // try an "internal" default language as a last resort...
                f_revision = f_content_plugin->get_current_revision(key, f_branch, "en", get_working_branch());
                if(snap_version::SPECIAL_VERSION_UNDEFINED != f_revision)
                {
                    f_locale = "en";
                }
            }
        }
    }

    return f_revision;
}


/** \brief Check whether a revision is defined for that path.
 *
 * This function checks for a revision number for that path.
 *
 * Note that this function may return false when the get_revision_key()
 * function may return a valid key. This is because the revision key
 * may create a new key or make use of some other heuristic to define
 * a key.
 *
 * \return true if the revision is defined.
 */
bool path_info_t::has_revision() const
{
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision
    || snap_version::SPECIAL_VERSION_INVALID == f_revision)
    {
        get_revision();
        return snap_version::SPECIAL_VERSION_UNDEFINED != f_revision
            && snap_version::SPECIAL_VERSION_INVALID != f_revision;
    }

    return true;
}


QString path_info_t::get_locale() const
{
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision
    || snap_version::SPECIAL_VERSION_INVALID == f_revision)
    {
        get_revision();
    }
    return f_locale;
}


QString path_info_t::get_branch_key() const
{
    // if f_branch is still undefined, get it from the database
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
    {
        get_branch();
    }

// avoid the warning about the minimum being zero and thus the useless test
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
    if(snap_version::SPECIAL_VERSION_MIN > f_branch
    || snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER < f_branch)
#pragma GCC diagnostic pop
    {
        // the branch is still undefined...
        throw content_exception_data_missing(QString("get_branch_key() request failed for \"%1\", branch not defined (%2)").arg(f_cpath).arg(f_branch));
    }

    if(f_branch_key.isEmpty())
    {
        f_branch_key = f_content_plugin->generate_branch_key(f_key, f_branch);
    }
    return f_branch_key;
}


QString path_info_t::get_revision_key() const
{
    if(f_revision_key.isEmpty())
    {
        if(snap_version::SPECIAL_VERSION_EXTENDED == f_revision)
        {
            // if f_revision is set to "extended" then the branch is
            // not included as a separate number; it is directly part
            // of the revision string
            //
            // this is currently used for .js and .css files
            //
            f_revision_key = f_content_plugin->generate_revision_key(f_key, f_revision_string, f_locale);
        }
        else
        {
            if(snap_version::SPECIAL_VERSION_UNDEFINED == f_revision
            || snap_version::SPECIAL_VERSION_INVALID == f_revision)
            {
                get_revision();
            }

            // name of the field in the content table of that page
            QString field(QString("%1::%2::%3")
                        .arg(get_name(name_t::SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(get_working_branch()
                                ? name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                                : name_t::SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                        .arg(f_branch));
            if(!f_locale.isEmpty())
            {
                field += "::" + f_locale;
            }

            QtCassandra::QCassandraTable::pointer_t content_table(f_content_plugin->get_content_table());
            if(content_table->exists(f_key)
            && content_table->row(f_key)->exists(field))
            {
                QtCassandra::QCassandraValue value(content_table->row(f_key)->cell(field)->value());
                f_revision_key = value.stringValue();
            }
            // else -- no default revision...

            if(f_revision_key.isEmpty())
            {
                // the revision is still undefined...
                throw content_exception_data_missing(QString("get_revision_key() request failed for \"%1\", revision for \"%2\" not defined.").arg(f_cpath).arg(f_key));
            }
        }
    }

    return f_revision_key;
}


QString path_info_t::get_extended_revision() const
{
    return f_revision_string;
}


QString path_info_t::get_draft_key(int64_t user_identifier) const
{
    if(f_draft_key.isEmpty())
    {
        // make sure the branch is defined
        if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
        {
            // when create_new_if_required is set to false, then the locale
            // parameter is never used; i.e. a draft cannot be created if
            // we have no branch
            get_branch(false, "");
        }

// avoid the warning about the minimum being zero and thus the useless test
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
        if(snap_version::SPECIAL_VERSION_MIN > f_branch
        || snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER < f_branch)
#pragma GCC diagnostic pop
        {
            // the branch is still undefined...
            throw content_exception_data_missing(QString("get_draft_key() request failed for \"%1\", no branch defined").arg(f_cpath));
        }

        f_draft_key = QString("%1#user/%2/%3").arg(f_key).arg(user_identifier).arg(f_branch);
    }

    return f_draft_key;
}


QString path_info_t::get_suggestion_key(int64_t suggestion) const
{
    if(f_suggestion_key.isEmpty())
    {
        // make sure the branch is defined
        if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
        {
            // when create_new_if_required is set to false, then the locale
            // parameter is never used; i.e. a draft cannot be created if
            // we have no branch
            get_branch(false, "");
        }

// avoid the warning about the minimum being zero and thus the useless test
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
        if(snap_version::SPECIAL_VERSION_MIN > f_branch
        || snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER < f_branch)
#pragma GCC diagnostic pop
        {
            // the branch is still undefined...
            throw content_exception_data_missing(QString("get_suggestion_key() request failed for \"%1\", no branch defined").arg(f_cpath));
        }

        f_suggestion_key = QString("%1#suggestion/%2.%3").arg(f_key).arg(f_branch).arg(suggestion);
    }

    return f_suggestion_key;
}


void path_info_t::clear(bool keep_parameters)
{
    f_branch = snap_version::SPECIAL_VERSION_UNDEFINED;
    f_revision = snap_version::SPECIAL_VERSION_UNDEFINED;
    f_revision_string.clear();
    f_locale.clear();
    f_branch_key.clear();
    f_revision_key.clear();
    f_segments.clear();

    // in case of a set_real_path() we do not want to lose the parameters
    if(!keep_parameters)
    {
        f_parameters.clear();
    }
}



SNAP_PLUGIN_EXTENSION_END()

// vim: ts=4 sw=4 et

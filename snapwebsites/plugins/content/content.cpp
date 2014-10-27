// Snap Websites Server -- all the user content and much of the system content
// Copyright (C) 2011-2014  Made to Order Software Corp.
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
 * \brief The implementation of the content plugin.
 *
 * The implementation of the content plugin handles the content, branch, and
 * revision tables in a way that gives other plugins access to all the data
 * without themselves having to directly peek and poke at the data.
 *
 * This allows the content plugin a way to control that modified data does
 * general all the necessary "side effects" as expected in the system. The
 * main problem we have when modifying one or more fields in a propagation
 * of the information. By using the path_info_t and the content plugin to
 * make all data changes we ensure that the related signals get emitted
 * and thus that all plugins get a chance to do further updates as they
 * require to make to finish up the work (i.e. when changing a title and
 * that page is part of a list which shows that title, we want the list
 * plugin to kick in and fix the corresponding list.)
 */

#include "content.h"

//#include "../messages/messages.h" -- we now have 2 levels (messages and output) so we could include messages.h here

#include "plugins.h"
#include "log.h"
#include "compression.h"
#include "not_reached.h"
#include "dom_util.h"
#include "dbutils.h"
#include "snap_magic.h"
#include "snap_image.h"
#include "snap_version.h"
#include "qdomhelpers.h"

#include <QtCassandra/QCassandraLock.h>

#include <iostream>

#include <openssl/md5.h>

#include <QFile>
#include <QUuid>
#include <QTextStream>

#include "poison.h"


SNAP_PLUGIN_START(content, 1, 0)

/** \brief Get a fixed content name.
 *
 * The content plugin makes use of different names in the database. This
 * function ensures that you get the right spelling for a given name.
 *
 * \param[in] name  The name to retrieve.
 *
 * \return A pointer to the name.
 */
char const *get_name(name_t name)
{
    // Note: <branch>.<revision> are actually replaced by a full version
    //       when dealing with JavaScript and CSS files (Version: field)
    switch(name)
    {
    case SNAP_NAME_CONTENT_ACCEPTED:
        return "content::accepted";

    case SNAP_NAME_CONTENT_ATTACHMENT:
        return "content::attachment";

    case SNAP_NAME_CONTENT_ATTACHMENT_FILENAME:
        return "content::attachment::filename";

    case SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE:
        return "content::attachment::mime_type";

    case SNAP_NAME_CONTENT_ATTACHMENT_PATH_END:
        return "path";

    case SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN: // this is a forward definition...
        return "attachment";

    case SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE:
        return "content::attachment::reference";

    case SNAP_NAME_CONTENT_BODY:
        return "content::body";

    case SNAP_NAME_CONTENT_BRANCH:
        return "content::branch";

    case SNAP_NAME_CONTENT_BRANCH_TABLE:
        return "branch";

    case SNAP_NAME_CONTENT_CHILDREN:
        return "content::children";

    case SNAP_NAME_CONTENT_CLONE:
        return "content::clone";

    case SNAP_NAME_CONTENT_CLONED:
        return "content::cloned";

    case SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED:
        return "uncompressed";

    case SNAP_NAME_CONTENT_CONTENT_TYPES:
        return "Content Types";

    case SNAP_NAME_CONTENT_CONTENT_TYPES_NAME:
        return "content-types";

    case SNAP_NAME_CONTENT_COPYRIGHTED:
        return "content::copyrighted";

    case SNAP_NAME_CONTENT_CREATED:
        return "content::created";

    case SNAP_NAME_CONTENT_ERROR_FILES:
        return "error_files";

    case SNAP_NAME_CONTENT_DESCRIPTION:
        return "content::description";

    case SNAP_NAME_CONTENT_FILES_COMPRESSOR:
        return "content::files::compressor";

    case SNAP_NAME_CONTENT_FILES_CREATED:
        return "content::files::created";

    case SNAP_NAME_CONTENT_FILES_CREATION_TIME:
        return "content::files::creation_time";

    case SNAP_NAME_CONTENT_FILES_DATA:
        return "content::files::data";

    case SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED:
        return "content::files::data::gzip_compressed";

    case SNAP_NAME_CONTENT_FILES_DEPENDENCY:
        return "content::files::dependency";

    case SNAP_NAME_CONTENT_FILES_FILENAME:
        return "content::files::filename";

    case SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT:
        return "content::files::image_height";

    case SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH:
        return "content::files::image_width";

    case SNAP_NAME_CONTENT_FILES_MIME_TYPE:
        return "content::files::mime_type";

    case SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME:
        return "content::files::modification_time";

    case SNAP_NAME_CONTENT_FILES_NEW:
        return "new";

    case SNAP_NAME_CONTENT_FILES_REFERENCE:
        return "content::files::reference";

    case SNAP_NAME_CONTENT_RESETSTATUS:
        return "resetstatus";

    case SNAP_NAME_CONTENT_FILES_SECURE: // -1 -- unknown, 0 -- unsecure, 1 -- secure
        return "content::files::secure";

    case SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK:
        return "content::files::secure::last_check";

    case SNAP_NAME_CONTENT_FILES_SECURITY_REASON:
        return "content::files::security_reason";

    case SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE:
        return "content::files::original_mime_type";

    case SNAP_NAME_CONTENT_FILES_SIZE:
        return "content::files::size";

    case SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED:
        return "content::files::size::gzip_compressed";

    case SNAP_NAME_CONTENT_FILES_TABLE:
        return "files";

    case SNAP_NAME_CONTENT_FILES_UPDATED:
        return "content::files::updated";

    case SNAP_NAME_CONTENT_FINAL:
        return "content::final";

    case SNAP_NAME_CONTENT_ISSUED:
        return "content::issued";

    case SNAP_NAME_CONTENT_LONG_TITLE:
        return "content::long_title";

    case SNAP_NAME_CONTENT_MINIMAL_LAYOUT_NAME:
        return "notheme";

    case SNAP_NAME_CONTENT_MODIFIED:
        return "content::modified";

    case SNAP_NAME_CONTENT_ORIGINAL_PAGE:
        return "content::original_page";

    case SNAP_NAME_CONTENT_OUTPUT_PLUGIN: // this a forward declaration of the name of the "output" plugin...
        return "output";

    case SNAP_NAME_CONTENT_PAGE:
        return "content::page";

    case SNAP_NAME_CONTENT_PAGE_TYPE:
        return "content::page_type";

    case SNAP_NAME_CONTENT_PARENT:
        return "content::parent";

    case SNAP_NAME_CONTENT_PREVENT_DELETE:
        return "content::prevent_delete";

    case SNAP_NAME_CONTENT_PRIMARY_OWNER:
        return "content::primary_owner";

    case SNAP_NAME_CONTENT_PROCESSING_TABLE:
        return "processing";

    case SNAP_NAME_CONTENT_REVISION_CONTROL: // content::revision_control::...
        return "content::revision_control";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH: // content::revision_control::current_branch [uint32_t]
        return "current_branch";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY: // content::revision_control::current_branch_key [string]
        return "current_branch_key";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION: // content::revision_control::current_revision::<branch>::<locale> [uint32_t]
        return "current_revision";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY: // content::revision_control::current_revision_key::<branch>::<locale> [string]
        return "current_revision_key";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH: // content::revision_control::current_working_branch [uint32_t]
        return "current_working_branch";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY: // content::revision_control::current_working_branch_key [string]
        return "current_working_branch_key";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION: // content::revision_control::current_working_revision::<branch>::<locale> [uint32_t]
        return "current_working_revision";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY: // content::revision_control::current_working_revision_key::<branch>::<locale> [string]
        return "current_working_revision_key";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH: // content::revision_control::last_branch [uint32_t]
        return "last_branch";

    case SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION: // content::revision_control::last_revision::<branch>::<locale> [uint32_t]
        return "last_revision";

    case SNAP_NAME_CONTENT_REVISION_TABLE:
        return "revision";

    case SNAP_NAME_CONTENT_SHORT_TITLE:
        return "content::short_title";

    case SNAP_NAME_CONTENT_SINCE:
        return "content::since";

    case SNAP_NAME_CONTENT_STATUS:
        return "content::status";

    case SNAP_NAME_CONTENT_STATUS_CHANGED:
        return "content::status_changed";

    case SNAP_NAME_CONTENT_SUBMITTED:
        return "content::submitted";

    case SNAP_NAME_CONTENT_TABLE: // pages, tags, comments, etc.
        return "content";

    case SNAP_NAME_CONTENT_TAG:
        return "content";

    case SNAP_NAME_CONTENT_TITLE:
        return "content::title";

    case SNAP_NAME_CONTENT_TRASHCAN:
        return "content::trashcan";

    case SNAP_NAME_CONTENT_UNTIL:
        return "content::until";

    case SNAP_NAME_CONTENT_UPDATED:
        return "content::updated";

    case SNAP_NAME_CONTENT_VARIABLE_REVISION:
        return "revision";

    default:
        // invalid index
        throw snap_logic_exception("invalid SNAP_NAME_CONTENT_...");

    }
    NOTREACHED();
}



namespace
{

/** \brief Extensions we accept as JavaScript file extensions.
 *
 * This table lists JavaScript extensions that we understand as
 * acceptable JavaScript extensions. This table is used to make
 * sure JavaScript files get added to the right place when
 * uploaded to the website.
 */
char const *js_extensions[] =
{
    // longer first
    ".min.js",
    ".org.js",
    ".js",
    nullptr
};

char const *css_extensions[] =
{
    // longer first
    ".min.css",
    ".org.css",
    ".css",
    nullptr
};

} // no name namespace


/** \typedef uint32_t path_info_t::status_t::status_type
 * \brief Basic status type to save the status in the database.
 *
 * This basic status is used by the content plugin to manage a page
 * availability. It is called "basic" because this feature does not
 * use the taxonomy to mark the page as being in a specific status
 * that the end user has control over.
 *
 * By default a page is in the "normal state"
 * (path_info_t::status_t::NORMAL). A normal page can be viewed as
 * fully available and will be shown to anyone with enough permissions
 * to access that page.
 *
 * A page can also be hidden from view (path_info_t::status_t::HIDDEN),
 * in which case the page is accessible by the administrators with enough
 * permissions to see hidden pages, but no one else who get an error
 * (probably a 404, although if the hidden page is to be shown again
 * later a 503 is probably more appropriate.)
 *
 * Finally, a page can be given a working status:
 *
 * \li path_info_t::status_t::NOT_WORKING -- no processes are working on
 *     the page
 * \li path_info_t::status_t::CREATING -- the page is being created
 * \li path_info_t::status_t::CLONING -- the page is being cloned from
 *     another page
 * \li path_info_t::status_t::REMOVING -- the page is being moved or deleted
 * \li path_info_t::status_t::UPDATING -- the page is being updated
 *
 * These states are used in parallel with the basic state of the page.
 * So a page can be normal and updating at the same time. This is useful
 * in order to allow a page to revert back to a standard state (i.e. not
 * being processed) without having to have many more states making it
 * much harder to handle.
 *
 * The status_t class gives you two sets of functions to handle the state
 * and the working state separatly. There is also a common function,
 * reset_state(), which modifies both values at the same time.
 *
 * Note that a deleted page (path_info_t::status_t::DELETED) is similar
 * to a normal page, only it is found in the trashcan and thus it cannot
 * be edited. It can only be "undeleted" (cloned back to its original
 * location or to a new location in the regular tree.)
 */

/** \var path_info_t::status_t::NO_ERROR
 * \brief No error occured.
 *
 * When creating a new status object, we mark it as a "no error" object.
 *
 * In this state a status can be saved to the database. If not in this
 * state, trying to save the status will fail with an exception.
 */

/** \var path_info_t::status_t::UNSUPPORTED
 * \brief Read a status that this version does not know about.
 *
 * This value is returned by the get_status() function whenever a path
 * to a page returns a number that the current status implementation does
 * not understand. Unfortunately, such status cannot really be dealt with
 * otherwise.
 */

/** \var path_info_t::status_t::UNDEFINED
 * \brief The state is not defined in the database.
 *
 * This value is returned by the get_status() function whenever a path
 * to a non-existant page is read.
 *
 * This is similar to saying this is a 404. There is no redirect or anything
 * else that will help in this circumstance.
 */

/** \var path_info_t::status_t::UNKNOWN_STATE
 * \brief The state was not yet defined.
 *
 * This value is used internally to indicate that the status was not yet
 * read from the database. It should never be saved in the database itself.
 *
 * This is used in the status_t class up until the status gets read from
 * the content table.
 */

/** \var path_info_t::status_t::CREATE
 * \brief We are in the process of creating a page.
 *
 * While creating a page, the page is marked with this state.
 *
 * Once the page is created, it is marked as path_info_t::status_t::NORMAL.
 */

/** \var path_info_t::status_t::NORMAL
 * \brief This page is valid. You can use it as is.
 *
 * This is the only status that makes a page 100% valid for anyone with
 * enough permissions to visit the page.
 */

/** \var path_info_t::status_t::HIDDEN
 * \brief The page is currently hidden.
 *
 * A hidden page is similar to a normal page, only it returns a 404 to
 * normal users.
 *
 * Only administrators with the correct permissions can see the page.
 */

/** \var path_info_t::status_t::MOVED
 * \brief This page was moved, users coming here shall be redirected.
 *
 * This page content is still intact from the time it was cloned and it
 * should not be used. Instead, since it is considered moved, it generates
 * a 301 (it could be made a 302?) so that way the users who had links to
 * the old path still get to the page.
 *
 * A moved page may get deleted at a later time.
 */

/** \var path_info_t::status_t::DELETED
 * \brief This page was deleted (moved to the trash).
 *
 * A page that gets moved to the trashcan is marked as deleted since we
 * cannot redirect someone (other than an administrator with enough
 * permissions) to the trashcan.
 *
 * Someone with enough permission can restore a deleted page.
 *
 * A page marked as deleted is eventually removed from the database by
 * the content backend. Pages in the trashcan are also eventually deleted
 * from the database. That depends on the trashcan policy settings.
 */

/** \var path_info_t::status_t::NOT_WORKING
 * \brief Indicate that no processes are working on this page.
 *
 * This value indicates that the page is not being worked on. In most cases
 * backend processes use that signal to know whether to process a page
 * or not for a reason or another. For example, the list plugin will
 * avoid including pages in a list while those pages are being
 * created or updated. It will keep those pages in its list of pages
 * to be processed later on instead.
 */

/** \var path_info_t::status_t::CREATING
 * \brief Working on a page while creating it.
 *
 * This working value is used to mark a page being created. In a way, this
 * working state is a plain state too (we use CREATE/CREATING and then
 * transform that in NORMAL/NOT_WORKING).
 */

/** \var path_info_t::status_t::CLONING
 *
 * This status is similar to the creating status
 * (path_info_t::status_t::CREATING) only the data comes from another page
 * instead of the user.
 *
 * You have similar restriction on a page being cloned as a page being
 * created. While this status is set, someone visiting the page can only
 * get a signal such as "server busy".
 *
 * Once the cloning is done, the page can go to the normal state.
 */

/** \var path_info_t::status_t::REMOVING
 *
 * This status is used to mark the source page in a cloning process as
 * the page is going to be removed (i.e. the page is being moved to the
 * trashcan).
 *
 * If the page is simply being moved, then the status can remain normal
 * (path_info_t::status_t::NORMAL) since the source remains perfectly valid
 * while the page gets cloned. Once the cloning is done then the page is
 * marked as moved (path_info_t::status_t::MOVED).
 *
 * Once the remove process is done, the page gets marked as deleted
 * (path_info_t::status_t::DELETED). Remember that deleted pages return
 * a 404 to the client even though all the data is still available in
 * the database.
 */

/** \var path_info_t::status_t::UPDATING
 *
 * A page that gets heavily updated (more than one or two fields in a row)
 * should be marked as path_info_t::status_t::UPDATING. However, you want
 * to be careful as a page current status should not change once the update
 * is done (i.e. if the page was hidden then reverting it back to hidden
 * after the update is what you should do; so if you change that to normal
 * instead, you are in trouble.)
 */

/** \var path_info_t::status_t::f_error
 * \brief The current error of this status object.
 *
 * The error of this status. By default this parameter is set to
 * path_info_t::status_t::NO_ERROR.
 *
 * When a status is erroneous, the is_error() function returns true and
 * the status cannot be saved in the database.
 *
 * The state and working state of the status are ignored if
 * the status is in error (is_error() returns true.)
 *
 * There is one special case the transition function accepts a
 * path_info_t::status_t::UNDEFINED status as a valid input to
 * transit to a path_info_t::status_t::CREATE and
 * path_info_t::status_t::CREATING status. However, the erroneous
 * status itself is otherwise still considered to be in error.
 */

/** \var path_info_t::status_t::f_state
 * \brief The current state of the status.
 *
 * The state of this status. By default this parameter is set to
 * path_info_t::status_t::UNKNOWN_STATE. You may check whether the
 * state is unknown using the is_unknown() function.
 *
 * \warning
 * The working state is ignored if is_error() is true.
 */

/** \var path_info_t::status_t::f_working
 * \brief The current working state of the status.
 *
 * The status of a page may include a working state which represents what
 * the process working on the page is doing. By default this parameter is
 * set to path_info_t::status_t::NOT_WORKING.
 *
 * When a process is working on a page, its status is_working() function
 * returns true.
 *
 * \warning
 * The working state is ignored if is_error() is true.
 */

/** \typedef path_info_t::status_t::safe_error_t
 * \brief Safe as in auto-initialized error_t variable type.
 *
 * This typedef is used to define error_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the error data saved into them when modifying them.
 */

/** \typedef path_info_t::status_t::safe_state_t
 * \brief Safe as in auto-initialized state_t variable type.
 *
 * This typedef is used to define state_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the state data saved into them when modifying them.
 */

/** \typedef path_info_t::status_t::safe_working_t
 * \brief Safe as in auto-initialized working_t variable type.
 *
 * This typedef is used to define working_t variable members in classes
 * so they automatically get initialized and they test the range of
 * the working data saved into them when modifying them.
 */





/** \class field_search
 * \brief Retrieve one or more parameters from one or more path.
 *
 * This function is used to search for a parameter in one or more paths
 * in your existing database tree.
 *
 * In many cases, the parameter exists in the specified path (i.e. the
 * "modified" parameter). In some other cases, the parameter only
 * exists in a child, a parent, the template, or a settings page.
 * This function is very easy to use and it will return said parameter
 * from wherever it is first found.
 *
 * If you are creating an administrative screen (and in some other
 * circumstances) it may be useful to find all instances of the parameter.
 * In that case you can request all instances. Note that this case is
 * considered SLOW and it should not be used lightly while generating
 * a page!
 *
 * The following shows you an example of a tree that this function can
 * search. Say that the input path represents B. If your search setup
 * asks for SELF, its CHILDREN with a depth limit of 2, a template (assuming
 * its template is D,) its type found using LINK (and assuming its type is
 * F) and the PARENTS of that type with a limit on C then the search can
 * check the following nodes in that order:
 *
 * \li B
 * \li E (switched to children)
 * \li H (switched to children; last time because depth is limited to 2)
 * \li I
 * \li J
 * \li D (switched to template)
 * \li F (switched to page type)
 * \li C (switched to parent, stop on C)
 *
 * Pages A, K and G are therefore ignored.
 *
 *                +-------+       +------+       +-------+
 *          +---->| B     |+----->| E    |+-+--->| H     |
 *          |     +-------+       +------+  |    +-------+
 *          |                               |
 *          |                               |
 *          |                     +------+  |    +-------+     +------+
 *          |     +-------+  +--->| F    |  +--->| I     |+--->| K    |
 *          +---->| C     |+-+    +------+  |    +-------+     +------+
 *  +----+  |     +-------+  |              |
 *  | A  |+-+                |              |
 *  +----+  |                |    +------+  |
 *          |                +--->| G    |  |    +-------+
 *          |     +-------+       +------+  +--->| J     |
 *          +---->| D     |                      +-------+
 *                +-------+
 *
 * View: http://www.asciiflow.com/#1357940162213390220
 * Edit: http://www.asciiflow.com/#1357940162213390220/1819073096
 *
 * This type of search can be used to gather pretty much all the
 * necessary parameters used in a page to display that page.
 *
 * Note that this function is not used by the permissions because in
 * that case all permission links defined in a page are sought. Whereas
 * here we're interested in the content of a field in a page.
 *
 * Note that when searching children we first search all the children at
 * a given depth, then repeat the search at the next level. So in our
 * example, if we had a search depth of 3, we would end up searching
 * K after J, not between I and J.
 *
 * Since the cmd_info_t object is like a mini program, it is possible
 * to do things such as change the name of the field being sought as
 * the different parts of the tree are searched. So a parameter named
 * "created" in SELF, could change to "modified" when searching the
 * PARENT, and "primary-date" when searching the TYPE. It may, however,
 * not be a good idea as in most situations you probably want to use
 * just and only "modified". This being said, when you try to determine
 * the modification date, you could try the "modified" date first, then
 * try the "updated" and finally "created" and since "created" is
 * mandatory you know you'll always find it (and it not, there is no
 * other valid default):
 *
 * \code
 * QStringList result(cmd_info_t()
 *      (PARAMETER_OPTION_FIELD_NAME, "modified")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "updated")
 *      (PARAMETER_OPTION_SELF, path)
 *      (PARAMETER_OPTION_FIELD_NAME, "created")
 *      (PARAMETER_OPTION_SELF, path)
 *      .run(PARAMETER_OPTION_MODE_FIRST)); // run
 * \endcode
 *
 * In this example notice that we just lose the cmd_info_t object. It is
 * temporarily created on the stack, initialized, used to gather the
 * first match, then return a list of strings our of which we expect
 * either nothing (empty list) or one entry (the first parameter found.)
 */


/** \class cmd_info_t
 * \brief Instructions about the search to perform.
 *
 * This sub-class is used by the parameters_t class as an instruction:
 * what to search next to find a given parameter.
 */


/** \brief Create an empty cmd_info_t object.
 *
 * To be able to create cmd_info_t objects in a vector we have to create
 * a constructor with no parameters. This creates an invalid command
 * object.
 */
field_search::cmd_info_t::cmd_info_t()
    //: f_cmd(COMMAND_UNKNOWN) -- auto-init
    //, f_value() -- auto-init
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd)
    : f_cmd(static_cast<int>(cmd)) // FIXME fix cast
    //, f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_PARENT_ELEMENT:
    case COMMAND_ELEMENT_TEXT:
    case COMMAND_RESET:
    case COMMAND_SELF:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an instruction without parameters").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] str_value  The string value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QString const& str_value)
    : f_cmd(static_cast<int>(cmd)) // FIXME fix cast
    , f_value(str_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_FIELD_NAME:
    case COMMAND_PATH:
    case COMMAND_PARENTS:
    case COMMAND_LINK:
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
    case COMMAND_PATH_ELEMENT:
    case COMMAND_CHILD_ELEMENT:
    case COMMAND_NEW_CHILD_ELEMENT:
    case COMMAND_ELEMENT_ATTR:
    case COMMAND_SAVE:
    case COMMAND_SAVE_INT64:
    case COMMAND_SAVE_INT64_DATE:
    case COMMAND_SAVE_XML:
    case COMMAND_WARNING:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a string (%2)").arg(static_cast<int>(cmd)).arg(str_value));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] int_value  The integer value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, int64_t int_value)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    , f_value(int_value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_MODE:
    case COMMAND_CHILDREN:
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
    case COMMAND_LABEL:
    case COMMAND_GOTO:
    case COMMAND_IF_FOUND:
    case COMMAND_IF_NOT_FOUND:
    case COMMAND_IF_ELEMENT_NULL:
    case COMMAND_IF_NOT_ELEMENT_NULL:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a string (%2)").arg(static_cast<int>(cmd)).arg(int_value));

    }
}


/** \brief Initialize a cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] value  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QtCassandra::QCassandraValue& value)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    , f_value(value)
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_DEFAULT_VALUE:
    case COMMAND_DEFAULT_VALUE_OR_NULL:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] element  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QDomElement element)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    , f_element(element)
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_ELEMENT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize a cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in] doc  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, QDomDocument doc)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    , f_element(doc.documentElement())
    //, f_result(nullptr) -- auto-init
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_ELEMENT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a QCassandraValue").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. SELF, PARENTS, etc.)
 * \param[in,out] result  The value attached to that instruction.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, search_result_t& result)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    //, f_element(element)
    , f_result(&result)
    //, f_path_info() -- auto-init
{
    switch(cmd)
    {
    case COMMAND_RESULT:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for a search_result_t").arg(static_cast<int>(cmd)));

    }
}


/** \brief Initialize an cmd_info_t object.
 *
 * This function initializes the cmd_info_t. Note that the parameters
 * cannot be changed later (read-only.)
 *
 * \param[in] cmd  The search instruction (i.e. COMMAND_PATH_INFO, etc.)
 * \param[in] ipath  A full defined path to a page.
 */
field_search::cmd_info_t::cmd_info_t(command_t cmd, path_info_t const& ipath)
    : f_cmd(static_cast<int>(cmd)) // XXX fix cast
    //, f_value() -- auto-init
    //, f_element() -- auto-init
    //, f_result(nullptr) -- auto-init
    , f_path_info(ipath)
{
    switch(cmd)
    {
    case COMMAND_PATH_INFO_GLOBAL:
    case COMMAND_PATH_INFO_BRANCH:
    case COMMAND_PATH_INFO_REVISION:
        break;

    default:
        throw content_exception_type_mismatch(QString("invalid parameter option (command %1) for an ipath (%2)").arg(static_cast<int>(cmd)).arg(ipath.get_cpath()));

    }
}


/** \brief Initialize a field search object.
 *
 * This constructor saves the snap child pointer in the field_search so
 * it can be referenced later to access pages.
 */
field_search::field_search(char const *filename, char const *func, int line, snap_child *snap)
    : f_filename(filename)
    , f_function(func)
    , f_line(line)
    , f_snap(snap)
    //, f_program() -- auto-init
{
}


/** \brief Generate the data and then destroy the field_search object.
 *
 * The destructor makes sure that the program runs once, then it cleans
 * up the object. This allows you to create a tempoary field_search object
 * on the stack and at the time it gets deleted, it runs the program.
 */
field_search::~field_search()
{
    run();
}


/** \brief Add a command with no parameter.
 *
 * The following commands support this scheme:
 *
 * \li COMMAND_PARENT_ELEMENT
 * \li COMMAND_ELEMENT_TEXT
 * \li COMMAND_RESET
 * \li COMMAND_SELF
 *
 * \param[in] cmd  The command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd)
{
    cmd_info_t inst(cmd);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a "char const *".
 *
 * The following commands support the "char const *" value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_PATH_ELEMENT
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_NEW_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 * \li COMMAND_SAVE
 * \li COMMAND_SAVE_INT64
 * \li COMMAND_SAVE_INT64_DATE
 * \li COMMAND_SAVE_XML
 * \li COMMAND_WARNING
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, char const *str_value)
{
    cmd_info_t inst(cmd, QString(str_value));
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QString.
 *
 * The following commands support the QString value:
 *
 * \li COMMAND_FIELD_NAME
 * \li COMMAND_PATH
 * \li COMMAND_PARENTS
 * \li COMMAND_LINK
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_PATH_ELEMENT
 * \li COMMAND_CHILD_ELEMENT
 * \li COMMAND_NEW_CHILD_ELEMENT
 * \li COMMAND_ELEMENT_ATTR
 *
 * \param[in] cmd  The command.
 * \param[in] str_value  The string attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QString const& str_value)
{
    cmd_info_t inst(cmd, str_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a 64 bit integer.
 *
 * The following commands support the integer:
 *
 * \li COMMAND_CHILDREN
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 * \li COMMAND_LABEL
 * \li COMMAND_GOTO
 * \li COMMAND_IF_FOUND
 * \li COMMAND_IF_NOT_FOUND
 * \li COMMAND_IF_ELEMENT_NULL
 * \li COMMAND_IF_NOT_ELEMENT_NULL
 *
 * \param[in] cmd  The command.
 * \param[in] value  The integer attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, int64_t int_value)
{
    cmd_info_t inst(cmd, int_value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QCassandraValue.
 *
 * The following commands support the QCassandraValue:
 *
 * \li COMMAND_DEFAULT_VALUE
 * \li COMMAND_DEFAULT_VALUE_OR_NULL
 *
 * \param[in] cmd  The command.
 * \param[in] value  The value attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QtCassandra::QCassandraValue value)
{
    cmd_info_t inst(cmd, value);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QDomElement.
 *
 * The following commands support the QDomElement:
 *
 * \li COMMAND_ELEMENT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QDomElement element)
{
    cmd_info_t inst(cmd, element);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a QDomDocument.
 *
 * The following commands support the QDomDocument:
 *
 * \li COMMAND_ELEMENT
 *
 * \param[in] cmd  The command.
 * \param[in] element  The element attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, QDomDocument doc)
{
    cmd_info_t inst(cmd, doc);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a search_result_t reference.
 *
 * The following commands support the result reference:
 *
 * \li COMMAND_RESULT
 *
 * \param[in] cmd  The command.
 * \param[in] result  The result attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, search_result_t& result)
{
    cmd_info_t inst(cmd, result);
    f_program.push_back(inst);
    return *this;
}


/** \brief Add a command with a path_info_t reference.
 *
 * The following commands support the path_info_t reference:
 *
 * \li COMMAND_PATH_INFO_GLOBAL
 * \li COMMAND_PATH_INFO_BRANCH
 * \li COMMAND_PATH_INFO_REVISION
 *
 * \param[in] cmd  The command.
 * \param[in] ipath  The ipath attached to that command.
 *
 * \return A reference to the field_search so further () can be used.
 */
field_search& field_search::operator () (command_t cmd, path_info_t& ipath)
{
    cmd_info_t inst(cmd, ipath);
    f_program.push_back(inst);
    return *this;
}


/** \brief Run the search commands.
 *
 * This function runs the search commands over the data found in Cassandra.
 * It is somewhat similar to an XPath only it applies to a tree in Cassandra
 * instead of an XML tree.
 *
 * By default, you are expected to search for the very first instance of
 * the parameter sought. It is possible to transform the search in order
 * to search all the parameters that match.
 *
 * \return An array of QCassandraValue's.
 */
void field_search::run()
{
    struct auto_search
    {
        auto_search(char const *filename, char const *func, int line, snap_child *snap, cmd_info_vector_t& program)
            : f_content_plugin(content::content::instance())
            , f_filename(filename)
            , f_function(func)
            , f_line(line)
            , f_snap(snap)
            , f_program(program)
            //, f_mode(SEARCH_MODE_FIRST) -- auto-init
            , f_site_key(f_snap->get_site_key_with_slash())
            , f_revision_owner(f_content_plugin->get_plugin_name())
            //, f_field_name("") -- auto-init
            //, f_self("") -- auto-init
            , f_current_table(f_content_plugin->get_content_table())
            //, f_element() -- auto-init
            //, f_found_self(false) -- auto-init
            //, f_saved(false) -- auto-init
            //, f_result() -- auto-init
            //, f_variables() -- auto-init
            //, f_path_info() -- auto-init
        {
        }

        void cmd_field_name(QString const& field_name)
        {
            if(field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("COMMAND_FIELD_NAME cannot be set to an empty string");
            }
            f_field_name = field_name;
        }

        void cmd_field_name_with_vars(QString const& field_name)
        {
            if(field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("COMMAND_FIELD_NAME_WITH_VARS cannot be set to an empty string");
            }
            f_field_name.clear();
            QByteArray name(field_name.toUtf8());
            for(char const *n(name.data()); *n != '\0'; ++n)
            {
                if(*n == '$')
                {
                    if(n[1] != '{')
                    {
                        throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable name \"%1\" must be enclosed in { and }.").arg(field_name));
                    }
                    QString varname;
                    for(n += 2; *n != '}'; ++n)
                    {
                        if(*n == '\0')
                        {
                            throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable \"%1\" not ending with }.").arg(field_name));
                        }
                        varname += *n;
                    }
                    if(!f_variables.contains(varname))
                    {
                        throw content_exception_invalid_sequence(QString("COMMAND_FIELD_NAME_WITH_VARS variable \"%1\" is not defined.").arg(varname));
                    }
                    f_field_name += f_variables[varname];
                }
                else
                {
                    f_field_name += *n;
                }
            }
        }

        void cmd_mode(int64_t mode)
        {
            f_mode = static_cast<int>(mode); // XXX fix, should be a cast to mode_t
        }

        void cmd_branch_path(int64_t main_page)
        {
            // retrieve the path from this cell:
            //   content::revision_control::current_branch_key
            f_path_info.set_path(f_self);
            f_path_info.set_main_page(main_page != 0);
            cmd_path(f_path_info.get_branch_key());

            // make sure the current table is the branch table
            f_current_table = f_content_plugin->get_branch_table();
        }

        void cmd_revision_path(int64_t main_page)
        {
            // retrieve the path from this cell:
            //   content::revision_control::current_revision_key::<branch>::<locale>
            f_path_info.set_path(f_self);
            f_path_info.set_main_page(main_page != 0);
            cmd_path(f_path_info.get_revision_key());

            // make sure the current table is the revision table
            f_current_table = f_content_plugin->get_revision_table();
        }

        void cmd_table(QString const& name)
        {
            if(name == get_name(SNAP_NAME_CONTENT_TABLE))
            {
                f_current_table = f_content_plugin->get_content_table();
            }
            else if(name == get_name(SNAP_NAME_CONTENT_BRANCH_TABLE))
            {
                f_current_table = f_content_plugin->get_branch_table();
            }
            else if(name == get_name(SNAP_NAME_CONTENT_REVISION_TABLE))
            {
                f_current_table = f_content_plugin->get_revision_table();
            }
            else
            {
                throw content_exception_invalid_sequence("COMMAND_TABLE expected the name of the table to access: \"content\", \"branch\", or \"revision\"");
            }
        }

        void cmd_self(QString const& self)
        {
            // verify that a field name is defined
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_SELF without first being given a COMMAND_FIELD_NAME");
            }

            if(f_current_table->exists(self)
            && f_current_table->row(self)->exists(f_field_name))
            {
                f_found_self = true;

                // found a field, add it to result
                if(SEARCH_MODE_PATHS == f_mode)
                {
                    // save the path(s) only
                    f_result.push_back(self);
                }
                else
                {
                    // save the value
                    f_result.push_back(f_current_table->row(self)->cell(f_field_name)->value());
                }
            }
        }

        void cmd_path(QString const& path)
        {
            f_found_self = false;

            // get the self path and add the site key if required
            // (it CAN be empty in case we are trying to access the home page
            f_self = path;
            if(f_self.isEmpty() || !f_self.startsWith(f_site_key))
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(f_self);
                f_self = f_site_key + f_self;
            }
        }

        void cmd_path_info(path_info_t const& ipath, content::param_revision_t mode)
        {
            switch(mode)
            {
            case content::PARAM_REVISION_GLOBAL:
                cmd_path(ipath.get_cpath());
                f_current_table = f_content_plugin->get_content_table();
                break;

            case content::PARAM_REVISION_BRANCH:
                cmd_path(ipath.get_branch_key());
                f_current_table = f_content_plugin->get_branch_table();
                break;

            case content::PARAM_REVISION_REVISION:
                cmd_path(ipath.get_revision_key());
                f_current_table = f_content_plugin->get_revision_table();
                break;

            default:
                throw snap_logic_exception(QString("invalid mode (%1) in cmd_path_info.").arg(static_cast<int>(mode)));

            }
        }

        void cmd_children(int64_t depth)
        {
            // invalid depth?
            if(depth < 0)
            {
                throw content_exception_invalid_sequence("COMMAND_CHILDREN expects a depth of 0 or more");
            }
            if(depth == 0 || !f_found_self)
            {
                // no depth or no self
                return;
            }

            QString match;

            // last part is dynamic?
            // (later we could support * within the path and not just at the
            // very end...)
            if(f_self.endsWith("::*"))
            {
                int const pos = f_self.lastIndexOf('/');
                if(pos == -1)
                {
                    throw content_exception_invalid_name("f_self is expected to always include at least one slash, \"" + f_self + "\" does not");
                }
                // the match is everything except the '*'
                match = f_self.left(f_self.length() - 1);
                f_self = f_self.left(pos);
            }

            QStringList children;
            children += f_self;

            for(int i(0); i < children.size(); ++i, --depth)
            {
                // first loop through all the children of self for f_field_name
                // and if depth is larger than 1, repeat the process with those children
                path_info_t ipath;
                ipath.set_path(children[i]);
                links::link_info info(get_name(SNAP_NAME_CONTENT_CHILDREN), false, ipath.get_key(), ipath.get_branch());
                QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
                links::link_info child_info;
                while(link_ctxt->next_link(child_info))
                {
                    QString const child(child_info.key());
                    if(match.isEmpty() || child.startsWith(match))
                    {
                        cmd_self(child);
                        if(!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                        {
                            return;
                        }

                        if(depth >= 2)
                        {
                            // record this child as its children will have to be tested
                            children += child;
                        }
                    }
                }
            }
        }

        void cmd_parents(QString limit_path)
        {
            // verify that a field name is defined in self or any parent
            if(f_field_name.isEmpty())
            {
                throw content_exception_invalid_sequence("the field_search cannot check COMMAND_PARENTS without first being given a COMMAND_FIELD_NAME");
            }
            if(!f_found_self)
            {
                return;
            }

            // fix the parent limit
            if(!limit_path.startsWith(f_site_key) || limit_path.isEmpty())
            {
                // path does not yet include the site key
                f_snap->canonicalize_path(limit_path);
                limit_path = f_site_key + limit_path;
            }

            if(f_self.startsWith(limit_path))
            {
                // we could use the parent link from each page, but it is
                // a lot faster to compute it each time (no db access)
                QStringList parts(f_self.right(f_self.length() - f_site_key.length()).split('/'));
                while(!parts.isEmpty())
                {
                    parts.pop_back();
                    QString self(parts.join("/"));
                    cmd_self(f_site_key + self);
                    if((!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                    || self == limit_path)
                    {
                        return;
                    }
                }
            }
        }

        void cmd_link(QString const& link_name)
        {
            if(!f_found_self)
            {
                // no self, no link to follow
                return;
            }

            bool const unique_link(true);
            path_info_t ipath;
            ipath.set_path(f_self);
            links::link_info info(link_name, unique_link, ipath.get_key(), ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info type_info;
            if(link_ctxt->next_link(type_info))
            {
                f_self = type_info.key();
                cmd_self(f_self);
            }
            else
            {
                // no such link
                f_self.clear();
                f_found_self = false;
            }
        }

        void cmd_default_value(QtCassandra::QCassandraValue const& value, bool keep_null)
        {
            if(!value.nullValue() || keep_null)
            {
                f_result.push_back(value);
            }
        }

        void cmd_element(QDomElement element)
        {
            f_element = element;
        }

        // retrieve an element given a path, element must exist, if
        // not there it ends up being NULL; test with COMMAND_IF_ELEMENT_NULL
        // and COMMAND_IF_NOT_ELEMENT_NULL
        void cmd_path_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QStringList names(child_name.split("/"));
                int const max_names(names.size());
                for(int idx(0); idx < max_names && !f_element.isNull(); ++idx)
                {
                    QString name(names[idx]);
                    if(name.isEmpty())
                    {
                        // happens when child_name starts/ends with '/'
                        continue;
                    }
                    f_element = f_element.firstChildElement(name);
                }
            }
        }

        void cmd_child_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QDomElement child(f_element.firstChildElement(child_name));
                if(child.isNull())
                {
                    // it doesn't exist yet, add it
                    QDomDocument doc(f_element.ownerDocument());
                    child = doc.createElement(child_name);
                    f_element.appendChild(child);
                }
                f_element = child;
            }
        }

        void cmd_new_child_element(QString const& child_name)
        {
            if(!f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                f_element = child;
            }
        }

        void cmd_parent_element()
        {
            if(!f_element.isNull())
            {
                f_element = f_element.parentNode().toElement();
            }
        }

        void cmd_element_text()
        {
            if(!f_element.isNull())
            {
                f_result.push_back(f_element.text());
            }
        }

        void cmd_element_attr(QString const& attr)
        {
            if(!f_element.isNull())
            {
                QStringList a(attr.split('='));
                if(a.size() == 1)
                {
                    // checked="checked"
                    a += a[0];
                }
                f_element.setAttribute(a[0], a[1]);
            }
        }

        void cmd_reset(bool status)
        {
            f_saved = status;
            f_result.clear();
        }

        void cmd_result(search_result_t& result)
        {
            result = f_result;
        }

        void cmd_last_result_to_var(QString const& varname)
        {
            if(f_result.isEmpty())
            {
                throw content_exception_invalid_sequence(QString("no result to save in variable \"%1\"").arg(varname));
            }
            QtCassandra::QCassandraValue value(f_result.last());
            f_result.pop_back();
            f_variables[varname] = value.stringValue();
        }

        void cmd_save(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QStringList children(child_name.split('/'));
                QDomElement parent(f_element);
                while(children.size() != 1)
                {
                    // TODO write a clean parser seeking in the string
                    //      it would make it faster (i.e. no intermediate
                    //      list of strings)
                    QStringList child_attr(children[0].split('['));
                    QDomElement child(doc.createElement(child_attr[0]));
                    parent.appendChild(child);
                    while(child_attr.size() > 1)
                    {
                        // remove the ']' if present
                        if(child_attr[1].right(1) != "]")
                        {
                            throw content_exception_invalid_sequence("invalid attribute definition, missing ']'");
                        }
                        child_attr[1].remove(child_attr[1].length() - 1, 1);
                        QStringList attr_value(child_attr[1].split('='));
                        if(attr_value.size() == 1)
                        {
                            attr_value += attr_value[0];
                        }
                        child.setAttribute(attr_value[0], attr_value[1]);
                        child_attr.removeAt(1);
                    }
                    parent = child;
                    children.removeAt(0);
                }
                QDomElement last_child(doc.createElement(children[0]));
                parent.appendChild(last_child);
                QString const string(f_result[0].stringValue());
                snap_dom::insert_html_string_to_xml_doc(last_child, string);
                cmd_reset(true);
            }
        }

        void cmd_save_int64(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                QDomText text(doc.createTextNode(QString("%1").arg(f_result[0].int64Value())));
                child.appendChild(text);
                cmd_reset(true);
            }
        }

        void cmd_save_int64_date(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);
                QDomText text(doc.createTextNode(f_snap->date_to_string(f_result[0].int64Value())));
                child.appendChild(text);
                cmd_reset(true);
            }
        }

        void cmd_save_xml(QString const& child_name)
        {
            if(!f_result.isEmpty() && !f_element.isNull())
            {
                QDomDocument doc(f_element.ownerDocument());
                QDomElement child(doc.createElement(child_name));
                f_element.appendChild(child);

                // parse the XML (XHTML) string
                snap_dom::insert_html_string_to_xml_doc(child, f_result[0].stringValue());

                cmd_reset(true);
            }
        }

        void cmd_if_found(int& i, int64_t label, bool equal)
        {
            if(f_result.isEmpty() == equal)
            {
                cmd_goto(i, label);
            }
        }

        void cmd_if_element_null(int& i, int64_t label, bool equal)
        {
            if(f_element.isNull() == equal)
            {
                cmd_goto(i, label);
            }
        }

        void cmd_goto(int& i, int64_t label)
        {
            int const max_size(f_program.size());
            for(int j(0); j < max_size; ++j)
            {
                if(f_program[j].get_command() == COMMAND_LABEL
                && f_program[j].get_int64() == label)
                {
                    // NOTE: the for() loop will do a ++i which is fine
                    //       since we're giving the label position here
                    i = j;
                    return;
                }
            }
            throw content_exception_invalid_sequence(QString("found unknown label %1 at %2").arg(label).arg(i));
        }

        void cmd_warning(QString const& warning_msg)
        {
            // XXX only problem is we do not get the right filename,
            //     line number, function name on this one...
            if(!f_saved)
            {
                SNAP_LOG_WARNING("in ")(f_filename)(":")(f_function)(":")(f_line)(": ")(warning_msg)(" (path: \"")(f_self)("\" and field name: \"")(f_field_name)("\")");
                f_saved = false;
            }
        }

        void run()
        {
            int const max_size(f_program.size());
            for(int i(0); i < max_size; ++i)
            {
                switch(f_program[i].get_command())
                {
                case COMMAND_RESET:
                    cmd_reset(false);
                    break;

                case COMMAND_FIELD_NAME:
                    cmd_field_name(f_program[i].get_string());
                    break;

                case COMMAND_FIELD_NAME_WITH_VARS:
                    cmd_field_name_with_vars(f_program[i].get_string());
                    break;

                case COMMAND_MODE:
                    cmd_mode(f_program[i].get_int64());
                    break;

                case COMMAND_BRANCH_PATH:
                    cmd_branch_path(f_program[i].get_int64());
                    break;

                case COMMAND_REVISION_PATH:
                    cmd_revision_path(f_program[i].get_int64());
                    break;

                case COMMAND_TABLE:
                    cmd_table(f_program[i].get_string());
                    break;

                case COMMAND_SELF:
                    cmd_self(f_self);
                    break;

                case COMMAND_PATH:
                    cmd_path(f_program[i].get_string());
                    break;

                case COMMAND_PATH_INFO_GLOBAL:
                    cmd_path_info(f_program[i].get_ipath(), content::PARAM_REVISION_GLOBAL);
                    break;

                case COMMAND_PATH_INFO_BRANCH:
                    cmd_path_info(f_program[i].get_ipath(), content::PARAM_REVISION_BRANCH);
                    break;

                case COMMAND_PATH_INFO_REVISION:
                    cmd_path_info(f_program[i].get_ipath(), content::PARAM_REVISION_REVISION);
                    break;

                case COMMAND_CHILDREN:
                    cmd_children(f_program[i].get_int64());
                    break;

                case COMMAND_PARENTS:
                    cmd_parents(f_program[i].get_string());
                    break;

                case COMMAND_LINK:
                    cmd_link(f_program[i].get_string());
                    break;

                case COMMAND_DEFAULT_VALUE:
                    cmd_default_value(f_program[i].get_value(), true);
                    break;

                case COMMAND_DEFAULT_VALUE_OR_NULL:
                    cmd_default_value(f_program[i].get_value(), false);
                    break;

                case COMMAND_ELEMENT:
                    cmd_element(f_program[i].get_element());
                    break;

                case COMMAND_PATH_ELEMENT:
                    cmd_path_element(f_program[i].get_string());
                    break;

                case COMMAND_CHILD_ELEMENT:
                    cmd_child_element(f_program[i].get_string());
                    break;

                case COMMAND_NEW_CHILD_ELEMENT:
                    cmd_new_child_element(f_program[i].get_string());
                    break;

                case COMMAND_PARENT_ELEMENT:
                    cmd_parent_element();
                    break;

                case COMMAND_ELEMENT_TEXT:
                    cmd_element_text();
                    break;

                case COMMAND_ELEMENT_ATTR:
                    cmd_element_attr(f_program[i].get_string());
                    break;

                case COMMAND_RESULT:
                    cmd_result(*f_program[i].get_result());
                    break;

                case COMMAND_LAST_RESULT_TO_VAR:
                    cmd_last_result_to_var(f_program[i].get_string());
                    break;

                case COMMAND_SAVE:
                    cmd_save(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_INT64:
                    cmd_save_int64(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_INT64_DATE:
                    cmd_save_int64_date(f_program[i].get_string());
                    break;

                case COMMAND_SAVE_XML:
                    cmd_save_xml(f_program[i].get_string());
                    break;

                case COMMAND_LABEL:
                    // this is a nop
                    break;

                case COMMAND_IF_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), false);
                    break;

                case COMMAND_IF_NOT_FOUND:
                    cmd_if_found(i, f_program[i].get_int64(), true);
                    break;

                case COMMAND_IF_ELEMENT_NULL:
                    cmd_if_element_null(i, f_program[i].get_int64(), true);
                    break;

                case COMMAND_IF_NOT_ELEMENT_NULL:
                    cmd_if_element_null(i, f_program[i].get_int64(), false);
                    break;

                case COMMAND_GOTO:
                    cmd_goto(i, f_program[i].get_int64());
                    break;

                case COMMAND_WARNING:
                    cmd_warning(f_program[i].get_string());
                    break;

                default:
                    throw content_exception_invalid_sequence(QString("encountered an unknown instruction (%1)").arg(static_cast<int>(f_program[i].get_command())));

                }
                if(!f_result.isEmpty() && SEARCH_MODE_FIRST == f_mode)
                {
                    return;
                }
            }
        }

        content *                                       f_content_plugin;
        char const *                                    f_filename;
        char const *                                    f_function;
        int                                             f_line;
        zpsnap_child_t                                  f_snap;
        cmd_info_vector_t&                              f_program;
        safe_mode_t                                     f_mode;
        QString const                                   f_site_key;
        QString                                         f_revision_owner;
        QString                                         f_field_name;
        QString                                         f_self;
        QtCassandra::QCassandraTable::pointer_t         f_current_table;
        QDomElement                                     f_element;
        controlled_vars::fbool_t                        f_found_self;
        controlled_vars::fbool_t                        f_saved;
        search_result_t                                 f_result;
        variables_t                                     f_variables;
        path_info_t                                     f_path_info;
    } search(f_filename, f_function, f_line, f_snap, f_program);

    search.run();
}


/** \brief This function is used by the FIELD_SEARCH macro.
 *
 * This function creates a field_search object and initializes it
 * with the information specified by the FIELD_SEARCH macro. The
 * result is a field_search that we can use to instantly run a
 * search program.
 *
 * \param[in] filename  The name of the file where the FIELD_SEARCH macro is used.
 * \param[in] func  The name of the function using the FIELD_SEARCH macro.
 * \param[in] line  The line where the FIELD_SEARCH macro can be found.
 * \param[in] snap  A pointer to the snap server.
 */
field_search create_field_search(char const *filename, char const *func, int line, snap_child *snap)
{
    field_search fs(filename, func, line, snap);
    return fs;
}




/** \brief Create a structure used to setup an attachment file.
 *
 * This contructor is used whenever loading an attachment from the
 * database. In this case the file is setup from the database
 * information.
 *
 * The other constructor is used when creating an attachment from
 * data received in a POST or generated by a backend. It includes
 * the file information.
 *
 * \param[in] snap  A pointer to the snap_child object.
 */
attachment_file::attachment_file(snap_child *snap)
    : f_snap(snap)
    //, f_file()
    //, f_multiple(false) -- auto-init
    //, f_parent_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_cpath("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
{
}


/** \brief Create a structure used to setup an attachment file.
 *
 * Create and properly initialize this structure and then you can call
 * the create_attachment() function which takes this structure as a parameter
 * to create a new file in the database.
 *
 * To finish the initialization of this structure you must call the
 * following functions:
 *
 * \li set_parent_cpath()
 * \li set_field_name()
 * \li set_attachment_cpath()
 * \li set_attachment_owner()
 * \li set_attachment_type()
 *
 * By default the attachment file structure is set to work on unique
 * files. Call the set_multiple() function to make sure that the
 * user does not overwrite previous attachments.
 *
 * \warning
 * Each attachment file structure can really only be used once (it is
 * used for throw away objects.) The get_name() function, for example,
 * generates the name internally and it is not possible to change it
 * afterward.
 *
 * \warning
 * Calling the get_name() function fails with a throw if some of
 * the mandatory parameters were not properly set.
 *
 * \param[in] snap  A pointer to the snap_child object.
 * \param[in] file  The file to attach to this page.
 */
attachment_file::attachment_file(snap_child *snap, snap_child::post_file_t const& file)
    : f_snap(snap)
    , f_file(file)
    //, f_multiple(false) -- auto-init
    //, f_parent_cpath("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_attachment_cpath("") -- auto-init
    //, f_attachment_owner("") -- auto-init
    //, f_attachment_type("") -- auto-init
{
}


/** \brief Whether multiple files can be saved under this one name.
 *
 * This function is used to mark the attachment as unique (false) or
 * not (true). If unique, saving the attachment again with a different
 * files removes the existing file first.
 *
 * When multiple is set to true, saving a new file adds it to the list
 * of existing files. The list may be empty too.
 *
 * Multiple adds a unique number at the end of each field name
 * which gives us a full name such as:
 *
 * \code
 * "content::attachment::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * By default a file is expected to be unique (multiple is set to false).
 *
 * \param[in] multiple  Whether the field represents a multi-file attachment.
 *
 * \sa get_multiple()
 * \sa get_name()
 */
void attachment_file::set_multiple(bool multiple)
{
    f_multiple = multiple;
}


/** \brief Set the path where the attachment is being added.
 *
 * This is the path to the parent page to which this attachment is
 * being added. A path is mandatory so you will have to call this
 * function, although the empty path is allowed (it represents the
 * home page so be careful!)
 *
 * \note
 * The class marks whether you set the path or not. If not, trying
 * to use it (get_cpath() function called) generates an exception
 * because it is definitively a mistake.
 *
 * \param[in] cpath  The path to the page receiving the attachment.
 *
 * \sa get_cpath()
 */
void attachment_file::set_parent_cpath(QString const& cpath)
{
    f_parent_cpath = cpath;
    f_has_cpath = true;
}


/** \brief Set the name of the field for the attachment.
 *
 * When saving a file as an attachment, we want to save the reference
 * in the parent as such. This makes it a lot easier to find the
 * attachments attached to a page.
 *
 * Note that to retreive the full name to the field, make sure to
 * call the get_name() function, the get_field_name() will return
 * just and only the \<field name> part, not the whole name.
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<field name>::path::<server_name>_<unique number>"
 * \endcode
 *
 * \param[in] field_name  The name of the field used to save this attachment.
 *
 * \sa get_field_name()
 * \sa get_name()
 */
void attachment_file::set_field_name(QString const& field_name)
{
    f_field_name = field_name;
}


/** \brief Define the path of the attachment page.
 *
 * This function saves the path to the attachment itself in the
 * attachment_file object.
 *
 * Only the create_attachment() function is expected to call this function,
 * although if you replicate your own similar function, then you will have
 * to call this function from your replica.
 *
 * The path is expected to the canonicalized and set only once the full
 * path in the content table is known.
 *
 * \param[in] cpath  The path to the attachment file.
 *
 * \sa get_attachment_cpath()
 */
void attachment_file::set_attachment_cpath(QString const& cpath)
{
    f_attachment_cpath = cpath;
}


/** \brief Set the owner of this attachment.
 *
 * This name represents the plugin owner of the attachment. It must be
 * a valid plugin name as it is saved as the owner of the attachment.
 * This allows the plugin to specially handle the attachment when the
 * client wants to retrieve it.
 *
 * This name is saved as the primary owner of the attachment page.
 *
 * \param[in] owner  The name of the plugin that owns that attachment.
 *
 * \sa get_attachment_owner()
 */
void attachment_file::set_attachment_owner(QString const& owner)
{
    f_attachment_owner = owner;
}


/** \brief Define the type of the attachment page.
 *
 * When adding an attachment to the database, a new page is created as
 * a child of the page where the attachment is added. This allows us
 * to easily do all sorts of things with attachments. This new page being
 * content it needs to have a type and this type represents that type.
 *
 * In most cases the type is set to the parent by default.
 *
 * \param[in] type  The type of the page.
 *
 * \sa get_attachment_type()
 */
void attachment_file::set_attachment_type(QString const& type)
{
    f_attachment_type = type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was added to the database.
 *
 * \sa get_creation_time()
 */
void attachment_file::set_creation_time(int64_t time)
{
    f_creation_time = time;
}


/** \brief Set the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \param[in] time  The time when the attachment was last modified.
 *
 * \sa get_update_time()
 */
void attachment_file::set_update_time(int64_t time)
{
    f_update_time = time;
}


/** \brief Set the dependencies of this attachment.
 *
 * Attachments can be given dependencies, with versions, and specific
 * browsers. This is particularly useful for JS and CSS files as in
 * this way we can server exactly what is necessary.
 *
 * One dependency looks like a name, one or two versions with an operator
 * (usually \< to define a range), and a browser name. The versions are
 * written between parenthesis and the browser name between square brackets:
 *
 * \code
 * <attachment name> ...
 *    ... (<version>) ...
 *    ... (<op> <version>) ...
 *    ... (<version> <op> <version>) ...
 *    ... (<version>, <version>, ...) ...
 *    ... (<op> <version>, <op> <version>, ...) ...
 *       ... [<browser>]
 *       ... [<browser>, <browser>, ...]
 * \endcode
 *
 * When two versions are used, the operator must be \<. It defines a range
 * and any versions defined between the two versions are considered valid.
 * The supported operators are =, \<, \<=, \>, \>=, !=, and ,. The comma
 * can be used to define a set of versions.
 *
 * Each attachment name must be defined only once.
 *
 * Attachments that are given dependencies are also added to a special
 * list so they can be found instantly. This is important since when a page
 * says to insert a JavaScript file, all its dependencies have to be added
 * too and that can be done automatically using these dependencies.
 *
 * \param[in] dependencies  The dependencies of this attachment.
 */
void attachment_file::set_dependencies(dependency_list_t& dependencies)
{
    f_dependencies = dependencies;
}


/** \brief Set the name of the field the attachment comes from.
 *
 * This function is used by the load_attachment() function to set the
 * name of the file attachment as if it had been sent by a POST.
 *
 * \param[in] name  The name of the field used to upload this attachment.
 */
void attachment_file::set_file_name(QString const& name)
{
    f_file.set_name(name);
}


/** \brief Set the name of the file.
 *
 * This function sets the name of the file as it was sent by the POST
 * sending the attachment.
 *
 * \param[in] filename  The name of the file as sent by the POST message.
 */
void attachment_file::set_file_filename(QString const& filename)
{
    f_file.set_filename(filename);
}


/** \brief Set the mime_type of the file.
 *
 * This function can be used to setup the MIME type of the file when
 * the data if the file is not going to be set in the attachment file.
 * (It is useful NOT to load the data if you are not going to use it
 * anyway!)
 *
 * The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_mime_type(QString const& mime_type)
{
    f_file.set_mime_type(mime_type);
}


/** \brief Set the original mime_type of the file.
 *
 * This function can be used to setup the original MIME type of the
 * file. The original MIME type is the one sent by the browser at
 * the time the attachment was POSTed.
 *
 * \param[in] mime_type  The original MIME time as sent by the client.
 */
void attachment_file::set_file_original_mime_type(QString const& mime_type)
{
    f_file.set_original_mime_type(mime_type);
}


/** \brief Set the creation time.
 *
 * This function sets the creating time of the file.
 *
 * \param[in] ctime  The creation time.
 */
void attachment_file::set_file_creation_time(time_t ctime)
{
    f_file.set_creation_time(ctime);
}


/** \brief Set the modification time.
 *
 * This function sets the creation time of the file.
 *
 * \param[in] mtime  The last modification time.
 */
void attachment_file::set_file_modification_time(time_t mtime)
{
    f_file.set_modification_time(mtime);
}


/** \brief Set the data of the file.
 *
 * This function sets the data of the file. This is the actual file
 * content.
 *
 * \param[in] data  The last modification time.
 */
void attachment_file::set_file_data(QByteArray const& data)
{
    f_file.set_data(data);
}


/** \brief Set the size of the file.
 *
 * This function sets the size of the file. This is particularly
 * useful if you do not want to load the data but still want to
 * get the size for display purposes.
 *
 * \param[in] size  Set the size of the file.
 */
void attachment_file::set_file_size(int size)
{
    f_file.set_size(size);
}


/** \brief Set the image width.
 *
 * This function is used to set the width of the image.
 *
 * \param[in] width  The image width.
 */
void attachment_file::set_file_image_width(int width)
{
    f_file.set_image_width(width);
}


/** \brief Set the image height.
 *
 * This function is used to set the height of the image.
 *
 * \param[in] height  The image height.
 */
void attachment_file::set_file_image_height(int height)
{
    f_file.set_image_height(height);
}


/** \brief Set the index of the field.
 *
 * This function is used to set the field index within the form.
 *
 * \param[in] index  The index of the field.
 */
void attachment_file::set_file_index(int index)
{
    f_file.set_index(index);
}


/** \brief Return whether the attachment is unique or not.
 *
 * This function returns the flag as set by the set_multiple().
 * If true it means that as many attachments as necessary can
 * be added under the same field name. Otherwise only one
 * attachment can be added.
 *
 * \return Whether multiple attachments can be saved.
 *
 * \sa set_multiple()
 * \sa get_name()
 */
bool attachment_file::get_multiple() const
{
    return f_multiple;
}


/** \brief Return the file structure.
 *
 * When receiving a file, in most cases it is via an upload so we
 * use that structure directly to avoid copying all that data all
 * the time.
 *
 * This function returns a reference so you can directly use a
 * reference instead of a copy.
 *
 * \note
 * The only way to setup the file is via the constructor.
 *
 * \return A reference to the file representing this attachment.
 *
 * \sa attachment_file()
 */
snap_child::post_file_t const& attachment_file::get_file() const
{
    return f_file;
}


/** \brief Path to the parent of the file.
 *
 * This path represents the parent receiving this attachment.
 *
 * \return The path to the parent of the attachment.
 *
 * \sa set_parent_cpath()
 */
QString const& attachment_file::get_parent_cpath() const
{
    if(!f_has_cpath)
    {
        throw content_exception_invalid_name("the cpath parameter of a attachment_file object was never set");
    }
    return f_parent_cpath;
}


/** \brief Retrieve the name of the field.
 *
 * This function retrieves the raw name of the field. For the complete
 * name, make sure to use the get_name() function instead.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The field name to use for this attachment file.
 *
 * \sa set_field_name()
 */
QString const& attachment_file::get_field_name() const
{
    if(f_field_name.isEmpty())
    {
        throw content_exception_invalid_name("the field name of a attachment_file object cannot be empty");
    }
    return f_field_name;
}


/** \brief Retrieve the path of the attachment page.
 *
 * This function returns the path that the create_attachment() function
 * creates to save the attachment. This can be used to later access the
 * attachment.
 *
 * The path is expected to the canonicalized.
 *
 * The function may return an empty string if the create_attachment()
 * function was not called or it failed. It is considered a bug to
 * set this path outside of the create_attachment() function.
 *
 * \return The path to the attachment file.
 *
 * \sa set_attachment_cpath()
 */
QString const& attachment_file::get_attachment_cpath() const
{
    return f_attachment_cpath;
}


/** \brief Retrieve the owner of the attachment page.
 *
 * This function returns the name of the plugin that becomes the attachment
 * owner in the content table. The owner has rights over the content to
 * display it, allow the client to download it, etc.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the owner was
 * not defined an the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the plugin that owns this attachment file.
 *
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_attachment_owner() const
{
    if(f_attachment_owner.isEmpty())
    {
        throw content_exception_invalid_name("the attachment owner of a attachment_file object cannot be empty");
    }
    return f_attachment_owner;
}


/** \brief Retrieve the type of the attachment page.
 *
 * This function returns the type to use for the page we are to create for
 * this attachment. This is one of the .../content-types/\<name> types.
 *
 * \exception content_exception_invalid_name
 * This function generates the invalid name exception if the type was
 * not defined and the parameter is still empty at the time it is to be
 * used.
 *
 * \return The name of the type to use for the attachment page.
 *
 * \sa set_attachment_type()
 */
QString const& attachment_file::get_attachment_type() const
{
    if(f_attachment_type.isEmpty())
    {
        throw content_exception_invalid_name("the attachment type of a attachment_file object cannot be empty");
    }
    return f_attachment_type;
}


/** \brief Set the creation time of the attachment.
 *
 * The first time the user POSTs an attachment, it saves the start date of
 * the HTTP request as the creation date. The loader sets the date back in
 * the attachment.
 *
 * \return The time when the attachment was added to the database.
 *
 * \sa set_creation_time()
 */
int64_t attachment_file::get_creation_time() const
{
    return f_creation_time;
}


/** \brief Get the modification time of the attachment.
 *
 * Each time the user POSTs an attachment, it saves the start date of the
 * HTTP request as the modification date. The loader sets the date back in
 * the attachment.
 *
 * \return The time when the attachment was last updated.
 *
 * \sa set_update_time()
 */
int64_t attachment_file::get_update_time() const
{
    return f_update_time;
}


/** \brief Retrieve the list of dependencies of an attachment.
 *
 * The list of dependencies on an attachment are set with the
 * set_dependencies() function. These are used to determine which files are
 * required in a completely automated way.
 *
 * \return The list of dependency of this attachment.
 */
dependency_list_t const& attachment_file::get_dependencies() const
{
    return f_dependencies;
}


/** \brief Generate the full field name.
 *
 * The name of the field in the parent page in the content is defined
 * as follow:
 *
 * \code
 * // name of the field in the database:
 * "content::attachment::<field name>::path"
 *
 * // or, if multiple is set to true:
 * "content::attachment::<field name>::path::<server name>_<unique number>"
 * \endcode
 *
 * To make sure that everyone always uses the same name each time, we
 * created this function and you'll automatically get the right name
 * every time.
 *
 * \warning
 * After the first call this function always returns exactly the same
 * name. This is because we cache the name so it can be called any
 * number of time and it will quickly return with the name.
 *
 * \return The full name of the field.
 *
 * \sa set_multiple()
 * \sa set_field_name()
 * \sa set_attachment_owner()
 */
QString const& attachment_file::get_name() const
{
    // this name appears in the PARENT of the attachment
    if(f_name.isEmpty())
    {
        if(f_multiple)
        {
            f_name = QString("%1::%2::%3::%4")
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_field_name())
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    .arg(f_snap->get_unique_number())
                    ;
        }
        else
        {
            f_name = QString("%1::%2::%3")
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT))
                    .arg(get_field_name())
                    .arg(snap::content::get_name(SNAP_NAME_CONTENT_ATTACHMENT_PATH_END))
                    ;
        }
    }
    return f_name;
}









/** \brief Initialize the status with the default status values.
 *
 * The default constructor of the status class defines a status object
 * with default values.
 *
 * The default values are:
 *
 * \li path_info_t::status_t::NO_ERROR for error
 * \li path_info_t::status_t::UNKNOWN_STATE for state
 * \li path_info_t::status_t::NOT_WORKING for working
 *
 * The default values can then be changed using the set_...() functions
 * of the class.
 *
 * You may also set the status using the set_status() function in case
 * you get a \p current_status after you created a status object.
 */
path_info_t::status_t::status_t()
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
    //, f_working(working_t::NOT_WORKING)
{
}


/** \brief Initialize the status with the specified current_status value.
 *
 * The constructor and get_status() make use of an integer to
 * save in the database but they do not declare the exact format
 * of that integer (i.e. the format is internal, hermetic.)
 *
 * The input parameter can only be defined from the get_status() of
 * another status. If you are not reading a new status, you must make
 * use of the constructor without a status specified.
 *
 * \param[in] current_status  The current status to save in this instance.
 */
path_info_t::status_t::status_t(status_type current_status)
    //: f_error(error_t::NO_ERROR)
    //, f_state(state_t::UNKNOWN_STATE)
    //, f_working(working_t::NOT_WORKING)
{
    set_status(current_status);
}


/** \brief Set the current status from the specified \p current_status value.
 *
 * This function accepts a \p current_status value which gets saved in the
 * corresponding f_state and f_working variable members.
 *
 * How the status is encoded in the \p current_status value is none of your
 * business. It is encoded by the get_status() and decoded using the
 * set_status(). That value can be saved in the database.
 *
 * \note
 * The constructor accepting a \p current_status parameter calls this
 * set_status() function to save its input value.
 *
 * \note
 * The error value is set to error_t::NO in this case.
 *
 * \param[in] current_status  The current status to save in this instance.
 */
void path_info_t::status_t::set_status(status_type current_status)
{
    // set some defaults so that way we have "proper" defaults on errors
    f_state = state_t::UNKNOWN_STATE;
    f_working = working_t::NOT_WORKING;

    state_t state(static_cast<state_t>(static_cast<int>(current_status) & 255));
    switch(state)
    {
    case state_t::UNKNOWN_STATE:
    case state_t::CREATE:
    case state_t::NORMAL:
    case state_t::HIDDEN:
    case state_t::MOVED:
    case state_t::DELETED:
        break;

    default:
        // any other status is not understood by this version of snap
//std::cerr << "status is " << static_cast<int>(static_cast<status_t>(f_status)) << "\n";
        f_error = error_t::UNSUPPORTED;
        return;

    }

    working_t working(static_cast<working_t>((static_cast<int>(current_status) / 256) & 255));
    switch(working)
    {
    case working_t::UNKNOWN_WORKING:
    case working_t::NOT_WORKING:
    case working_t::CREATING:
    case working_t::CLONING:
    case working_t::REMOVING:
    case working_t::UPDATING:
        break;

    default:
        // any other status is not understood by this version of snap
        f_error = error_t::UNSUPPORTED;
        return;

    }

    f_error = error_t::NO_ERROR;
    f_state = state;
    f_working = working;
}


/** \brief Retrieve the current value of the status of this object.
 *
 * This function returns the encoded status so one can save it in a
 * database, or some other place. The returned value is an integer.
 *
 * Internally, the value is handled as an error, a state, and a
 * working status. The encoder does not know how to handle errors
 * in this function, so if an error is detected, it actually
 * throws an exception. It is expected that your code will first
 * check whether is_error() returns true. If so, then you cannot
 * call this function.
 *
 * Note that if the state is still set to state_t::UNKNOWN_STATE, then
 * the function also raises an exception. This is because we
 * cannot allow saving that kind of a status in the database.
 * Some other combinations are forbidden. For example the
 * working_t::CREATING can only be used with the state_t::CREATING
 * status. All such mixes generate an error here.
 *
 * \exception snap_logic_exception
 * This exception is raised if this function gets called when the
 * status is currently representing an error. This is done that
 * way because there is really no reasons to allow for saving
 * an error in the database.
 *
 * \return The current status encrypted for storage.
 */
path_info_t::status_t::status_type path_info_t::status_t::get_status() const
{
    struct subfunc
    {
        static constexpr int status_combo(state_t s, working_t w)
        {
            return static_cast<unsigned char>(static_cast<int>(s))
                 | static_cast<unsigned char>(static_cast<int>(w)) * 256;
        }
    };

    // errors have priority and you cannot convert an error to a status_type
    if(f_error != error_t::NO_ERROR)
    {
        throw snap_logic_exception(QString("attempting to convert a status to status_type when it represents an error (%1).").arg(static_cast<int>(static_cast<error_t>(f_error))));
    }

    // of the 4 x 5 = 20 possibilities, we only allow 14 of them
    switch(subfunc::status_combo(f_state, f_working))
    {
    // creating
    case subfunc::status_combo(state_t::CREATE, working_t::CREATING):
    // normal
    case subfunc::status_combo(state_t::NORMAL, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL, working_t::CLONING):
    case subfunc::status_combo(state_t::NORMAL, working_t::REMOVING):
    case subfunc::status_combo(state_t::NORMAL, working_t::UPDATING):
    // hidden
    case subfunc::status_combo(state_t::HIDDEN, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::CLONING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::REMOVING):
    case subfunc::status_combo(state_t::HIDDEN, working_t::UPDATING):
    // moved
    case subfunc::status_combo(state_t::MOVED, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED, working_t::REMOVING):
    case subfunc::status_combo(state_t::MOVED, working_t::UPDATING):
    // deleted
    case subfunc::status_combo(state_t::DELETED, working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::DELETED, working_t::UPDATING):
        break;

    default:
        throw snap_logic_exception(QString("attempting to convert status with state %1 and working %2 which is not allowed").arg(static_cast<int>(static_cast<state_t>(f_state))).arg(static_cast<int>(static_cast<working_t>(f_working))));

    }

    // if no error, then the value is (state | (working << 8))
    return static_cast<status_type>(static_cast<state_t>(f_state))
         | static_cast<status_type>(static_cast<working_t>(f_working)) * 256;
}


/** \brief Get 
 * 
 * Verify that going from the current status (this) to the \p destination
 * status is acceptable.
 *
 * \param[in] destination  The new state.
 *
 * \return true if the transition is acceptable, false otherwise.
 */
bool path_info_t::status_t::valid_transition(status_t destination) const
{
    struct subfunc
    {
        static constexpr int status_combo(state_t s1, working_t w1, state_t s2, working_t w2)
        {
            return static_cast<unsigned char>(static_cast<int>(s1))
                 | static_cast<unsigned char>(static_cast<int>(w1)) * 0x100
                 | static_cast<unsigned char>(static_cast<int>(s2)) * 0x10000
                 | static_cast<unsigned char>(static_cast<int>(w2)) * 0x1000000;
        }
    };

    if(is_error())
    {
        return f_error == error_t::UNDEFINED
            && destination.f_state == state_t::CREATE
            && destination.f_working == working_t::CREATING;
    }

    // shift by 8 is safe since the status is expected to be one byte
    // however, the special statuses are negative so we clear a few bits
    switch(subfunc::status_combo(f_state, f_working, destination.f_state, destination.f_working))
    {
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::MOVED,     working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::CLONING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::REMOVING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::UPDATING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::CLONING,     state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::REMOVING,    state_t::NORMAL,    working_t::NOT_WORKING): // in case of a reset
    case subfunc::status_combo(state_t::NORMAL,     working_t::REMOVING,    state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::NORMAL,     working_t::UPDATING,    state_t::NORMAL,    working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::CLONING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::REMOVING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::NOT_WORKING, state_t::HIDDEN,    working_t::UPDATING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::CLONING,     state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::REMOVING,    state_t::HIDDEN,    working_t::NOT_WORKING): // in case of a reset
    case subfunc::status_combo(state_t::HIDDEN,     working_t::REMOVING,    state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::HIDDEN,     working_t::UPDATING,    state_t::HIDDEN,    working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::MOVED,     working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::HIDDEN,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::MOVED,      working_t::NOT_WORKING, state_t::MOVED,     working_t::CLONING):
    case subfunc::status_combo(state_t::MOVED,      working_t::CLONING,     state_t::MOVED,     working_t::NOT_WORKING):

    case subfunc::status_combo(state_t::DELETED,    working_t::NOT_WORKING, state_t::DELETED,   working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::DELETED,    working_t::NOT_WORKING, state_t::DELETED,   working_t::CLONING):
    case subfunc::status_combo(state_t::DELETED,    working_t::CLONING,     state_t::DELETED,   working_t::NOT_WORKING):

    // see error handle prior to this switch
    //case subfunc::status_combo(state_t::UNDEFINED,  working_t::NOT_WORKING, state_t::CREATE,    working_t::CREATING):

    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::CREATE,    working_t::CREATING):
    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::NORMAL,    working_t::NOT_WORKING):
    case subfunc::status_combo(state_t::CREATE,     working_t::CREATING,    state_t::HIDDEN,    working_t::NOT_WORKING):

        // this is a valid combo
        return true;

    default:
        // we do not like this combo at this time
        return false;

    }
}


/** \brief Set the error number in this status.
 *
 * Change the current status in an erroneous status. By default an object
 * is considered to not have any errors.
 *
 * The current state and working statuses do not get modified.
 *
 * \param[in] error  The new error to save in this status.
 */
void path_info_t::status_t::set_error(error_t error)
{
    f_error = error;
}


/** \brief Retrieve the current error.
 *
 * This function returns the current error of an ipath. If this status
 * represents an error, you may also call the is_error() function which
 * will return true for any errors except error_t::NO.
 *
 * \return The current error.
 */
path_info_t::status_t::error_t path_info_t::status_t::get_error() const
{
    return f_error;
}


/** \brief Check whether the path represents an error.
 *
 * If a path represents an error (which means the set_error() was called
 * with a value other than error_t::NO) then this function returns true.
 * Otherwise it returns true.
 *
 * \return true if the error in this status is not error_t::NO.
 */
bool path_info_t::status_t::is_error() const
{
    return f_error != error_t::NO_ERROR;
}


/** \brief Reset this status with the specified values.
 *
 * This function can be used to reset the status to the specified
 * state and working values. It also resets the current error
 * status.
 *
 * This is particularly useful to go from an undefined status to
 * a creating status.
 *
 * This function is a shortcut from doing:
 *
 * \code
 *      status.set_error(error_t::NO);
 *      status.set_state(state);
 *      status.set_working(working);
 * \endcode
 *
 * \param[in] state  The new state.
 * \param[in] working  The new working value.
 */
void path_info_t::status_t::reset_state(state_t state, working_t working)
{
    f_error = error_t::NO_ERROR;
    f_state = state;
    f_working = working;
}


/** \brief Change the current state of this status.
 *
 * This function can be used to save a new state in this status object.
 *
 * \note
 * This function does NOT affect the error state. This means that if the
 * status object has an error state other than error_t::NO, it is still
 * considered to be erroneous.
 *
 * \param[in] state  The new state of this status.
 */
void path_info_t::status_t::set_state(state_t state)
{
    f_state = state;
}


/** \brief Retrieve the current state.
 *
 * This function returns the current state of this status. The state is
 * set to unknown (path_info_t::status_t::UNKNOWN_STATE) by default if
 * no current_status is passed to the constructor.
 *
 * \return The current state.
 */
path_info_t::status_t::state_t path_info_t::status_t::get_state() const
{
    return f_state;
}


/** \brief Check whether the current state is unknown.
 *
 * When creating a new state object, the state is set to unknown by
 * default. It remains that way until you change it with set_state()
 * or reset_state().
 *
 * This function can be used to know whether the state is still set
 * to unknown.
 *
 * Note that is important because you cannot save an unknown state in
 * the database. The get_status() function will raise an exception
 * if that is attempted.
 */
bool path_info_t::status_t::is_unknown() const
{
    return f_state == state_t::UNKNOWN_STATE;
}


/** \brief Change the working state.
 *
 * This function is used to change the working state of the status object.
 *
 * The state can be set to any valid working state value, however, note
 * that the get_status() prevents a certain number of combinations such
 * as the working_t::CREATING working state with a state other than
 * state_t::CREATING.
 *
 * The default value of the working state is working_t::NO meaning that
 * the page is not being worked on.
 *
 * \note
 * So, this function allows any combinations to be generated, because
 * that way we do not enforce the use of the reset_state() function
 * or a specific order (i.e. change state first then working or
 * vice versa.)
 *
 * \param[in] working  The new working state.
 */
void path_info_t::status_t::set_working(working_t working)
{
    f_working = working;
}


/** \brief Retrieve the current working state.
 *
 * This function returns the current working state of this status.
 *
 * Note that if is_error() is returning true, then this working state
 * is not considered when calling the get_status() function.
 *
 * By default the working state is set to working_t::NO which means
 * that the page is not being worked on.
 *
 * \return The current status working state.
 */
path_info_t::status_t::working_t path_info_t::status_t::get_working() const
{
    return f_working;
}


/** \brief Indicate whether a process is currently working on that page.
 *
 * This function returns true if the current working status was
 * something else than path_info_t::status_t::NOT_WORKING.
 *
 * \return true if a process is working on this page.
 */
bool path_info_t::status_t::is_working() const
{
    return f_working != working_t::NOT_WORKING;
}




path_info_t::raii_status_t::raii_status_t(path_info_t& ipath, status_t now, status_t end)
    : f_ipath(ipath)
    , f_end(end)
{
    status_t current(f_ipath.get_status());

    // reset the error in case we are loading from a non-existant page
    if(current.is_error())
    {
        if(current.get_error() != status_t::error_t::UNDEFINED)
        {
            // the page probably exists, but we still got an error
            throw content_exception_content_invalid_state(QString("get error %1 when trying to change \"%2\" status.")
                    .arg(static_cast<int>(current.get_error()))
                    .arg(f_ipath.get_key()));
        }
        current.set_error(status_t::error_t::NO_ERROR);
    }

    // setup state if requested
    if(now.get_state() != status_t::state_t::UNKNOWN_STATE)
    {
        current.set_state(now.get_state());
    }

    // setup working state if requested
    if(now.get_working() != status_t::working_t::UNKNOWN_WORKING)
    {
        current.set_working(now.get_working());
    }

    f_ipath.set_status(current);
}


path_info_t::raii_status_t::~raii_status_t()
{
    status_t current(f_ipath.get_status());
    if(f_end.get_state() != status_t::state_t::UNKNOWN_STATE)
    {
        current.set_state(f_end.get_state());
    }
    if(f_end.get_working() != status_t::working_t::UNKNOWN_WORKING)
    {
        current.set_working(f_end.get_working());
    }
    f_ipath.set_status(current);
}










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


void path_info_t::set_path(QString const& path)
{
    if(path != f_cpath && path != f_key)
    {
        QString const& site_key(f_snap->get_site_key_with_slash());
        if(path.startsWith(site_key))
        {
            // already canonicalized
            f_key = path;
            f_cpath = path.mid(site_key.length());
        }
        else
        {
            // may require canonicalization
            f_cpath = path;
            f_snap->canonicalize_path(f_cpath);
            f_key = f_snap->get_site_key_with_slash() + f_cpath;
        }

        // retrieve the action from this path
        // (note that in case of the main page the action is NOT included)
        // f_action will be an empty string if no action was specified
        snap_uri const uri(f_key);
        QString const action(uri.query_option(server::server::instance()->get_parameter("qs_action")));
        if(!action.isEmpty())
        {
            set_parameter("action", action);
        }

        // the other info becomes invalid
        clear();
    }
}


void path_info_t::set_real_path(QString const& path)
{
    if(path != f_real_cpath && path != f_real_key)
    {
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
        // execpt for the parameters which we keep in place
        clear(true);
    }
}


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


void path_info_t::set_parameter(QString const& name, QString const& value)
{
    f_parameters[name] = value;
}


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


void path_info_t::force_extended_revision(QString const& revision, QString const& filename)
{
    snap_version::version v;
    if(!v.set_version_string(revision))
    {
        throw snap_logic_exception(QString("invalid version string (%1) in \"%2\" (force_extended_revision).").arg(revision).arg(filename));
    }
    snap_version::version_numbers_vector_t const& version_numbers(v.get_version());
    if(version_numbers.size() < 1)
    {
        throw snap_logic_exception(QString("invalid version string (%1) in \"%2\" (force_extended_revision): not enough numbers (at least 1 required).").arg(revision).arg(filename));
    }
    f_branch = version_numbers[0];
    f_revision = static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_EXTENDED); // FIXME cast

    // WARNING: the revision string includes the branch
    f_revision_string = v.get_version_string();
}


void path_info_t::force_locale(QString const& locale)
{
    // TBD: not too sure how valid this is...
    f_locale = locale;
}


void path_info_t::get_parent(path_info_t& parent_ipath) const
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


void path_info_t::get_child(path_info_t& parent_ipath, QString const& child) const
{
    // since the path will not include the domain name, it will get
    // canonicalized automatically
    parent_ipath.set_path(f_cpath + "/" + child);
}


snap_child *path_info_t::get_snap() const
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


QString path_info_t::get_real_cpath() const
{
    return f_real_cpath;
}


bool path_info_t::is_main_page() const
{
    return f_main_page;
}


QString path_info_t::get_parameter(QString const& name) const
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
    QtCassandra::QCassandraCell::pointer_t cell(content_table->row(f_key)->cell(get_name(SNAP_NAME_CONTENT_STATUS)));
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
void path_info_t::set_status(status_t const& status)
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
        processing_table->row(f_key)->cell(get_name(SNAP_NAME_CONTENT_STATUS_CHANGED))->setValue(one_byte);
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
    content_table->row(f_key)->cell(get_name(SNAP_NAME_CONTENT_STATUS_CHANGED))->setValue(changed);

    QtCassandra::QCassandraValue value;
    value.setUInt32Value(status.get_status());
    value.setConsistencyLevel(QtCassandra::CONSISTENCY_LEVEL_QUORUM);
    content_table->row(f_key)->cell(get_name(SNAP_NAME_CONTENT_STATUS))->setValue(value);
}


bool path_info_t::get_working_branch() const
{
    return f_main_page ? f_snap->get_working_branch() : false;
}


snap_version::version_number_t path_info_t::get_branch(bool create_new_if_required, QString const& locale) const
{
    if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
    {
        // FIXME cast
        f_branch = f_main_page
                    ? static_cast<snap_version::basic_version_number_t>(f_snap->get_branch())
                    : static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED);

        if(snap_version::SPECIAL_VERSION_UNDEFINED == f_branch)
        {
            QString const& key(f_real_key.isEmpty() ? f_key : f_real_key);
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
                    ? static_cast<snap_version::basic_version_number_t>(f_snap->get_revision())
                    : static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED);

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
        throw content_exception_data_missing(QString("get_branch_key() request failed for \"%1\", branch not defined").arg(f_cpath));
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
            // if f_revision is set to extended then the branch is already
            // defined, no need to call get_branch()
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
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(get_working_branch()
                                ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                                : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
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
                throw content_exception_data_missing(QString("get_revision_key() request failed for \"%1\", revision not defined").arg(f_cpath));
            }
        }
    }

    return f_revision_key;
}


QString path_info_t::get_extended_revision() const
{
    return f_revision_string;
}


void path_info_t::clear(bool keep_parameters)
{
    f_branch = static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED); // FIXME cast
    f_revision = static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED); // FIXME cast
    f_revision_string.clear();
    f_locale.clear();
    f_branch_key.clear();
    f_revision_key.clear();

    // in case of a set_real_path() we do not want to lose the parameters
    if(!keep_parameters)
    {
        f_parameters.clear();
    }
}





/** \brief Set the permission and reason for refusal.
 *
 * This function marks the permission flag as not permitted (i.e. it
 * sets it to false.) The default value of the permission flag is
 * true. Note that once this function was called once it is not possible
 * to set the flag back to true.
 *
 * \param[in] new_reason  The reason for the refusal, can be set to "".
 */
void permission_flag::not_permitted(QString const& new_reason)
{
    f_allowed = false;

    if(!new_reason.isEmpty())
    {
        if(!f_reason.isEmpty())
        {
            f_reason += "\n";
        }
        // TBD: should we prevent "\n" in "new_reason"?
        f_reason += new_reason;
    }
}








/** \brief Initialize the content plugin.
 *
 * This function is used to initialize the content plugin object.
 */
content::content()
    //: f_snap(nullptr) -- auto-init
{
}


/** \brief Clean up the content plugin.
 *
 * Ensure the content object is clean before it is gone.
 */
content::~content()
{
}


/** \brief Initialize the content.
 *
 * This function terminates the initialization of the content plugin
 * by registering for different events.
 *
 * \param[in] snap  The child handling this request.
 */
void content::on_bootstrap(snap_child *snap)
{
    f_snap = snap;

    SNAP_LISTEN0(content, "server", server, save_content);
    SNAP_LISTEN(content, "server", server, register_backend_action, _1);
    SNAP_LISTEN0(content, "server", server, backend_process);
}


/** \brief Get a pointer to the content plugin.
 *
 * This function returns an instance pointer to the content plugin.
 *
 * Note that you cannot assume that the pointer will be valid until the
 * bootstrap event is called.
 *
 * \return A pointer to the content plugin.
 */
content *content::instance()
{
    return g_plugin_content_factory.instance();
}


/** \brief Return the description of this plugin.
 *
 * This function returns the English description of this plugin.
 * The system presents that description when the user is offered to
 * install or uninstall a plugin on his website. Translation may be
 * available in the database.
 *
 * \return The description in a QString.
 */
QString content::description() const
{
    return "Manage nearly all the content of your website. This plugin handles"
        " your pages, the website taxonomy (tags, categories, permissions...)"
        " and much much more.";
}


/** \brief Check whether updates are necessary.
 *
 * This function updates the database when a newer version is installed
 * and the corresponding updates where not run.
 *
 * This works for newly installed plugins and older plugins that were
 * updated.
 *
 * \param[in] last_updated  The UTC Unix date when the website was last updated (in micro seconds).
 *
 * \return The UTC Unix date of the last update of this plugin.
 */
int64_t content::do_update(int64_t last_updated)
{
    SNAP_PLUGIN_UPDATE_INIT();

    SNAP_PLUGIN_UPDATE(2012, 1, 1, 0, 0, 0, initial_update);
    SNAP_PLUGIN_UPDATE(2013, 12, 25, 11, 19, 40, content_update);

    SNAP_PLUGIN_UPDATE_EXIT();
}


/** \brief First update to run for the content plugin.
 *
 * This function is the first update for the content plugin. It installs
 * the initial index page.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::initial_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);

    get_content_table();
    get_branch_table();
    get_revision_table();
    get_files_table();
}


/** \brief Update the database with our content references.
 *
 * Send our content to the database so the system can find us when a
 * user references our pages.
 *
 * \param[in] variables_timestamp  The timestamp for all the variables added to the database by this update (in micro-seconds).
 */
void content::content_update(int64_t variables_timestamp)
{
    static_cast<void>(variables_timestamp);
}


/** \brief Initialize the content table.
 *
 * This function creates the content table if it does not already exist.
 * Otherwise it simply initializes the f_content_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The content table is the one that includes the tree representing the
 * entire content of all the websites. Since tables can grow as big as
 * we want, this is not a concern. The content table looks like a tree
 * although each row represents one leaf at any one level (the row keys
 * are the site key with slash + path).
 *
 * The data in a row of the content table includes two branch and revision
 * references: the current branch/revision and the current working
 * branch revision. The working version is the one the website administrator
 * edits until it looks good and then publish that version so it becomes
 * the current branch/revision.
 *
 * Branch zero is special in that it is used by the system to define the
 * data from the various content.xml files (hard coded data.)
 *
 * Branch one and up are reserved for the user, although a few other branch
 * numbers are reserved to indicate errors.
 *
 * The revision information makes use of one entry for the current branch,
 * and one entry for the current revision per branch and language. This is
 * then repeated for the current working branch and revisions.
 *
 * \code
 * content::revision_control::current_branch = <branch>
 * content::revision_control::current_revision::<branch>::<language> = <revision>
 * content::revision_control::current_working_branch = <branch>
 * content::revision_control::current_working_revision::<branch>::<language> = <revision>
 * content::revision_control::last_revision::<branch>::<language> = <revision>
 * \endcode
 *
 * \return The pointer to the content table.
 */
QtCassandra::QCassandraTable::pointer_t content::get_content_table()
{
    if(!f_content_table)
    {
        f_content_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_TABLE), "Website content table.");
    }
    return f_content_table;
}


/** \brief Initialize the processing table.
 *
 * This function creates the processing table if it does not already exist.
 * Otherwise it simply initializes the f_processing_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The processing table is used to save all the URI of pages being processed
 * one way or the other. This allows the backend process to delete all
 * statuses (over 10 minutes old.)
 *
 * The data is set to the start date so we do not have to read anything
 * more to know whether we need to process that entry.
 *
 * \return The pointer to the content table.
 */
QtCassandra::QCassandraTable::pointer_t content::get_processing_table()
{
    if(!f_processing_table)
    {
        f_processing_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_PROCESSING_TABLE), "Website content table.");
    }
    return f_processing_table;
}


/** \brief Initialize the branch table.
 *
 * This function creates the branch table if it does not exist yet. Otherwise
 * it simple initializes the f_branch_table variable member before returning
 * it.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The branch table is the one that includes the links of the page at
 * a specific branch level (links cannot be defined on a per revision basis.)
 * It is referenced from the content table for the current branch and
 * current working branch. Older branches may be accessed by using branch
 * identifiers smaller than the largest branch in existance (i.e.
 * content::current_working_branch in most cases.) Intermediate branches
 * may have been deleted (in most cases because they were so old.)
 *
 * The branch table is similar to the content table in that it looks like a
 * tree although it includes one row per branch.
 *
 * The key used by a branch is defined as follow:
 *
 * \code
 * <site-key>/<path>#<branch>
 * \endcode
 *
 * The '#' is used because it cannot appear in a path (i.e. the browser
 * cannot send you a request with a # in it, it is not legal.)
 *
 * The content table has references to the current branch and the current
 * working branch as follow:
 *
 * \code
 * content::revision_control::current_branch_key = <site-key>/<path>#1
 * content::revision_control::current_working_branch_key = <site-key>/<path>#1
 * \endcode
 *
 * Note that for attachments we do use a language, most often "xx", but there
 * are pictures created with text on them and thus you have to have a
 * different version for each language for pictures too.
 *
 * Note that \<language> never represents a programming language here. So if
 * an attachment is a JavaScript file, the language can be set to "en" if
 * it includes messages in English, but it is expected that all JavaScript
 * files be assigned language "xx". This also applies to CSS files which are
 * likely to all be set to "xx".
 *
 * \return The pointer to the branch table.
 */
QtCassandra::QCassandraTable::pointer_t content::get_branch_table()
{
    if(!f_branch_table)
    {
        f_branch_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_BRANCH_TABLE), "Website branch table.");
    }
    return f_branch_table;
}


/** \brief Initialize the revision table.
 *
 * This function creates the revision table if it does not exist yet.
 * Otherwise it simple initializes the f_revision_table variable member
 * and returns its value.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The revision table is the one that includes the actual content of the
 * websites. It is referenced from the content table for the current
 * revision and current working revision. Older revisions can be listed
 * or tried with the exists() function.
 *
 * The revision table is similar to the content table in that it looks like
 * a tree although it includes one row per revision.
 *
 * The key is defined as follow:
 *
 * \code
 * <site-key>/<path>#<language>/<branch>.<revision>
 * \endcode
 *
 * The content table includes a couple of revision references: the
 * current revision and the current working revision.
 *
 * \code
 * content::revision_control::current_revision_key::<branch>::<language> = <site-key>/<path>#<language>/<branch>.<revision>
 * content::revision_control::current_working_revision_key::<branch>::<language> = <site-key>/<path>#<language>/<branch>.<revision>
 * \endcode
 *
 * Note that \<language> never represents a programming language here.
 *
 * \return The pointer to the revision table.
 */
QtCassandra::QCassandraTable::pointer_t content::get_revision_table()
{
    if(!f_revision_table)
    {
        f_revision_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_REVISION_TABLE), "Website data table.");
    }
    return f_revision_table;
}


/** \brief Initialize the files table.
 *
 * This function creates the files table if it doesn't exist yet. Otherwise
 * it simple initializes the f_files_table variable member.
 *
 * If the function is not able to create the table an exception is raised.
 *
 * The table is used to list all the files from all the websites managed
 * by this Snap! server. Note that the files are listed for all the
 * websites, by website & filename, when new and need to be checked
 * (anti-virus, etc.) and maybe a few other things later.
 *
 * \li Rows are MD5 sums of the files, this is used as the key in the
 *     content table
 * \li '*new*' includes MD5 sums of files to be checked (anti-virus, ...)
 * \li '*index*' lists of files by 'site key + filename'
 *
 * \return The pointer to the files table.
 */
QtCassandra::QCassandraTable::pointer_t content::get_files_table()
{
    if(!f_files_table)
    {
        f_files_table = f_snap->create_table(get_name(SNAP_NAME_CONTENT_FILES_TABLE), "List of all the files ever uploaded to all the websites.");
    }
    return f_files_table;
}


/** \brief Retrieve the snap_child pointer.
 *
 * This function returns the snap_child object pointer. It is generally used
 * internally by sub-classes to gain access to the outside world.
 *
 * \return A pointer to the snap_child running this process.
 */
snap_child *content::get_snap()
{
    if(!f_snap)
    {
        // in case someone tries to get this while in the on_bootstrap()
        // function (which should not happen...)
        throw content_exception_content_not_initialized("content::get_snap() called before f_snap got initialized");
    }
    return f_snap;
}


/** \brief Call if a revision control version is found to be invalid.
 *
 * While dealing with revision control information, this function may
 * be called if a branch or revision number if found to be incorrect.
 *
 * \note
 * Debug code should not call this function. Instead it should
 * throw an error which is much more effective to talk to programmers.
 *
 * \param[in] version  The version that generated the error.
 */
void content::invalid_revision_control(QString const& version)
{
    f_snap->die(snap_child::HTTP_CODE_INTERNAL_SERVER_ERROR, "Invalid Revision Control",
            "The revision control \"" + version + "\" does not look valid.",
            "The version does not seem to start with a valid decimal number.");
    NOTREACHED();
}


/** \brief Get the current branch.
 *
 * This function retrieves the current branch for data defined in a page.
 * The current branch is determined using the key of the page being
 * accessed.
 *
 * \note
 * The current branch number may not be the last branch number. The system
 * automatically forces branch 1 to become current when created. However,
 * the system does not set the newest branch as current when the user creates
 * a new branch. This way a new branch remains hidden until the user decides
 * that it should become current.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_branch(QString const& key, bool working_branch)
{
    QString current_branch_key(QString("%1::%2")
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(working_branch
                                ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH
                                : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH)));
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(current_branch_key))
    {
        return content_table->row(key)->cell(current_branch_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Retrieve the current branch or create a new one.
 *
 * This function retrieves the current user branch which means it returns
 * the current branch as is unless it is undefined or is set to the
 * system branch. In those two cases the function creates a new branch.
 *
 * The function does not change the current branch information.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The locale to create the first revision of that branch.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \sa get_current_branch()
 * \sa get_new_branch()
 */
snap_version::version_number_t content::get_current_user_branch(QString const& key, QString const& locale, bool working_branch)
{
    snap_version::version_number_t branch(get_current_branch(key, working_branch));
    if(snap_version::SPECIAL_VERSION_UNDEFINED == branch
    || snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch)
    {
        // not a valid user branch, first check whether there is a latest
        // user branch, if so, put the new data on the newest branch
        QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

        // get the last branch number
        QString const last_branch_key(QString("%1::%2")
                            .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                            .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(!branch_value.nullValue())
        {
            // a branch exists, although it may still be a system branch
            branch = branch_value.uint32Value();
        }

        if(snap_version::SPECIAL_VERSION_UNDEFINED == branch
        || snap_version::SPECIAL_VERSION_SYSTEM_BRANCH == branch)
        {
            // well... no user branch exists yet, create one
            return get_new_branch(key, locale);
        }
    }

    return branch;
}


/** \brief Get the current revision.
 *
 * This function retrieves the current revision for data defined in a page.
 * The current branch is determined using the get_current_branch()
 * function with the same key, owner, and working_branch parameters.
 *
 * \note
 * The current revision number may have been changed by an editor to a
 * number other than the last revision number.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] branch  The branch number.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_revision(QString const& key, snap_version::version_number_t const branch, QString const& locale, bool working_branch)
{
    QString revision_key(QString("%1::%2::%3")
            .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
            .arg(get_name(working_branch
                    ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION
                    : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
            .arg(branch));
    if(!locale.isEmpty())
    {
        revision_key += "::" + locale;
    }
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(revision_key))
    {
        return content_table->row(key)->cell(revision_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Get the current revision.
 *
 * This function retrieves the current revision for data defined in a page.
 * The current branch is determined using the get_current_branch()
 * function with the same key, owner, and working_branch parameters.
 *
 * \note
 * The current revision number may have been changed by an editor to a
 * number other than the last revision number.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  Whether the working branch (true) or the current
 *                            branch (false) is used.
 *
 * \return The current revision number.
 */
snap_version::version_number_t content::get_current_revision(QString const& key, QString const& locale, bool working_branch)
{
    snap_version::version_number_t const branch(get_current_branch(key, working_branch));
    QString revision_key(QString("%1::%2::%3")
            .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
            .arg(get_name(working_branch
                    ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION
                    : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
            .arg(branch));
    if(!locale.isEmpty())
    {
        revision_key += "::" + locale;
    }
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key)
    && content_table->row(key)->exists(revision_key))
    {
        return content_table->row(key)->cell(revision_key)->value().uint32Value();
    }

    return snap_version::SPECIAL_VERSION_UNDEFINED;
}


/** \brief Generate a new branch number and return it.
 *
 * This function generates a new branch number and returns it. This is used
 * each time the user requests to create a new branch.
 *
 * In most cases a user will create a new branch when he wants to be able to
 * continue to update the current branch until he is done with the new branch
 * of that page. This way the new branch can be written and moderated and
 * scheduled for publication on a future date without distrubing what visitors
 * see when they visit that page.
 *
 * The locale is used to generate the first revision of that branch. In most
 * cases this allows you to use revision 0 without having to request a new
 * revision by calling the get_new_revision() function (i.e. an early
 * optimization.) If empty, then no translations will be available for that
 * revision and no locale is added to the field name. This is different from
 * setting the locale to "xx" which still allows translation only this one
 * entry is considered neutral in terms of language.
 *
 * \note
 * Branch zero (0) is never created using this function. If no branch
 * exists this function returns one (1) anyway. This is because branch
 * zero (0) is reserved and used by the system when it saves the parameters
 * found in the content.xml file.
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] locale  The locale used for this revision.
 *
 * \return The new branch number.
 *
 * \sa get_current_user_branch()
 */
snap_version::version_number_t content::get_new_branch(QString const& key, QString const& locale)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    // get the last branch number
    QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));

    // increase revision if one exists, otherwise we keep the user default (1)
    snap_version::version_number_t branch(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_USER_FIRST_BRANCH));

    QtCassandra::QCassandraLock lock(f_snap->get_context(), key);

    QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
    if(!branch_value.nullValue())
    {
        // it exists, increase it
        branch = branch_value.uint32Value();
        if(snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER > branch)
        {
            ++branch;
        }
        // else -- probably need to warn the user we reached 4 billion
        //         branches (this is pretty much impossible without either
        //         hacking the database or having a robot that generates
        //         many branches every day.)
    }
    content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));

    QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(branch));
    if(!locale.isEmpty())
    {
        last_revision_key += "::" + locale;
    }
    content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));

    // unlock ASAP
    lock.unlock();

    return branch;
}


/** \brief Copy a branch in another.
 *
 * This function is generally used when a user creates a new branch on
 * a page where another branch already exists.
 *
 * The function checks whether the source branch exists. If not, it
 * silently returns.
 *
 * The destination may already have some parameters. The copy attempts to
 * not modify existing data in the destination branch.
 *
 * The destination receives the content::created field as of this process
 * instance (i.e. snap_child::get_start_date()) unless the field already
 * exists.
 *
 * \param[in] key  The page where the branch is copied.
 * \param[in] source_branch  The branch that gets copied.
 * \param[in] destination_branch  The new branch you just "created" (the
 *                                branch itself does not need to already
 *                                exist in the database.)
 */
void content::copy_branch(QString const& key, snap_version::version_number_t const source_branch, snap_version::version_number_t const destination_branch)
{
    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());

    if(source_branch >= destination_branch)
    {
        throw snap_logic_exception(QString("trying to copy a newer branch (%1) in an older one (%2)")
                    .arg(source_branch)
                    .arg(destination_branch));
    }

    path_info_t source_uri;
    source_uri.set_path(key);
    source_uri.force_branch(source_branch);

    QtCassandra::QCassandraRow::pointer_t source_row(branch_table->row(source_uri.get_branch_key()));
    if(!source_row->exists(get_name(SNAP_NAME_CONTENT_CREATED)))
    {
        // no source, ignore
        return;
    }

    path_info_t destination_uri;
    destination_uri.set_path(key);
    destination_uri.force_branch(destination_branch);

    QtCassandra::QCassandraRow::pointer_t destination_row(branch_table->row(destination_uri.get_branch_key()));

    QString const links_namespace(QString("%1::").arg(links::get_name(links::SNAP_NAME_LINKS_NAMESPACE)));
    QByteArray const links_bytearray(links_namespace.toAscii());

    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        source_row->clearCache();
        source_row->readCells(column_predicate);
        QtCassandra::QCassandraCells source_cells(source_row->cells());
        if(source_cells.isEmpty())
        {
            // done
            break;
        }
        copy_branch_cells(source_cells, destination_row, destination_branch);
    }
}


bool content::copy_branch_cells_impl(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, snap_version::version_number_t const destination_branch)
{
    // we handle the links here because the links cannot include the
    // content.h header file...
    links::links *link_plugin(links::links::instance());
    QString const links_namespace(QString("%1::").arg(links::get_name(links::SNAP_NAME_LINKS_NAMESPACE)));
    QByteArray const links_bytearray(links_namespace.toAscii());

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key == get_name(SNAP_NAME_CONTENT_MODIFIED)
        || destination_row->exists(cell_key))
        {
            // ignore the content::modified cell
            // ignore all the cells that already exist in the destination
            //
            // (TBD: we may want to limit those to content::... and
            //       links::... cells and leave the decision to each plugin
            //       for the others?)
            continue;
        }

        if(cell_key == get_name(SNAP_NAME_CONTENT_CREATED))
        {
            // handle the content::created field
            int64_t const now(f_snap->get_start_date());
            destination_row->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(now);
        }
        else if(cell_key.startsWith(links_bytearray))
        {
            // handle the links as a special case because the links plugin
            // cannot include content.h (circular includes...)
            link_plugin->fix_branch_copy_link(source_cell, destination_row, destination_branch);
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;

    // continue process if there are still cells to handle
    // (often false since content::... and links::... were already worked on)
    return !source_cells.isEmpty();
}


void content::copy_branch_cells_as_is(QtCassandra::QCassandraCells& source_cells, QtCassandra::QCassandraRow::pointer_t destination_row, QString const& plugin_namespace)
{
    QString const cell_namespace(QString("%1::").arg(plugin_namespace));
    QByteArray const cell_bytearray(cell_namespace.toAscii());

    QtCassandra::QCassandraCells left_cells;

    // handle one batch
    for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
            nc != source_cells.end();
            ++nc)
    {
        QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
        QByteArray cell_key(source_cell->columnKey());

        if(cell_key.startsWith(cell_bytearray))
        {
            // copy our fields as is
            destination_row->cell(cell_key)->setValue(source_cell->value());
        }
        else
        {
            // keep the other branch fields as is, other plugins can handle
            // them as required by implementing this signal
            //
            // note that the map is a map a shared pointers so it is fast
            // to make a copy like this
            left_cells[cell_key] = source_cell;
        }
    }

    // overwrite the source with the cells we allow to copy "further"
    source_cells = left_cells;
}


/** \brief Generate a new revision number and return it.
 *
 * This function generates a new revision number and returns it. This is used
 * each time the system or a user saves a new revision of content to a page.
 *
 * The function takes in the branch in which the new revision is to be
 * generated which means the locale needs to also be specified. However,
 * it is possible to set the locale parameter to the empty string in which
 * case the data being revisioned cannot be translated. Note that this is
 * different from setting the value to "xx" since in that case it means
 * that specific entry is neutral whereas using the empty string prevents
 * translations altogether (because the language/country are not taken
 * in account.)
 *
 * The \p repeat parameter is used to determine whether the data is expected
 * to be copied from the previous revision if there is one. Note that at this
 * time no data gets automatically copied if you create a new revision for a
 * new language. We will most certainly change that later so we can copy the
 * data from a default language such as "xx" or "en"...
 *
 * Note that the repeated data includes the date when the entry gets created.
 * The entry is adjusted to use the start date of the child process, which
 * means that you do not have to re-update the creation time of the revision
 * after this call. However, this function does NOT update the branch last
 * modification time. To do so, make sure to call the content_modified()
 * function once you are done with your changes.
 *
 * \note
 * In debug mode the branch number is verified for validity. It has to
 * be an existing branch.
 *
 * \note
 * This function may return zero (0) if the concerned locale did not
 * yet exist for this page.
 *
 * \todo
 * We probably should be using the path_info_t class to generate the
 * URIs. At this point these are done "by hand" here.
 *
 * \todo
 * We may want to create a class that allows us to define a set of the new
 * fields so instead of copying we can immediately save the new value. Right
 * now we are going to write the same field twice (once here in the repeat
 * to save the old value and once by the caller to save the new value.)
 *
 * \param[in] key  The key of the page concerned.
 * \param[in] branch  The branch for which this new revision is being created.
 * \param[in] locale  The locale used for this revision.
 * \param[in] repeat  Whether the existing data should be duplicated in the
 *                    new revision.
 *
 * \return The new revision number.
 */
snap_version::version_number_t content::get_new_revision(QString const& key,
                snap_version::version_number_t const branch,
                QString const& locale, bool repeat,
                snap_version::version_number_t const old_branch)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    snap_version::version_number_t const previous_branch(
        old_branch == static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED)
            ? branch
            : old_branch);

    // define the key
    QString last_revision_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                .arg(branch));
    if(!locale.isEmpty())
    {
        last_revision_key += "::" + locale;
    }
    QString current_revision_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION))
                .arg(previous_branch));
    if(!locale.isEmpty())
    {
        current_revision_key += "::" + locale;
    }

    // increase revision if one exists, otherwise we keep the default (0)
    snap_version::version_number_t revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));

    QtCassandra::QCassandraLock lock(f_snap->get_context(), key);

#ifdef DEBUG
    {
        // verify correctness of branch
        QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(!branch_value.nullValue()
        && branch > branch_value.uint32Value())
        {
            // the 'branch' parameter cannot be larger than the last branch allocated
            throw snap_logic_exception(QString("trying to create a new revision for branch %1 which does not exist (last branch is %2)")
                        .arg(branch).arg(branch_value.uint32Value()));
        }
    }
#endif

    QtCassandra::QCassandraValue revision_value(content_table->row(key)->cell(last_revision_key)->value());
    if(!revision_value.nullValue())
    {
        // it exists, increase it
        revision = revision_value.uint32Value();
        if(snap_version::SPECIAL_VERSION_MAX_BRANCH_NUMBER > revision)
        {
            ++revision;
        }
        // else -- probably need to warn the user we reached 4 billion
        //         revisions (this is assuming we delete old revisions
        //         in the meantime, but even if you make 10 changes a
        //         day and say it makes use of 20 revision numbers each
        //         time, it would still take... over half a million
        //         YEARS to reach that many revisions in that one
        //         branch...)
    }
    content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));

    // copy from the current revision at this point
    // (the editor WILL tell us to copy from a specific revisions at some
    // point... it is important because if user A edits revision X, and user
    // B creates a new revision Y in the meantime, we may still want to copy
    // revision X at the time A saves his changes.)
    snap_version::version_number_t previous_revision(revision);
    QtCassandra::QCassandraValue current_revision_value(content_table->row(key)->cell(current_revision_key)->value());
    if(!current_revision_value.nullValue())
    {
        previous_revision = current_revision_value.uint32Value();
    }

    // TBD: should the repeat be done before or after the lock?
    //      it seems to me that since the next call will now generate a
    //      new revision, it is semi-safe (problem is that the newer
    //      version may miss some of the fields...)
    //      also the caller will lose the lock too!

    if(repeat
    && (revision != static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION)
       || old_branch != static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED))
    && previous_revision != revision)
    {
        // get two revision keys like:
        // http://csnap.m2osw.com/verify-credentials#en/0.2
        // and:
        // http://csnap.m2osw.com/verify-credentials#en/0.3
        QString const previous_revision_key(generate_revision_key(key, previous_branch, previous_revision, locale));
        QString const revision_key(generate_revision_key(key, branch, revision, locale));
        QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());

        dbutils::copy_row(revision_table, previous_revision_key, revision_table, revision_key);

        // change the creation date
        QtCassandra::QCassandraValue created;
        created.setInt64Value(f_snap->get_start_date());
        revision_table->row(revision_key)->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(created);
    }

    // unlock ASAP
    lock.unlock();

    return revision;
}


/** \brief Generate a key from a branch, revision, and locale.
 *
 * This function transforms a page key and a branch number in a key that
 * is to be used to access the user information in the data table.
 *
 * The branch is used as is in the key because it is very unlikely that
 * can cause a problem as all the other extended keys do not start with
 * a number.
 *
 * \param[in] key  The key of the page being worked on.
 * \param[in] working_branch  Whether we use the working branch or not.
 *
 * \return A string representing the branch key in the data table.
 */
QString content::get_branch_key(QString const& key, bool working_branch)
{
    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY)));

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue const value(content_table->row(key)->cell(current_key)->value());
    return value.stringValue();
}


/** \brief Generate the key to use in the data table for a branch.
 *
 * This function generates the key of the row used in the data table
 * to access branch specific data, whatever the revision.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 *
 * \return This function returns a copy of the current branch key.
 */
QString content::generate_branch_key(QString const& key, snap_version::version_number_t branch)
{
    return QString("%1#%2").arg(key).arg(branch);
}


/** \brief Set the current (working) branch.
 *
 * This function is used to save the \p branch. This is rarely used since
 * in most cases the branch is created when getting a new branch.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 * \param[in] working_branch  Update the current working branch (true) or the current branch (false).
 *
 * \return This function returns a copy of the current branch key.
 */
void content::set_branch(QString const& key, snap_version::version_number_t branch, bool working_branch)
{
    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH)));

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));

    // Last branch
    QString const last_branch_key(QString("%1::%2")
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
    QtCassandra::QCassandraValue last_branch_value(content_table->row(key)->cell(last_branch_key)->value());
    if(last_branch_value.nullValue())
    {
        // last branch does not exist yet, create it
        content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));
    }
    else
    {
        snap_version::version_number_t const last_branch(last_branch_value.uint32Value());
        if(branch > last_branch)
        {
            content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch));
        }
    }
}


/** \brief Set the current (working) branch key.
 *
 * This function is used to mark that \p branch is now the current branch or
 * the current working branch.
 *
 * The current branch is the one shown to your anonymous visitors. By default
 * only editors can see the other branches and revisions.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The number of the new current branch.
 * \param[in] working_branch  Update the current working branch (true) or the current branch (false).
 *
 * \return This function returns a copy of the current branch key.
 */
QString content::set_branch_key(QString const& key, snap_version::version_number_t branch, bool working_branch)
{
    // key in the data table
    QString const current_branch_key(generate_branch_key(key, branch));

    // key in the content table
    QString const current_key(QString("%1::%2")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY)));

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_branch_key);
    return current_branch_key;
}


/** \brief Initialize the system branch for a specific key.
 *
 * This function initializes all the branch values for the specified
 * path. This is used by the system to initialize a system branch.
 *
 * \todo
 * We have to initialize branches and a similar function for user content
 * will be necessary. User content starts with branch 1. I'm not entirely
 * sure anything more is required than having a way to specify the branch
 * on the call...
 *
 * \param[in] key  The path of the page concerned.
 */
void content::initialize_branch(QString const& key)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    // *** BRANCH ***
    snap_version::version_number_t branch_number(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH));
    {
        // Last branch
        QString const last_branch_key(QString("%1::%2")
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                        .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(last_branch_key)->value());
        if(branch_value.nullValue())
        {
            // last branch does not exist yet, create it with zero (0)
            content_table->row(key)->cell(last_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
        else
        {
            branch_number = branch_value.uint32Value();
        }
    }

    {
        QString const current_branch_key(QString("%1::%2")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(current_branch_key)->value());
        if(branch_value.nullValue())
        {
            content_table->row(key)->cell(current_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
    }

    {
        QString const current_branch_key(QString("%1::%2")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH)));
        QtCassandra::QCassandraValue branch_value(content_table->row(key)->cell(current_branch_key)->value());
        if(branch_value.nullValue())
        {
            content_table->row(key)->cell(current_branch_key)->setValue(static_cast<snap_version::basic_version_number_t>(branch_number));
        }
    }

    {
        // Current branch key
        QString const current_branch_key(get_branch_key(key, false));
        if(current_branch_key.isEmpty())
        {
            // there is no branch yet, create one
            set_branch_key(key, branch_number, false);
        }
    }

    {
        // Current working branch key
        QString const current_branch_key(get_branch_key(key, true));
        if(current_branch_key.isEmpty())
        {
            // there is no branch yet, create one
            set_branch_key(key, branch_number, true);
        }
    }
}


/** \brief Generate a key from a branch, revision, and locale.
 *
 * This function transforms a page key, a branch number, a revision number,
 * and a locale (\<language> or \<language>_\<country>) to a key that
 * is to be used to access the user information in the data table.
 *
 * \param[in] key  The key of the page being worked on.
 * \param[in] branch  The concerned branch.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current working branch (true).
 *
 * \return A string representing the end of the key
 */
QString content::get_revision_key(QString const& key, snap_version::version_number_t branch, QString const& locale, bool working_branch)
{
    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue const value(content_table->row(key)->cell(current_key)->value());
    return value.stringValue();
}


/** \brief Generate the data table key from different parameters.
 *
 * This function generates a data table key using the path to the data
 * (key), the branch and revision, and the locale (language and country).
 * The locale parameter is not mandatory. If empty, then no locale is
 * added to the key. This is legal for any data that cannot be translated.
 *
 * The resulting key looks like:
 *
 * \code
 * <path>#<language>_<country>/<branch>.<revision>
 * \endcode
 *
 * The the language and country being optional. If language is not specified
 * then no country can be specified. The slash is not added when no language
 * is specified.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch this revision is being saved for.
 * \param[in] revision  The new revision number.
 * \param[in] locale  The language and country information.
 *
 * \return The revision key as used in the data table.
 */
QString content::generate_revision_key(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale)
{
    if(locale.isEmpty())
    {
        return QString("%1#%2.%3").arg(key).arg(branch).arg(revision);
    }

    return QString("%1#%2/%3.%4").arg(key).arg(locale).arg(branch).arg(revision);
}


/** \brief Generate the data table key from different parameters.
 *
 * This function generates a data table key using the path to the data
 * (key), a predefined revision, and the locale (language and country).
 * The locale parameter is not mandatory. If empty, then no locale is
 * added to the key. This is legal for any data that cannot be translated.
 *
 * This function is used whenever your revision number is managed by
 * you and not by the content system. For example the JavaScript and
 * CSS attachment files are read for a Version field. That version may
 * use a different scheme than the normal system version limited to
 * a branch and a revision number. (Although our system is still
 * limited to only numbers, so a version such as 3.5.7b is not supported
 * as is.)
 *
 * The resulting key looks like:
 *
 * \code
 * <path>#<language>_<country>/<revision>
 * \endcode
 *
 * The the language and country being optional. If language is not specified
 * then no country can be specified. The slash is not added when no language
 * is specified.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] revision  The new branch and revision as a string.
 * \param[in] locale  The language and country information.
 *
 * \return The revision key as used in the data table.
 */
QString content::generate_revision_key(QString const& key, QString const& revision, QString const& locale)
{
    if(locale.isEmpty())
    {
        return QString("%1#%2").arg(key).arg(revision);
    }

    return QString("%1#%2/%3").arg(key).arg(locale).arg(revision);
}


/** \brief Save the revision as current.
 *
 * This function saves the specified \p revision as the current revision.
 * The function takes a set of parameters necessary to generate the
 * key of the current revision.
 *
 * \param[in] key  The page concerned.
 * \param[in] branch  The branch which current revision is being set.
 * \param[in] revision  The revision being set as current.
 * \param[in] locale  The locale (\<language>_\<country>) or an empty string.
 * \param[in] working_branch  Whether this is the current branch (true)
 *                            or current working branch (false).
 */
void content::set_current_revision(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale, bool working_branch)
{
    // revision key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION))
                .arg(branch));

    // key to the last revision
    QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(branch));

    if(!locale.isEmpty())
    {
        // append locale if defined
        current_key += "::" + locale;
        last_revision_key += "::" + locale;
    }

    // get the data key from the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));

    // avoid changing the revision if defined and larger or equal
    QtCassandra::QCassandraValue const revision_value(content_table->row(key)->cell(last_revision_key)->value());
    if(revision_value.nullValue())
    {
        // last revision does not exist yet, create it
        content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));
    }
    else
    {
        snap_version::version_number_t const last_revision(revision_value.uint32Value());
        if(revision > last_revision)
        {
            content_table->row(key)->cell(last_revision_key)->setValue(static_cast<snap_version::basic_version_number_t>(revision));
        }
    }
}


/** \brief Set the current (working) revision key.
 *
 * This function saves the current revision key or current working revision
 * key in the database as a string. This is the string used when people access
 * the data (read-only mode).
 *
 * This function is often called when creating a new revision key as the
 * user, in most cases, will want the latest revision to become the current
 * revision.
 *
 * You may call the generate_revision_key() function to regenerate the
 * revision key without saving it in the database too.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch this revision is being saved for.
 * \param[in] revision  The new revision number.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current working branch (true).
 *
 * \return This function returns a copy of the revision key just computed.
 */
QString content::set_revision_key(QString const& key, snap_version::version_number_t branch, snap_version::version_number_t revision, QString const& locale, bool working_branch)
{
    // key in the data table
    QString const current_revision_key(generate_revision_key(key, branch, revision, locale));

    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_revision_key);
    return current_revision_key;
}


/** \brief Save a revision key from a revision string.
 *
 * This function is used when the branching mechanism is used with a scheme
 * that does not follow the internal \<branch>.\<revision> scheme. For example
 * a JavaScript source must define a version and that version most often will
 * have 2 or 3 numbers ([0-9]+) separated by periods (.). These are handled
 * with this function.
 *
 * You may call the generate_revision_key() function to regenerate the
 * revision key without saving it in the database too.
 *
 * \param[in] key  The key to the page concerned.
 * \param[in] branch  The branch of the page.
 * \param[in] revision  The new revision string.
 * \param[in] locale  The language and country information.
 * \param[in] working_branch  The current branch (false) or the current
 *                            working branch (true).
 *
 * \return A copy of the current revision key saved in the database.
 */
QString content::set_revision_key(QString const& key, snap_version::version_number_t branch, QString const& revision, QString const& locale, bool working_branch)
{
    // key in the data table
    QString const current_revision_key(generate_revision_key(key, revision, locale));

    // key in the content table
    QString current_key(QString("%1::%2::%3")
                .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                .arg(get_name(working_branch
                        ? SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY
                        : SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY))
                .arg(branch));
    if(!locale.isEmpty())
    {
        current_key += "::" + locale;
    }

    // save the data key in the content table
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    content_table->row(key)->cell(current_key)->setValue(current_revision_key);
    return current_revision_key;
}


/** \brief Generate a key from a branch and user identifier.
 *
 * This function creates a key from the page key, a branch number, and a
 * user identifier. These keys are used to save drafts. Drafts are not
 * revisioned, however, they are assigned to specific users and the system
 * can detect whether a draft is older than the latest revision of a branch.
 *
 * \todo
 * Move to the user plugin?
 *
 * \param[in] key  The key of the page being handled.
 * \param[in] branch  The branch this draft is linked with.
 * \param[in] identifier  The user identifier.
 *
 * \return The key to the user branch.
 */
QString content::get_user_key(QString const& key, snap_version::version_number_t branch, int64_t identifier)
{
    return QString("%1#user/%2/%3").arg(key).arg(identifier).arg(branch);
}


/** \brief Create a page at the specified path.
 *
 * This function creates a page in the database at the specified path.
 * The page will be ready to be used once all the plugins had a chance
 * to run their own on_create_content() function.
 *
 * Note that if the page (as in, the row as defined by the path) already
 * exists then the function returns immediately.
 *
 * The full key for the page makes use of the site key which cannot already
 * be included in the path.
 *
 * The type of a new page must be specified. By default, the type is set
 * to "page". Specific modules may offer additional types. The three offered
 * by the content plugin are:
 *
 * \li "page" -- a standard user page.
 * \li "administration-page" -- in general any page under /admin.
 * \li "system-page" -- a page created by the content.xml which is not under
 *                      /admin.
 *
 * The page type MUST be just the type. It may be a path since a type
 * of page may be a sub-type of an basic type. For example, a "blog"
 * type would actually be a page and thus the proper type to pass to
 * this function is "page/blog" and not a full path or just "blog".
 * We force you in this way so any plugin can test the type without
 * having to frantically test all sorts of cases.
 *
 * The create function always generates  a new revision. If the specified
 * branch exists, then the latest revision + 1 is used. Otherwise, revision
 * zero (0) is used. When the system creates content it always uses
 * SPECIAL_VERSION_SYSTEM_BRANCH as the branch number (which is zero).
 *
 * \param[in] ipath  The path of the new page.
 * \param[in] owner  The name of the plugin that is to own this page.
 * \param[in] type  The type of page.
 *
 * \return true if the signal is to be propagated.
 */
bool content::create_content_impl(path_info_t& ipath, QString const& owner, QString const& type)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const key(ipath.get_key());

    // create the row
    QString const primary_owner(get_name(SNAP_NAME_CONTENT_PRIMARY_OWNER));
    QtCassandra::QCassandraRow::pointer_t row(content_table->row(key));
    if(row->exists(primary_owner))
    {
        // the row already exists, this is considered created.
        // (we may later want to have a repair_content signal
        // which we could run as an action from the backend...)
        // however, if it were created by an add_xml() call,
        // then the on_create_content() of all the other plugins
        // should probably be called (i.e. f_updating is true then)
        return f_updating;
    }

    // note: we do not need to test whether the home page ("") allows
    //       for children; if not we'd have a big problem!
    if(!ipath.get_cpath().isEmpty())
    {
        // parent path is the path without the last "/..." part
        int const pos(ipath.get_cpath().lastIndexOf('/'));
        if(pos >= 0)
        {
            QString const parent_key(site_key + ipath.get_cpath().left(pos));
            if(is_final(parent_key))
            {
                // the user was trying to add content under a final leaf
                f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Final Parent",
                        QString("Page \"%1\" cannot be added under \"%2\" since \"%2\" is marked as final.")
                                    .arg(key).arg(parent_key),
                        "The parent row does not allow for further children.");
                NOTREACHED();
            }
        }
    }

    // first, we want to save the status
    path_info_t::status_t status(ipath.get_status());
    status.reset_state(path_info_t::status_t::state_t::CREATE, path_info_t::status_t::working_t::CREATING);
    ipath.set_status(status);

    // save the owner
    row->cell(primary_owner)->setValue(owner);

    snap_version::version_number_t const branch_number(ipath.get_branch());

    set_branch(key, branch_number, false);
    set_branch(key, branch_number, true);
    set_branch_key(key, branch_number, true);
    set_branch_key(key, branch_number, false);

    snap_version::version_number_t const revision_number(ipath.get_revision());
    if(revision_number != snap_version::SPECIAL_VERSION_UNDEFINED
    && revision_number != snap_version::SPECIAL_VERSION_INVALID
    && revision_number != snap_version::SPECIAL_VERSION_EXTENDED)
    {
        QString const locale(ipath.get_locale());
        set_current_revision(key, branch_number, revision_number, locale, false);
        set_current_revision(key, branch_number, revision_number, locale, true);
        set_revision_key(key, branch_number, revision_number, locale, true);
        set_revision_key(key, branch_number, revision_number, locale, false);
    }

    // add the different basic content dates setup
    int64_t const start_date(f_snap->get_start_date());
    row->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(start_date);

    //QString const branch_key(generate_branch_key(key, branch_number));
    QtCassandra::QCassandraRow::pointer_t data_row(branch_table->row(ipath.get_branch_key()));
    data_row->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
    data_row->cell(get_name(SNAP_NAME_CONTENT_MODIFIED))->setValue(start_date);

    // link the page to its type (very important for permissions)
    {
        // TODO We probably should test whether that content-types exists
        //      because if not it's certainly completely invalid (i.e. the
        //      programmer mistyped the type [again].)
        //      However, we have to be very careful as the initialization
        //      process may not be going in the right order and thus not
        //      have created the type yet when this starts to happen.
        QString const destination_key(site_key + "types/taxonomy/system/content-types/" + (type.isEmpty() ? "page" : type));
        path_info_t destination_ipath;
        destination_ipath.set_path(destination_key);
        QString const link_name(get_name(SNAP_NAME_CONTENT_PAGE_TYPE));
        QString const link_to(get_name(SNAP_NAME_CONTENT_PAGE));
        bool const source_unique(true);
        bool const destination_unique(false);
        links::link_info source(link_name, source_unique, key, branch_number);
        links::link_info destination(link_to, destination_unique, destination_key, destination_ipath.get_branch());
        links::links::instance()->create_link(source, destination);
    }

    // link this entry to its parent automatically
    // first we need to remove the site key from the path
    snap_version::version_number_t child_branch(branch_number);
    snap_version::version_number_t parent_branch;
    QStringList parts(ipath.get_cpath().split('/', QString::SkipEmptyParts));
    while(parts.count() > 0)
    {
        QString src(parts.join("/"));
        src = site_key + src;
        parts.pop_back();
        QString dst(parts.join("/"));
        dst = site_key + dst;

        // TBD: 3rd parameter should be true or false?
        parent_branch = get_current_branch(dst, true);

        // TBD: is the use of the system branch always correct here?
        links::link_info source(get_name(SNAP_NAME_CONTENT_PARENT), true, src, child_branch);
        links::link_info destination(get_name(SNAP_NAME_CONTENT_CHILDREN), false, dst, parent_branch);
// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//printf("parent/children [%s]/[%s]\n", src.toUtf8().data(), dst.toUtf8().data());
        links::links::instance()->create_link(source, destination);

        child_branch = parent_branch;
    }

    return true;
}


/** \brief Function called after all the other plugins signal were called.
 *
 * This function gives a chance to the content plugin to fix the status
 * to NORMAL since on creation it is set to CREATING instead.
 *
 * \param[in] path  The path of the new page.
 * \param[in] owner  The name of the plugin that is to own this page.
 * \param[in] type  The type of page.
 */
void content::create_content_done(path_info_t& ipath, QString const& owner, QString const& type)
{
    static_cast<void>(owner);
    static_cast<void>(type);

    // now the page was created and is ready to be used
    // (although the revision data is not yet available...
    // but at this point we do not have a good way to handle
    // that part yet.)
    path_info_t::status_t status(ipath.get_status());
    if(status.get_state() == path_info_t::status_t::state_t::CREATE)
    {
        status.set_state(path_info_t::status_t::state_t::NORMAL);
    }
    status.set_working(path_info_t::status_t::working_t::NOT_WORKING);
    ipath.set_status(status);
}


/** \brief Create a page which represents an attachment (A file).
 *
 * This function creates a page that represents an attachment with the
 * specified file, owner, and type. The new file path is saved in the
 * attachment_file object. On a successfully return (when the function
 * returns true) you can retrieve the attachment path with the
 * get_attachment_cpath() function.
 *
 * This function prepares the file and sends a create_content() event
 * to create the actual content entry if it did not yet exist.
 *
 * Note that the MIME type of the file is generated using the magic
 * database. The \p attachment_type parameter is the one saved in the
 * page referencing that file. However, only the one generated by magic
 * is official.
 *
 * \note
 * It is important to understand that we only save each file only ONCE
 * in the database. This is accomplished by create_attachment() by computing
 * the MD5 sum of the file and then checking whether the file was
 * previously loaded. If so, then the existing copy is used
 * (even if it was uploaded by someone else on another website!)
 *
 * Possible cases when creating an attachment:
 *
 * \li The file does not yet exist in the files table; in that case we
 *     simply create it
 *
 * \li If the file already existed, we do not add it again (obviously)
 *     and we can check whether it was already attached to that very
 *     same page; if so then we have nothing else to do (files have
 *     links of all the pages were they are attachments)
 *
 * \li When adding a JavaScript or CSS file, the version and browser
 *     information also gets checked; it is extracted from the file itself
 *     and used to version the file in the database (in the content row);
 *     note that each version of a JavaScript or CSS file ends up in
 *     the database (just like with a tool such as SVN or git).
 *
 * \warning
 * Since most files are versions (branch/revision numbers, etc.) you have
 * to realize that the function manages multiple filenames. There is one
 * filename which is \em bare and one filename which is versioned. The
 * bare filename is used as the attachment name. The versioned filename
 * is used as the attachment filename (in the files table.)
 *
 * \code
 *  // access the file as "editor.js" on the website
 *  http://snapwebsites.org/js/editor/editor.js
 *
 *  // saved the file as editor_1.2.3.js in files
 *  files["editor_1.2.3.js"]
 * \endcode
 *
 * This is particularly confusing because the server is capable of
 * recognizing a plethora of filenames that all resolve to the same
 * file in the files table only "tweaked" as required internally.
 * Tweaked here means reformatted as requested.
 *
 * \code
 *  // minimized version 1.2.3, current User Agent
 *  http://snapwebsites.org/js/editor/editor_1.2.3.min.js
 *
 *  // original version, compressed, current User Agent
 *  http://snapwebsites.org/js/editor/editor_1.2.3.org.js.gz
 *
 *  // specifically the version for Internet Explorer
 *  http://snapwebsites.org/js/editor/editor_1.2.3_ie.min.js
 *
 *  // the same with query strings
 *  http://snapwebsites.org/js/editor/editor.js?v=1.2.3&b=ie&e=min
 *
 *  // for images, you upload a JPEG and you can access it as a PNG...
 *  http://snapwebsites.org/some/page/image.png
 *
 *  // for images, you upload a 300x900 page, and access it as a 100x300 image
 *  http://snapwebsites.org/some/page/image.png?d=100x300
 * \endcode
 *
 * The supported fields are:
 *
 * \li \<name> -- the name of the file
 * \li [v=] \<version> -- a specific version of the file (if not specified, get
 *                   latest)
 * \li [b=] \<browser> -- a specific version for that browser
 * \li [e=] \<encoding> -- a specific encoding, in most cases a compression,
 *                         for a JavaScript/CSS file "minimize" is also
 *                         understood (i.e. min,gz or org,bz2); this can be
 *                         used to convert an image to another format
 * \li [d=] \<width>x<height> -- dimensions for an image
 *
 * \param[in,out] file  The file to save in the Cassandra database. It is an
 *                      in,out parameter because the exact filename used to
 *                      save the file in the content table is saved in this
 *                      object.
 * \param[in] branch_number  The branch used to save the attachment.
 * \param[in] locale  The language & country to use for this file.
 *
 * \return true if other plugins are to receive the signal too, the function
 *         generally returns false if the attachment cannot be created or
 *         already exists
 */
bool content::create_attachment_impl(attachment_file& file, snap_version::version_number_t branch_number, QString const& locale)
{
    // quick check for security reasons so we can avoid unwanted uploads
    // (note that we already had the check for size and similar "problems")
    permission_flag secure;
    check_attachment_security(file, secure, true);
    if(!secure.allowed())
    {
        return false;
    }

    // TODO: uploading compressed files is a problem if we are to match the
    //       proper MD5 of the file; we will want to check and decompress
    //       files so we only save the decompressed version MD5 and not the
    //       compressed MD5 (otherwise we end up with TWO files.)

    // verify that the row specified by file::get_cpath() exists
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QString const site_key(f_snap->get_site_key_with_slash());
    QString const parent_key(site_key + file.get_parent_cpath());
    if(!content_table->exists(parent_key))
    {
        // the parent row does not even exist yet...
        SNAP_LOG_ERROR("user attempted to create an attachment in page \"")(parent_key)("\" that doesn't exist.");
        return false;
    }

    // create the path to the new attachment itself
    // first get the basename
    snap_child::post_file_t const& post_file(file.get_file());
    QString attachment_filename(post_file.get_basename());

    // make sure that the parent of the attachment isn't final
    if(is_final(parent_key))
    {
        // the user was trying to add content under a final leaf
        f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Final Parent",
                QString("The attachment \"%1\" cannot be added under \"%2\" as this page is marked as final.")
                            .arg(attachment_filename).arg(parent_key),
                "The parent row does not allow for further children.");
        NOTREACHED();
    }

    snap_version::quick_find_version_in_source fv;
    QString revision; // there is no default
    QString extension;

    // if JavaScript or CSS, add the version to the filename before
    // going forward (unless the version is already there, of course)
    bool const is_js(file.get_parent_cpath().startsWith("js/"));
    bool const is_css(file.get_parent_cpath().startsWith("css/"));
    if(is_js)
    {
        extension = snap_version::find_extension(attachment_filename, js_extensions);
        if(extension.isEmpty())
        {
            f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Invalid Extension",
                    QString("The attachment \"%1\" cannot be added under \"%2\" as it does not represent JavaScript code.")
                                .arg(attachment_filename).arg(parent_key),
                    "The filename does not have a .js extension.");
            NOTREACHED();
        }
    }
    else if(is_css)
    {
        extension = snap_version::find_extension(attachment_filename, css_extensions);
        if(extension.isEmpty())
        {
            f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Invalid Extension",
                    QString("The attachment \"%1\" cannot be added under \"%2\" as it does not represent CSS data.")
                                .arg(attachment_filename).arg(parent_key),
                    "The filename does not have a .css extension.");
            NOTREACHED();
        }
    }
    if(is_js || is_css)
    {
        // TODO: In this case, really, we probably should only accept
        //       filenames without anything specified although the version
        //       is fine if it matches what is defined in the file...
        //       However, if the name includes .min. (minimized) then we've
        //       got a problem because the non-minimized version would not
        //       match properly. This being said, a version that is
        //       pre-minimized can be uploaded as long as the .org. is not
        //       used to see a non-minimized version.

        if(!fv.find_version(post_file.get_data().data(), post_file.get_size()))
        {
            f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Invalid File",
                    "The attachment \"" + attachment_filename + "\" does not include a valid C-like comment at the start. The comment must at least include a <a href=\"See http://snapwebsites.org/implementation/feature-requirements/attachments-core\">Version field</a>.",
                    "The content of this file is not valid for a JavaScript or CSS file (version required).");
            NOTREACHED();
        }

        if(attachment_filename.contains("_"))
        {
            // if there is a "_" then we have a file such as
            //
            //   <name>_<version>.js
            // or
            //   <name>_<version>_<browser>.js
            //
            snap_version::versioned_filename js_filename(extension);
            if(!js_filename.set_filename(attachment_filename))
            {
                f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Invalid Filename",
                        "The attachment \"" + attachment_filename + "\" has an invalid name and must be rejected. " + js_filename.get_error(),
                        "The name is not considered valid for a versioned file.");
                NOTREACHED();
            }
            if(fv.get_version_string() != js_filename.get_version_string())
            {
                f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Versions Mismatch",
                        QString("The attachment \"%1\" filename version (%2) is not the same as the version inside the file (%3).")
                            .arg(attachment_filename)
                            .arg(js_filename.get_version_string())
                            .arg(fv.get_version_string()),
                        "The version in the filename is not equal to the one defined in the file.");
                NOTREACHED();
            }
            // TBD can we verify the browser defined in the filename
            //     against Browsers field found in the file?

            // remove the version and browser information from the filename
            attachment_filename = js_filename.get_name() + extension;

            if(fv.get_name().isEmpty())
            {
                // no name field, use the filename
                fv.set_name(js_filename.get_name());
            }
        }
        else
        {
            // in this case the name is just <name> and must be
            //
            //    [a-z][-a-z0-9]*[a-z0-9]
            //
            // get the filename without the extension
            QString const fn(attachment_filename.left(attachment_filename.length() - extension.length()));
            QString name_string(fn);
            QString namespace_string;
            QString errmsg;
            if(!snap_version::validate_name(name_string, errmsg, namespace_string))
            {
                // unacceptable filename
                f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Invalid Filename",
                        QString("The attachment \"%1\" has an invalid name and must be rejected. %2").arg(attachment_filename).arg(errmsg),
                        "The name is not considered valid for a versioned file.");
                NOTREACHED();
            }

            if(fv.get_name().isEmpty())
            {
                // no name field, use the filename
                fv.set_name(fn);
            }
        }

        // the filename is now just <name> (in case it had a version and/or
        // browser indication on entry.)

        // ignore the input branch number, instead retrieve first version
        // number of the file as the branch number...
        branch_number = fv.get_branch();
        revision = fv.get_version_string();
#ifdef DEBUG
        if(revision.isEmpty() || snap_version::SPECIAL_VERSION_UNDEFINED == branch_number)
        {
            // we already checked for errors while parsing the file so we
            // should never reach here if the version is empty in the file
            throw snap_logic_exception("the version of a JavaScript or CSS file just cannot be empty here");
        }
#endif

        // in the attachment, save the filename with the version so that way
        // it is easier to see which is which there
    }
    else
    {
        // for other attachments, there could be a language specified as
        // in .en.jpg. In that case we want to get the filename without
        // the language and mark that file as "en"

        // TODO: actually implement the language extraction capability
    }

    // path in the content table, the attachment_filename is the simple
    // name without version, language, or encoding
    path_info_t attachment_ipath;
    //attachment_ipath.set_owner(...); -- this is not additional so keep the default (content)
    attachment_ipath.set_path(QString("%1/%2").arg(file.get_parent_cpath()).arg(attachment_filename));
    if(!revision.isEmpty())
    {
        // in this case the revision becomes a string with more than one
        // number and the branch is the first number (this is for js/css
        // files only at this point.)
        attachment_ipath.force_extended_revision(revision, attachment_filename);
    }

    // save the path to the attachment so the caller knows exactly where it
    // is (if required by that code.)
    file.set_attachment_cpath(attachment_ipath.get_cpath());

#ifdef DEBUG
//SNAP_LOG_DEBUG("attaching ")(file.get_file().get_filename())(", attachment_key = ")(attachment_ipath.get_key());
#endif

    // compute the MD5 sum of the file
    // TBD should we forbid the saving of empty files?
    unsigned char md[MD5_DIGEST_LENGTH];
    MD5(reinterpret_cast<unsigned char const *>(post_file.get_data().data()), post_file.get_size(), md);
    QByteArray const md5(reinterpret_cast<char const *>(md), sizeof(md));

    // check whether the file already exists in the database
    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    bool file_exists(files_table->exists(md5));
    if(!file_exists)
    {
        // the file does not exist yet, add it
        //
        // 1. create the row with the file data, the compression used,
        //    and size; also add it to the list of new cells
        files_table->row(md5)->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA))->setValue(post_file.get_data());
        files_table->row(get_name(SNAP_NAME_CONTENT_FILES_NEW))->cell(md5)->setValue(true);

        QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(md5));

        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_COMPRESSOR))->setValue(get_name(SNAP_NAME_CONTENT_COMPRESSOR_UNCOMPRESSED));
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE))->setValue(static_cast<int32_t>(post_file.get_size()));

        // Note we save the following mainly for completness because it is
        // not really usable (i.e. two people who are to upload the same file
        // with the same filename, the same original MIME type, the same
        // creation/modification dates... close to impossible!)
        //
        // 2. link back to the row where the file is saved in the content table
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_FILENAME))->setValue(attachment_filename);

        // 3. save the computed MIME type
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE))->setValue(post_file.get_mime_type());

        // 4. save the original MIME type
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->setValue(post_file.get_original_mime_type());

        // 5. save the creation date if available (i.e. if not zero)
        if(post_file.get_creation_time() != 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME))->setValue(static_cast<int64_t>(post_file.get_creation_time()));
        }

        // 6. save the modification date if available (i.e. if not zero)
        if(post_file.get_modification_time() != 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->setValue(static_cast<int64_t>(post_file.get_modification_time()));
        }

        // 7. save the date when the file was uploaded
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATED))->setValue(f_snap->get_start_date());

        // 8. save the date when the file was last updated
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());

        // 9. if the file is an image save the width & height
        int32_t width(post_file.get_image_width());
        int32_t height(post_file.get_image_height());
        if(width > 0 && height > 0)
        {
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->setValue(width);
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->setValue(height);
        }

        // 10. save the description
        // At this point we do not have that available, we could use the
        // comment/description from the file if there is such, but those
        // are often "broken" (i.e. version of the camera used...)

        // TODO should we also save a SHA1 of the files so people downloading
        //      can be given the SHA1 even if the file is saved compressed?

        // 11. Some additional fields
        signed char sflag(CONTENT_SECURE_UNDEFINED);
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(static_cast<int64_t>(0));
        file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString());

        // 12. save dependencies
        {
            // dependencies will always be the same for all websites so we
            // save them here too
            dependency_list_t const& deps(file.get_dependencies());
            QMap<QString, bool> found;
            int const max_size(deps.size());
            for(int i(0); i < max_size; ++i)
            {
                snap_version::dependency d;
                if(!d.set_dependency(deps[i]))
                {
                    // simply invalid...
                    SNAP_LOG_ERROR("Dependency \"")(deps[i])("\" is not valid (")(d.get_error())("). We cannot add it to the database. Note: the content plugin does not support <dependency> tags with comma separated dependencies. Instead create multiple tags.");
                }
                else
                {
                    QString const dependency_name(d.get_name());
                    QString full_name;
                    if(d.get_namespace().isEmpty())
                    {
                        full_name = dependency_name;
                    }
                    else
                    {
                        full_name = QString("%1::%2").arg(d.get_namespace()).arg(dependency_name);
                    }
                    if(found.contains(full_name))
                    {
                        // not unique
                        SNAP_LOG_ERROR("Dependency \"")(deps[i])("\" was specified more than once. We cannot safely add the same dependency (same name) more than once. Please merge both definitions or delete one of them.");
                    }
                    else
                    {
                        // save the canonicalized version of the dependency in the database
                        found[full_name] = true;
                        file_row->cell(QString("%1::%2").arg(get_name(SNAP_NAME_CONTENT_FILES_DEPENDENCY)).arg(full_name))->setValue(d.get_dependency_string());
                    }
                }
            }
        }
    }
// for test purposes to check a file over and over again
//files_table->row(get_name(SNAP_NAME_CONTENT_FILES_NEW))->cell(md5)->setValue(true);

    // make a full reference back to the attachment (which may not yet
    // exist at this point, we do that next)
    signed char const ref(1);
    files_table->row(md5)->cell(QString("%1::%2").arg(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(attachment_ipath.get_key()))->setValue(ref);

    QByteArray attachment_ref;
    attachment_ref.append(get_name(SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE));
    attachment_ref.append("::");
    attachment_ref.append(md5); // binary md5

    // check whether the row exists before we create it
    bool const content_row_exists(content_table->exists(attachment_ipath.get_key()));

    // this may be a new content row, that is, it may still be empty so we
    // have to test several things before we can call create_content()...

    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());

    // if the revision is still empty then we're dealing with a file
    // which is neither a JavaScript nor a CSS file
    if(revision.isEmpty())
    {
        // TODO: allow editing of any branch, not just the working
        //       branch... (when using "?branch=123"...)

        snap_version::version_number_t revision_number(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_UNDEFINED));

        if(file_exists
        && snap_version::SPECIAL_VERSION_UNDEFINED != branch_number
        && snap_version::SPECIAL_VERSION_INVALID != branch_number)
        {
            attachment_ipath.force_branch(branch_number);

            // the file already exists, it could very well be that the
            // file had an existing revision in this attachment row so
            // search for all existing revisions (need a better way to
            // instantly find those!)
            //QString const attachment_ref(QString("%1::%2").arg(get_name(SNAP_NAME_CONTENT_ATTACHMENT_REFERENCE)).arg(md5));
            file_exists = branch_table->exists(attachment_ipath.get_branch_key())
                       && branch_table->row(attachment_ipath.get_branch_key())->exists(attachment_ref);
            if(file_exists)
            {
                // the reference row exists!
                file_exists = true; // avoid generation of a new revision!
                revision_number = branch_table->row(attachment_ipath.get_branch_key())->cell(attachment_ref)->value().int64Value();
                attachment_ipath.force_revision(revision_number);
                //content_table->row(attachment_ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT_REVISION_CONTROL_CURRENT_WORKING_VERSION))->setValue(revision);
            }
        }

        if(!file_exists)
        {
            if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number
            || snap_version::SPECIAL_VERSION_INVALID == branch_number)
            {
                branch_number = get_current_branch(attachment_ipath.get_key(), true);
            }
            attachment_ipath.force_branch(branch_number);
            if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number)
            {
                // this should nearly never (if ever) happen
                branch_number = get_new_branch(attachment_ipath.get_key(), locale);
                set_branch_key(attachment_ipath.get_key(), branch_number, true);
                // new branches automatically get a revision of zero (0)
                revision_number = static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION);
            }
            else
            {
                revision_number = get_new_revision(attachment_ipath.get_key(), branch_number, locale, true);
            }
            attachment_ipath.force_revision(revision_number);
        }

        if(snap_version::SPECIAL_VERSION_UNDEFINED == branch_number
        || snap_version::SPECIAL_VERSION_UNDEFINED == revision_number)
        {
            throw snap_logic_exception(QString("the branch (%1) and/or revision (%2) numbers are still undefined").arg(branch_number).arg(revision_number));
        }

        set_branch(attachment_ipath.get_key(), branch_number, true);
        set_branch(attachment_ipath.get_key(), branch_number, false);
        set_branch_key(attachment_ipath.get_key(), branch_number, true);
        set_branch_key(attachment_ipath.get_key(), branch_number, false);

        // TODO: this call is probably wrong, that is, it works and shows the
        //       last working version but the user may want to keep a previous
        //       revision visible at this point...
        set_current_revision(attachment_ipath.get_key(), branch_number, revision_number, locale, false);
        set_current_revision(attachment_ipath.get_key(), branch_number, revision_number, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision_number, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision_number, locale, false);

        // back reference for quick search
        branch_table->row(attachment_ipath.get_branch_key())->cell(attachment_ref)->setValue(static_cast<int64_t>(revision_number));

        revision = QString("%1.%2").arg(branch_number).arg(revision_number);
    }
    else
    {
        // for JavaScript and CSS files we have it simple for now but
        // this is probably somewhat wrong... (remember that for JS/CSS files
        // we do not generate a revision number, we use the file version
        // instead.)
        set_branch(attachment_ipath.get_key(), branch_number, true);
        set_branch(attachment_ipath.get_key(), branch_number, false);
        set_branch_key(attachment_ipath.get_key(), branch_number, true);
        set_branch_key(attachment_ipath.get_key(), branch_number, false);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision, locale, true);
        set_revision_key(attachment_ipath.get_key(), branch_number, revision, locale, false);

        // TODO: add set_current_revision()/set_revision_key()/... to save
        //       that info (only the revision here may be multiple numbers)
    }

    // this name is "content::attachment::<plugin owner>::<field name>::path" (unique)
    //           or "content::attachment::<plugin owner>::<field name>::path::<server name>_<unique number>" (multiple)
    QString const name(file.get_name());
    QtCassandra::QCassandraRow::pointer_t parent_row(content_table->row(parent_key));

    QtCassandra::QCassandraRow::pointer_t content_attachment_row(content_table->row(attachment_ipath.get_key()));
    //QtCassandra::QCassandraRow::pointer_t branch_attachment_row(branch_table->row(attachment_ipath.get_branch_key()));
    QtCassandra::QCassandraRow::pointer_t revision_attachment_row(revision_table->row(attachment_ipath.get_revision_key()));

    // if the field exists and that attachment is unique (i.e. supports only
    // one single file), then we want to delete the existing page unless
    // the user uploaded a file with the exact same filename
    if(content_row_exists)
    {
        // if multiple it can already exist, we just created a new unique number
        if(!file.get_multiple())
        {
            // it exists, check the filename first
            if(parent_row->exists(name))
            {
                // check the filename
                QString old_attachment_key(parent_row->cell(name)->value().stringValue());
                if(!old_attachment_key.isEmpty() && old_attachment_key != attachment_ipath.get_key())
                {
                    // that's not the same filename, drop it
                    // WE CANNOT JUST DROP A ROW, it breaks all the links, etc.
                    // TODO: implement a delete_content() function which
                    //       does all the necessary work (and actually move
                    //       the content to the trashcan)
                    //content_table->dropRow(old_attachment_key, QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

                    // TODO: nothing should be deleted in our system, instead
                    //       it should be put in a form of trashcan; in this
                    //       case it could remain an attachment, only moved
                    //       to a special "old attachments" list

                    // TBD if I'm correct, the md5 reference was already dropped
                    //     in the next if() blocks...
                }
            }
        }

        if(revision_attachment_row->exists(get_name(SNAP_NAME_CONTENT_ATTACHMENT)))
        {
            // the MD5 is saved in there, get it and compare
            QtCassandra::QCassandraValue existing_ref(revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->value());
            if(!existing_ref.nullValue())
            {
                if(existing_ref.binaryValue() == md5)
                {
                    // this is the exact same file, do nearly nothing
                    // (i.e. the file may already exist but the path
                    //       may not be there anymore)
                    parent_row->cell(name)->setValue(attachment_ipath.get_key());

                    modified_content(attachment_ipath);

                    // TBD -- should it be true here to let the other plugins
                    //        do their own work?
                    return false;
                }

                // not the same file, we've got to remove the reference
                // from the existing file since it's going to be moved
                // to a new file (i.e. the current md5 points to a
                // different file)
                //
                // TODO: nothing should just be dropped in our system,
                //       instead it should be moved to some form of
                //       trashcan; in this case we'd use a new name
                //       for the reference although if the whole row
                //       is to be "dropped" (see below) then we should
                //       not even have to drop this cell at all because
                //       it will remain there, only under a different
                //       name...
                files_table->row(existing_ref.binaryValue())->dropCell(attachment_ipath.get_cpath());
            }
        }

        // it is not there yet, so go on...
        //
        // TODO: we want to check all the attachments and see if any
        //       one of them is the same file (i.e. user uploading the
        //       same file twice with two different file names...)

        files_table->row(md5)->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->setValue(f_snap->get_start_date());
    }

    // yes that path may already exists, no worries since the create_content()
    // function checks that and returns quickly if it does exist
    create_content(attachment_ipath, file.get_attachment_owner(), file.get_attachment_type());

    // if it is already filename it won't hurt too much to set it again
    parent_row->cell(name)->setValue(attachment_ipath.get_key());

    // mark all attachments as final (i.e. cannot create children below an attachment)
    signed char final(1);
    content_attachment_row->cell(get_name(SNAP_NAME_CONTENT_FINAL))->setValue(final);

    // in this case 'post' represents the filename as sent by the
    // user, the binary data is in the corresponding file
    revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT_FILENAME))->setValue(attachment_filename);

    // save the file reference
    revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->setValue(md5);

    // save the MIME type (this is the one returned by the magic library)
    revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT_MIME_TYPE))->setValue(post_file.get_mime_type());

    // the date when it was created
    int64_t const start_date(f_snap->get_start_date());
    revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(start_date);

    // XXX we could also save the modification and creation dates, but the
    //     likelihood that these exist is so small that I'll skip at this
    //     time; we do save them in the files table

    // TODO: create an even for this last part because it requires JavaScript
    //       or CSS support which is not part of the base content plugin.
    // We depend on the JavaScript plugin so we have to do some of its
    // work here...
    if(is_js || is_css)
    {
        // JavaScripts get added to a list so their dependencies
        // can be found "instantaneously".
        //snap_version::versioned_filename js_filename(".js");
        //js_filename.set_filename(attachment_filename);
        // the name is formatted to allow us to quickly find the files
        // we're interested; in that we put the name first, then the
        // browser, and finally the version which is saved as integers
        snap_version::name_vector_t browsers(fv.get_browsers());
        int const bmax(browsers.size());
        bool all(bmax == 1 && browsers[0].get_name() == "all");
        for(int i(0); i < bmax; ++i)
        {
            QByteArray jskey;
            jskey.append(fv.get_name());
            jskey.append('_');
            jskey.append(browsers[i].get_name());
            jskey.append('_');
            snap_version::version_numbers_vector_t const& version(fv.get_version());
            int const vmax(version.size());
            for(int v(0); v < vmax; ++v)
            {
                QtCassandra::appendUInt32Value(jskey, version[v]);
            }
            // TODO: find a proper way to access the JS plugin... unless
            //       we consider the JAVASCRIPT row name as a content plugin
            //       definition?
            files_table->row(is_css ? "css" : "javascripts"/*javascript::get_name(javascript::SNAP_NAME_JAVASCRIPT_ROW)*/)->cell(jskey)->setValue(md5);
            if(!all)
            {
                // TODO: need to parse the script for this specific browser
            }
        }
    }

    return true;
}


/** \brief Check whether a page is marked as final.
 *
 * A page is marked final with the field named "content::final" set to 1.
 * Attachments are always marked final because you cannot create a sub-page
 * under an attachment.
 *
 * \param[in] key  The full key to the page in the content table.
 *
 * \return true if the page at 'key' is marked as being final.
 */
bool content::is_final(QString const& key)
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(content_table->exists(key))
    {
        QtCassandra::QCassandraRow::pointer_t parent_row(content_table->row(key));
        if(parent_row->exists(get_name(SNAP_NAME_CONTENT_FINAL)))
        {
            QtCassandra::QCassandraValue final_value(parent_row->cell(get_name(SNAP_NAME_CONTENT_FINAL))->value());
            if(!final_value.nullValue())
            {
                if(final_value.signedCharValue())
                {
                    // it is final...
                    return true;
                }
            }
        }
    }

    return false;
}


/** \brief Load an attachment previously saved with create_attachment().
 *
 * The function checks that the attachment exists and is in good condition
 * and if so, loads it in the specified file parameter.
 *
 * \param[in] key  The key to the attachment to load.
 * \param[in] file  The file will be loaded in this structure.
 * \param[in] load_data  Whether the data should be loaded (true by default.)
 *
 * \return true if the attachment was successfully loaded.
 */
bool content::load_attachment(QString const& key, attachment_file& file, bool load_data)
{
    path_info_t ipath;
    ipath.set_path(key);

    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    if(!content_table->exists(ipath.get_key()))
    {
        // the row does not even exist yet...
        return false;
    }

    // TODO: select the WORKING_VERSION if the user is logged in and can
    //       edit this attachment
    QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());
    QtCassandra::QCassandraRow::pointer_t revision_attachment_row(revision_table->row(ipath.get_revision_key()));
    QtCassandra::QCassandraValue md5_value(revision_attachment_row->cell(get_name(SNAP_NAME_CONTENT_ATTACHMENT))->value());

    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    if(!files_table->exists(md5_value.binaryValue()))
    {
        // file not available?!
        return false;
    }
    QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(md5_value.binaryValue()));

    if(!file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_DATA)))
    {
        // no data available
        return false;
    }

    // TODO handle the compression of the file...
    //file.set_file_compressor(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_COMPRESSOR))->value()->stringValue());

    if(load_data)
    {
        file.set_file_data(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA))->value().binaryValue());

        // TODO if compressed, we may have (want) to decompress here?
    }
    else
    {
        // since we're not loading the data, we want to get some additional
        // information on the side: the verified MIME type and the file size
        if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE)))
        {
            // This one gets set automatically when we set the data so we only
            // load it if the data is not getting loaded
            file.set_file_mime_type(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MIME_TYPE))->value().stringValue());
        }
        if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_SIZE)))
        {
            // since we're not loading the data, we get the size parameter
            // like this (later we may want to always do that once we save
            // files compressed in the database!)
            file.set_file_size(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE))->value().int32Value());
        }
    }

    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_FILENAME)))
    {
        file.set_file_filename(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_FILENAME))->value().stringValue());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE)))
    {
        file.set_file_original_mime_type(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_ORIGINAL_MIME_TYPE))->value().stringValue());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME)))
    {
        file.set_file_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATION_TIME))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME)))
    {
        file.set_file_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_MODIFICATION_TIME))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_CREATED)))
    {
        file.set_creation_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_CREATED))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_UPDATED)))
    {
        file.set_update_time(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_UPDATED))->value().int64Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH)))
    {
        file.set_file_image_width(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_WIDTH))->value().int32Value());
    }
    if(file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT)))
    {
        file.set_file_image_height(file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_IMAGE_HEIGHT))->value().int32Value());
    }

    return true;
}


/** \brief Tell the system that data was updated.
 *
 * This signal should be called any time you modify something in a page.
 *
 * This very function takes care of updating the content::modified and
 * content:updated as required:
 *
 * \li content::modified -- if anything changes in a page, this date
 *                          is changed; in other words, any time this
 *                          function is called, this date is set to
 *                          the current date
 *
 * \li content::updated -- if the content gets updated then this date
 *                         is expected to change; "content" here means
 *                         the title, body, or "any" important content
 *                         that is shown to the user (i.e. a small
 *                         change in a field that is not displayed or
 *                         is not directly considered content as part of
 *                         the main body of the page should not change
 *                         this date)
 *
 * This signal also gives other modules a chance to update their own
 * data (i.e. the sitemap.xml needs to update this page information.)
 *
 * Since the other plugins may make use of your plugin changes, you have
 * to call this signal last.
 *
 * \note
 * The function returns false and generates a warning (in your log) in the
 * event the process cannot find the specified path.
 *
 * \param[in,out] ipath  The path to the page being udpated.
 *
 * \return true if the event should be propagated.
 */
bool content::modified_content_impl(path_info_t& ipath)
{
    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());
    QString const branch_key(ipath.get_branch_key());
    if(!branch_table->exists(branch_key))
    {
        // the row doesn't exist?!
        SNAP_LOG_WARNING("Page \"")(branch_key)("\" does not exist. We cannot do anything about it being modified.");;
        return false;
    }
    QtCassandra::QCassandraRow::pointer_t row(branch_table->row(branch_key));

    int64_t const start_date(f_snap->get_start_date());
    row->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

    return true;
}


/** \brief Retreive a content page parameter.
 *
 * This function reads a column from the content of the page using the
 * content key as defined by the canonicalization process. The function
 * cannot be called before the content::on_path_execute() function is
 * called and the key properly initialized.
 *
 * The table is opened once and remains opened so calling this function
 * many times is not a problem. Also the libQtCassandra library caches
 * all the data. Reading the same field multiple times is not a concern
 * at all.
 *
 * If the value is undefined, the result is a null value.
 *
 * \note
 * The path should be canonicalized before the call although we call
 * the remove_slashes() function on it cleanup starting and ending
 * slashes (because the URI object returns paths such as "/login" and
 * the get_content_parameter() requires just "login" to work right.)
 *
 * \param[in,out] ipath  The canonicalized path being managed.
 * \param[in] param_name  The name of the parameter to retrieve.
 * \param[in] revision  Which row we are to access for the required parameter.
 *
 * \return The content of the row as a Cassandra value.
 */
QtCassandra::QCassandraValue content::get_content_parameter(path_info_t& ipath, QString const& param_name, param_revision_t revision)
{
    switch(revision)
    {
    case PARAM_REVISION_GLOBAL:
        {
            QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

            if(!content_table->exists(ipath.get_key())
            || !content_table->row(ipath.get_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                QtCassandra::QCassandraValue value;
                return value;
            }

            return content_table->row(ipath.get_key())->cell(param_name)->value();
        }

    case PARAM_REVISION_BRANCH:
        {
            QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());

            if(!branch_table->exists(ipath.get_branch_key())
            || !branch_table->row(ipath.get_branch_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                QtCassandra::QCassandraValue value;
                return value;
            }

            return branch_table->row(ipath.get_branch_key())->cell(param_name)->value();
        }

    case PARAM_REVISION_REVISION:
        {
            QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());

            if(!revision_table->exists(ipath.get_revision_key())
            || !revision_table->row(ipath.get_revision_key())->exists(param_name))
            {
                // an empty value is considered to be a null value
                QtCassandra::QCassandraValue value;
                return value;
            }

            return revision_table->row(ipath.get_revision_key())->cell(param_name)->value();
        }

    default:
        throw snap_logic_exception("invalid PARAM_REVISION_... parameter to get_content_parameter().");

    }
    NOTREACHED();
}

/** \brief Prepare a set of content to add to the database.
 *
 * In most cases, plugins call this function in one of their do_update()
 * functions to add their content.xml file to the database.
 *
 * This function expects a plugin name as input to add the
 * corresponding content.xml file of that plugin. The data is search in
 * the resources (it is expected to be added there by the plugin).
 * The resource path is built as follow:
 *
 * \code
 * ":/plugins/" + plugin_name + "/content.xml"
 * \endcode
 *
 * The content is not immediately added to the database because
 * of dependency issues. At the time all the content is added
 * using this function, the order in which it is added is not
 * generally proper (i.e. the taxonomy "/types" may be
 * added after the content "/types/taxonomy/system/content-types"
 * which would then fail.)
 *
 * The content plugin saves this data when it receives the
 * save_content signal.
 *
 * To dynamically add content (opposed to adding information
 * from an XML file) you want to call the add_param() and
 * add_link() functions as required.
 *
 * \param[in] plugin_name  The name of the plugin loading this data.
 *
 * \sa on_save_content()
 * \sa add_param()
 * \sa add_link()
 */
void content::add_xml(const QString& plugin_name)
{
    if(!plugins::verify_plugin_name(plugin_name))
    {
        // invalid plugin name
        throw content_exception_invalid_content_xml("add_xml() called with an invalid plugin name: \"" + plugin_name + "\"");
    }
    QString const filename(":/plugins/" + plugin_name + "/content.xml");
    QFile xml_content(filename);
    if(!xml_content.open(QFile::ReadOnly))
    {
        // file not found
        throw content_exception_invalid_content_xml("add_xml() cannot open file: \"" + filename + "\"");
    }
    QDomDocument dom;
    if(!dom.setContent(&xml_content, false))
    {
        // invalid XML
        throw content_exception_invalid_content_xml("add_xml() cannot read the XML of content file: \"" + filename + "\"");
    }
    add_xml_document(dom, plugin_name);
}


/** \brief Add data to the database using a DOM.
 *
 * This function is called by the add_xml() function after a DOM was loaded.
 * It can be called by other functions which load content XML data from
 * a place other than the resources.
 *
 * \note
 * As an example, the layout plugin will call this function if it finds
 * a content.xml file in its list of files.
 *
 * \param[in] dom  The DOM to add to the content system.
 * \param[in] plugin_name  The name of the plugin loading this data.
 */
void content::add_xml_document(QDomDocument& dom, QString const& plugin_name)
{
    QDomNodeList content_nodes(dom.elementsByTagName(get_name(SNAP_NAME_CONTENT_TAG)));
    int const max_nodes(content_nodes.size());
    for(int i(0); i < max_nodes; ++i)
    {
        QDomNode content_node(content_nodes.at(i));
        if(!content_node.isElement())
        {
            // we're only interested in elements
            continue;
        }
        QDomElement content_element(content_node.toElement());
        if(content_element.isNull())
        {
            // somehow this is not an element
            continue;
        }

        QString owner(content_element.attribute("owner"));
        if(owner.isEmpty())
        {
            owner = plugin_name;
        }

        QString path(content_element.attribute("path"));
        if(path.isEmpty())
        {
            throw content_exception_invalid_content_xml("all <content> tags supplied to add_xml() must include a valid \"path\" attribute");
        }
        f_snap->canonicalize_path(path);
        QString const key(f_snap->get_site_key_with_slash() + path);

        // create a new entry for the database
        add_content(key, owner);

        QDomNodeList children(content_element.childNodes());
        bool found_content_type(false);
        bool found_prevent_delete(false);
        int const cmax(children.size());
        for(int c(0); c < cmax; ++c)
        {
            // grab <param> and <link> tags
            QDomNode child(children.at(c));
            if(!child.isElement())
            {
                // we're only interested by elements
                continue;
            }
            QDomElement element(child.toElement());
            if(element.isNull())
            {
                // somehow this is not really an element
                continue;
            }

            // <param name=... overwrite=... force-namespace=...> data </param>
            QString const tag_name(element.tagName());
            if(tag_name == "param")
            {
                QString const param_name(element.attribute("name"));
                if(param_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <param> tags supplied to add_xml() must include a valid \"name\" attribute");
                }

                // 1) prepare the buffer
                // the parameter value can include HTML (should be in a [CDATA[...]] in that case)
                QString buffer;
                QTextStream data(&buffer);
                // we have to save all the element children because
                // saving the element itself would save the <param ...> tag
                // also if the whole is a <![CDATA[...]]> entry, remove it
                // (but keep sub-<![CDATA[...]]> if any.)
                QDomNodeList values(element.childNodes());
                int lmax(values.size());
                if(lmax == 1)
                {
                    QDomNode n(values.at(0));
                    if(n.isCDATASection())
                    {
                        QDomCDATASection raw_data(n.toCDATASection());
                        data << raw_data.data();
                    }
                    else
                    {
                        // not a CDATA section, save as is
                        n.save(data, 0);
                    }
                }
                else
                {
                    // save all the children
                    for(int l(0); l < lmax; ++l)
                    {
                        values.at(l).save(data, 0);
                    }
                }

                // 2) prepare the name
                QString fullname;
                // It seems to me that if the developer included any namespace
                // then it was meant to be defined that way
                //if(param_name.left(plugin_name.length() + 2) == plugin_name + "::")
                if(param_name.contains("::")) // already includes a namespace
                {
                    // plugin namespace already defined
                    fullname = param_name;
                }
                else
                {
                    // plugin namespace not defined
                    if(element.attribute("force-namespace") == "no")
                    {
                        // but developer said no namespace needed (?!)
                        fullname = param_name;
                    }
                    else
                    {
                        // this is the default!
                        fullname = plugin_name + "::" + param_name;
                    }
                }

                if(fullname == get_name(SNAP_NAME_CONTENT_PREVENT_DELETE))
                {
                    found_prevent_delete = true;
                }

                param_revision_t revision_type(PARAM_REVISION_BRANCH);
                QString const revision_name(element.attribute("revision", "branch"));
                if(revision_name == "global")
                {
                    revision_type = PARAM_REVISION_GLOBAL;
                }
                else if(revision_name == "revision")
                {
                    revision_type = PARAM_REVISION_REVISION;
                }
                else if(revision_name != "branch")
                {
                    throw content_exception_invalid_content_xml(QString("<param> tag used an invalid \"revision\" attribute (%1); we expected \"global\", \"branch\", or \"revision\".").arg(revision_name));
                }

                QString locale(element.attribute("lang", "en"));
                QString country;
                f_snap->verify_locale(locale, country, true);
                if(!country.isEmpty())
                {
                    locale += '_';
                    locale += country;
                }

                // add the resulting parameter
                add_param(key, fullname, revision_type, locale, buffer);

                // check whether we allow overwrites
                if(element.attribute("overwrite") == "yes")
                {
                    set_param_overwrite(key, fullname, true);
                }

                // check whether a data type was defined
                QString type(element.attribute("type"));
                if(!type.isEmpty())
                {
                    param_type_t param_type;
                    if(type == "string")
                    {
                        param_type = PARAM_TYPE_STRING;
                    }
                    else if(type == "float")
                    {
                        param_type = PARAM_TYPE_FLOAT;
                    }
                    else if(type == "int8")
                    {
                        param_type = PARAM_TYPE_INT8;
                    }
                    else if(type == "int64")
                    {
                        param_type = PARAM_TYPE_INT64;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml(QString("unknown type in <param type=\"%1\"> tags").arg(type));
                    }
                    set_param_type(key, fullname, param_type);
                }
            }
            // <link name=... to=... [mode="1/*:1/*"]> destination path </link>
            else if(tag_name == "link")
            {
                QString link_name(element.attribute("name"));
                if(link_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                if(link_name == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"name\" attribute of a <link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_name.contains("::"))
                {
                    // force the owner in the link name
                    link_name = QString("%1::%2").arg(plugin_name).arg(link_name);
                }
                if(link_name == get_name(SNAP_NAME_CONTENT_PAGE_TYPE))
                {
                    found_content_type = true;
                }
                QString link_to(element.attribute("to"));
                if(link_to.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <link> tags supplied to add_xml() must include a valid \"to\" attribute");
                }
                if(link_to == plugin_name)
                {
                    throw content_exception_invalid_content_xml(QString("the \"to\" attribute of a <link> tag cannot be set to the plugin name (%1)").arg(plugin_name));
                }
                if(!link_to.contains("::"))
                {
                    // force the owner in the link name
                    link_to = QString("%1::%2").arg(plugin_name).arg(link_to);
                }
                bool source_unique(true);
                bool destination_unique(true);
                QString mode(element.attribute("mode"));
                if(!mode.isEmpty() && mode != "1:1")
                {
                    if(mode == "1:*")
                    {
                        destination_unique = false;
                    }
                    else if(mode == "*:1")
                    {
                        source_unique = false;
                    }
                    else if(mode == "*:*")
                    {
                        destination_unique = false;
                        source_unique = false;
                    }
                    else
                    {
                        throw content_exception_invalid_content_xml("<link> tags mode attribute must be one of \"1:1\", \"1:*\", \"*:1\", or \"*:*\"");
                    }
                }
                // the destination URL is defined in the <link> content
                QString destination_path(element.text());
                f_snap->canonicalize_path(destination_path);
                QString destination_key(f_snap->get_site_key_with_slash() + destination_path);
                links::link_info source(link_name, source_unique, key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                links::link_info destination(link_to, destination_unique, destination_key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
                add_link(key, source, destination);
            }
            // <attachment name=... type=... [owner=...]> resource path to file </link>
            else if(tag_name == "attachment")
            {
                content_attachment ca;

                // the owner is optional, it defaults to "content"
                // TODO: verify that "content" is correct, and that we should
                //       not instead use the plugin name (owner of this page)
                ca.f_owner = element.attribute("owner");
                if(ca.f_owner.isEmpty())
                {
                    // the output plugin is the default owner
                    ca.f_owner = get_name(SNAP_NAME_CONTENT_ATTACHMENT_PLUGIN);
                }
                ca.f_field_name = element.attribute("name");
                if(ca.f_field_name.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid \"name\" attribute");
                }
                ca.f_type = element.attribute("type");
                if(ca.f_type.isEmpty())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid \"type\" attribute");
                }

                // XXX Should we prevent filenames that do not represent
                //     a resource? If not a resource, changes that it is not
                //     accessible to the server are high unless the file was
                //     installed in a shared location (/usr/share/snapwebsites/...)
                QDomElement path_element(child.firstChildElement("path"));
                if(path_element.isNull())
                {
                    throw content_exception_invalid_content_xml("all <attachment> tags supplied to add_xml() must include a valid <paht> child tag");
                }
                ca.f_filename = path_element.text();

                QDomElement mime_type_element(child.firstChildElement("mime-type"));
                if(!mime_type_element.isNull())
                {
                    ca.f_mime_type = mime_type_element.text();
                }

                // there can be any number of dependencies
                // syntax is defined in the JavaScript plugin, something
                // like Debian "Depend" field:
                //
                //   <name> ( '(' (<version> <operator>)* <version> ')' )?
                //
                QDomElement dependency_element(child.firstChildElement("dependency"));
                while(!dependency_element.isNull())
                {
                    ca.f_dependencies.push_back(dependency_element.text());
                    dependency_element = dependency_element.nextSiblingElement("dependency");
                }

                ca.f_path = path;

                add_attachment(key, ca);
            }
        }
        if(!found_content_type)
        {
            QString const link_name(get_name(SNAP_NAME_CONTENT_PAGE_TYPE));
            QString const link_to(get_name(SNAP_NAME_CONTENT_PAGE));
            bool const source_unique(true);
            bool const destination_unique(false);
            QString destination_path;
            if(path.left(14) == "admin/layouts/")
            {
                // make sure that this is the root of that layout and
                // not an attachment or sub-page
                QString const base(path.mid(14));
                int const pos(base.indexOf('/'));
                if(pos < 0)
                {
                    destination_path = "types/taxonomy/system/content-types/layout-page";
                }
            }
            if(destination_path.isEmpty())
            {
                if(path.left(6) == "admin/")
                {
                    destination_path = "types/taxonomy/system/content-types/administration-page";
                }
                else
                {
                    destination_path = "types/taxonomy/system/content-types/system-page";
                }
            }
            QString const destination_key(f_snap->get_site_key_with_slash() + destination_path);
            links::link_info source(link_name, source_unique, key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            links::link_info destination(link_to, destination_unique, destination_key, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            add_link(key, source, destination);
        }
        if(!found_prevent_delete)
        {
            // add the "content::prevent_delete" to 1 on all that do not
            // set it to another value (1 byte value)
            add_param(key, get_name(SNAP_NAME_CONTENT_PREVENT_DELETE), PARAM_REVISION_GLOBAL, "en", "1");
            set_param_overwrite(key, get_name(SNAP_NAME_CONTENT_PREVENT_DELETE), true); // always overwrite
            set_param_type(key, get_name(SNAP_NAME_CONTENT_PREVENT_DELETE), PARAM_TYPE_INT8);
        }
    }
}


/** \brief Prepare to add content to the database.
 *
 * This function creates a new block of data to be added to the database.
 * Each time one wants to add content to the database, one must call
 * this function first. At this time the plugin_owner cannot be changed.
 * If that happens (i.e. two plugins trying to create the same piece of
 * content) then the system raises an exception.
 *
 * \exception content_exception_content_already_defined
 * This exception is raised if the block already exists and the owner
 * of the existing block doesn't match the \p plugin_owner parameter.
 *
 * \param[in] path  The path of the content being added.
 * \param[in] plugin_owner  The name of the plugin managing this content.
 */
void content::add_content(const QString& path, const QString& plugin_owner)
{
    if(!plugins::verify_plugin_name(plugin_owner))
    {
        // invalid plugin name
        throw content_exception_invalid_name("install_content() called with an invalid plugin name: \"" + plugin_owner + "\"");
    }

    content_block_map_t::iterator b(f_blocks.find(path));
    if(b != f_blocks.end())
    {
        if(b->f_owner != plugin_owner)
        {
            // cannot change owner!?
            throw content_exception_content_already_defined("adding block \"" + path + "\" with owner \"" + b->f_owner + "\" cannot be changed to \"" + plugin_owner + "\"");
        }
        // it already exists, we're all good
    }
    else
    {
        // create the new block
        content_block block;
        block.f_path = path;
        block.f_owner = plugin_owner;
        f_blocks.insert(path, block);
    }

    f_snap->new_content();
}


/** \brief Add a parameter to the content to be saved in the database.
 *
 * This function is used to add a parameter to the database.
 * A parameter is composed of a name and a block of data that may be of any
 * type (HTML, XML, picture, etc.)
 *
 * Other parameters can be attached to parameters using set_param_...()
 * functions, however, the add_param() function must be called first to
 * create the parameter.
 *
 * Note that the data added in this way is NOT saved in the database until
 * the save_content signal is sent.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the Cassandra function
 * to add the data to the content table.
 *
 * \bug
 * At this time the data of a parameter is silently overwritten if this
 * function is called multiple times with the same path and name.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised when this funtion is called before the
 * add_content() is called (i.e. the block of data referenced by
 * \p path is not defined yet.)
 *
 * \exception content_exception_unexpected_revision_type
 * This exception is raised if the \p revision_type parameter is not
 * equal to the revision_type that was used to create this page.
 *
 * \param[in] path  The path of this parameter (i.e. /types/taxonomy)
 * \param[in] name  The name of this parameter (i.e. "Website Taxonomy")
 * \param[in] revision_type  The type of revision for this parameter (i.e. global, branch, revision)
 * \param[in] locale  The locale (\<language>_\<country>) for this data.
 * \param[in] data  The data of this parameter.
 *
 * \sa add_param()
 * \sa add_link()
 * \sa on_save_content()
 */
void content::add_param(QString const& path, QString const& name, param_revision_t revision_type, QString const& locale, QString const& data)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" was found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        content_param param;
        param.f_name = name;
        param.f_data[locale] = data;
        param.f_revision_type = revision_type;
        b->f_params.insert(name, param);
    }
    else
    {
        // revision types cannot change between entries
        // (duplicates happen often when you have multiple languages)
        if(p->f_revision_type != revision_type)
        {
            throw content_exception_unexpected_revision_type(QString("the revision type cannot be different between locales; got %1 the first time and now %2")
                        .arg(static_cast<snap_version::basic_version_number_t>(p->f_revision_type))
                        .arg(static_cast<snap_version::basic_version_number_t>(revision_type)));
        }

        // replace the data
        // TBD: should we generate an error because if defined by several
        //      different plugins then we cannot ensure which one is going
        //      to make it to the database! At the same time, we cannot
        //      know whether we're overwriting a default value.
        p->f_data[locale] = data;
    }
}


/** \brief Set the overwrite flag to a specific parameter.
 *
 * The parameter must first be added with the add_param() function.
 * By default this is set to false as defined in the DTD of the
 * content XML format. This means if the attribute is not defined
 * then there is no need to call this function.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] overwrite  The new value of the overwrite flag.
 *
 * \sa add_param()
 */
void content::set_param_overwrite(const QString& path, const QString& name, bool overwrite)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_overwrite = overwrite;
}


/** \brief Set the type to a specific value.
 *
 * The parameter must first be added with the add_param() function.
 * By default the type of a parameter is "string". However, some
 * parameters are integers and this function can be used to specify
 * such. Note that it is important to understand that if you change
 * the type in the content.xml then when reading the data you'll have
 * to use the correct type.
 *
 * \exception content_exception_parameter_not_defined
 * This exception is raised if the path or the name parameters do
 * not match any block or parameter in that block.
 *
 * \param[in] path  The path of this parameter.
 * \param[in] name  The name of the parameter to modify.
 * \param[in] param_type  The new type for this parameter.
 *
 * \sa add_param()
 */
void content::set_param_type(QString const& path, QString const& name, param_type_t param_type)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_params_t::iterator p(b->f_params.find(name));
    if(p == b->f_params.end())
    {
        throw content_exception_parameter_not_defined("no param with name \"" + path + "\" found in block \"" + path + "\"");
    }

    p->f_type = static_cast<int>(param_type); // XXX fix cast
}


/** \brief Add a link to the specified content.
 *
 * This function links the specified content (defined by path) to the
 * specified destination.
 *
 * The source parameter defines the name of the link, the path (has to
 * be the same as path) and whether the link is unique.
 *
 * The path must already represent a block as defined by the add_content()
 * function call otherwise the function raises an exception.
 *
 * Note that the link is not searched. If it is already defined in
 * the array of links, it will simply be written twice to the
 * database.
 *
 * \warning
 * This function does NOT save the data immediately (if called after the
 * update, then it is saved after the execute() call returns!) Instead
 * the function prepares the data so it can be saved later. This is useful
 * if you expect many changes and dependencies may not all be available at
 * the time you add the content but will be at a later time. If you already
 * have all the data, you may otherwise directly call the
 * links::create_link() function.
 *
 * \exception content_exception_parameter_not_defined
 * The add_content() function was not called prior to this call.
 *
 * \param[in] path  The path where the link is added (source URI, site key + path.)
 * \param[in] source  The link definition of the source.
 * \param[in] destination  The link definition of the destination.
 *
 * \sa add_content()
 * \sa add_attachment()
 * \sa add_xml()
 * \sa add_param()
 * \sa create_link()
 */
void content::add_link(const QString& path, const links::link_info& source, const links::link_info& destination)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    content_link link;
    link.f_source = source;
    link.f_destination = destination;
    b->f_links.push_back(link);
}


/** \brief Add an attachment to the list of data to add on initialization.
 *
 * This function is used by the add_xml() function to add an attachment
 * to the database once the content and links were all created.
 *
 * Note that the \p attachment parameter does not include the actual data.
 * That data is to be loaded when the on_save_content() signal is sent.
 * This is important to avoid using a huge amount of memory on setup.
 *
 * \warning
 * To add an attachment from your plugin, make sure to call
 * create_attachment() instead. The add_attachment() is a sub-function of
 * the add_xml() feature. It will work on initialization, it is likely to
 * fail if called from your plugin.
 *
 * \param[in] path  The path (key) to the parent of the attachment.
 * \param[in] ca  The attachment information.
 *
 * \sa add_xml()
 * \sa add_link()
 * \sa add_content()
 * \sa add_param()
 * \sa on_save_content()
 * \sa create_attachment()
 */
void content::add_attachment(QString const& path, content_attachment const& ca)
{
    content_block_map_t::iterator b(f_blocks.find(path));
    if(b == f_blocks.end())
    {
        throw content_exception_parameter_not_defined("no block with path \"" + path + "\" found");
    }

    b->f_attachments.push_back(ca);
}


/** \brief Signal received when the system request that we save content.
 *
 * This function is called by the snap_child() after the update if any one of
 * the plugins requested content to be saved to the database (in most cases
 * from their content.xml file, although it could be created dynamically.)
 *
 * It may be called again after the execute() if anything more was saved
 * while processing the page.
 */
void content::on_save_content()
{
    // anything to save?
    if(f_blocks.isEmpty())
    {
        return;
    }

    QString const primary_owner(get_name(SNAP_NAME_CONTENT_PRIMARY_OWNER));
    QString const site_key(f_snap->get_site_key_with_slash());
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraTable::pointer_t branch_table(get_branch_table());
    QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        // now do the actual save
        // connect this entry to the corresponding plugin
        // (unless that field is already defined!)
        path_info_t ipath;
        ipath.set_path(d->f_path);
        path_info_t::status_t status(ipath.get_status());
        if(status.is_error())
        {
            if(status.get_error() == path_info_t::status_t::error_t::UNDEFINED)
            {
                status.reset_state(path_info_t::status_t::state_t::CREATE, path_info_t::status_t::working_t::CREATING);
                ipath.set_status(status);

                // we only set the primary owner on creation, which means
                // a plugin can take over the ownership of a page and we
                // do not reset that ownership on updates
                content_table->row(d->f_path)->cell(primary_owner)->setValue(d->f_owner);
            }
            else
            {
                throw snap_logic_exception(QString("somehow on_save_content() stumble on erroneous status %1 (%2)").arg(static_cast<int>(status.get_error())).arg(d->f_path));
            }
        }
        else
        {
            status.set_working(path_info_t::status_t::working_t::UPDATING);
            //status.reset_state(path_info_t::state_t::CREATING, path_info_t::working_t::CREATING);
            ipath.set_status(status);
        }

        // make sure we have our different basic content dates setup
        int64_t const start_date(f_snap->get_start_date());
        if(content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->value().nullValue())
        {
            // do not overwrite the created date
            content_table->row(d->f_path)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }

        // TODO: fix the locale... actually the revision for English is
        //       the default and many we do not have to create the revision
        //       field? At the same time, we could call this function with
        //       all the locales defined in the parameters.
        //
        //       Note:
        //       The first reason for adding this initialization is in link
        //       with a problem I had and that problem is now resolved. This
        //       does not mean it shouldn't be done, however, the revision
        //       is problematic because it needs to be incremented each time
        //       we do an update when at this point it won't be. (Although
        //       it seems to work fine at this point...)
        initialize_branch(d->f_path);

        // TODO: add support to specify the "revision owner" of the parameter
        QString const branch_key(QString("%1#%2").arg(d->f_path).arg(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH)));

        // do not overwrite the created date
        if(branch_table->row(branch_key)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->value().nullValue())
        {
            branch_table->row(branch_key)->cell(QString(get_name(SNAP_NAME_CONTENT_CREATED)))->setValue(start_date);
        }
        // always overwrite the modified date
        branch_table->row(branch_key)->cell(QString(get_name(SNAP_NAME_CONTENT_MODIFIED)))->setValue(start_date);

        // save the parameters (i.e. cells of data defined by the developer)
        bool use_new_revision(true);
        for(content_params_t::iterator p(d->f_params.begin());
                p != d->f_params.end(); ++p)
        {
            // make sure no parameter is defined as content::primary_owner
            // because we are 100% in control of that one!
            // (we may want to add more as time passes)
            if(p->f_name == primary_owner)
            {
                throw content_exception_invalid_content_xml("content::on_save_content() cannot accept a parameter named \"content::primary_owner\" as it is reserved");
            }

            for(QMap<QString, QString>::const_iterator data(p->f_data.begin());
                    data != p->f_data.end(); ++data)
            {
                QString const locale(data.key());

                // define the key and table affected
                QtCassandra::QCassandraTable::pointer_t param_table;
                QString row_key;
                switch(p->f_revision_type)
                {
                case PARAM_REVISION_GLOBAL:
                    // in the content table
                    param_table = content_table;
                    row_key = d->f_path;
                    break;

                case PARAM_REVISION_BRANCH:
                    // path + "#0" in the data table
                    param_table = branch_table;
                    row_key = branch_key;
                    break;

                case PARAM_REVISION_REVISION:
                    if(p->f_overwrite)
                    {
                        throw snap_logic_exception("the overwrite=\"yes\" flag cannot be used along revision=\"revision\"");
                    }

                    // path + "#xx/0.<revision>" in the data table
                    param_table = revision_table;
                    if(!use_new_revision)
                    {
                        row_key = get_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, locale, false);
                    }
                    // else row_key.clear(); -- I think it is faster to test the flag again
                    if(use_new_revision || row_key.isEmpty())
                    {
                        // the revision does not exist yet, create it
                        snap_version::version_number_t revision_number(get_new_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, locale, false));
                        set_current_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, false);
                        set_current_revision(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, true);
                        set_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, false);
                        row_key = set_revision_key(d->f_path, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, revision_number, locale, true);
                        use_new_revision = false;

                        // mark when the row was created
                        revision_table->row(row_key)->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
                    }
                    break;

                }

                // we just saved the content::primary_owner so the row exists now
                //if(content_table->exists(d->f_path)) ...

                // unless the developer said to overwrite the data, skip
                // the save if the data alerady exists
                if(p->f_overwrite
                || param_table->row(row_key)->cell(p->f_name)->value().nullValue())
                {
                    bool ok(true);
                    switch(p->f_type)
                    {
                    case PARAM_TYPE_STRING:
                        param_table->row(row_key)->cell(p->f_name)->setValue(*data);
                        break;

                    case PARAM_TYPE_FLOAT:
                        {
                            float const v(data->toFloat(&ok));
                            param_table->row(row_key)->cell(p->f_name)->setValue(v);
                        }
                        break;

                    case PARAM_TYPE_INT8:
                        {
                            int const v(data->toInt(&ok));
                            ok = ok && v >= -128 && v <= 127; // verify overflows
                            param_table->row(row_key)->cell(p->f_name)->setValue(static_cast<signed char>(v));
                        }
                        break;

                    case PARAM_TYPE_INT64:
                        param_table->row(row_key)->cell(p->f_name)->setValue(static_cast<int64_t>(data->toLongLong(&ok)));
                        break;

                    }
                    if(!ok)
                    {
                        throw content_exception_invalid_content_xml(QString("content::on_save_content() tried to convert %1 to a number and failed.").arg(*data));
                    }
                }
            }
        }

        // link this entry to its parent automatically
        // first we need to remove the site key from the path
        QString const path(d->f_path.mid(site_key.length()));
        QStringList parts(path.split('/', QString::SkipEmptyParts));
        while(parts.count() > 0)
        {
            QString src(parts.join("/"));
            src = site_key + src;
            parts.pop_back();
            QString dst(parts.join("/"));
            dst = site_key + dst;
            links::link_info source(get_name(SNAP_NAME_CONTENT_PARENT), true, src, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
            links::link_info destination(get_name(SNAP_NAME_CONTENT_CHILDREN), false, dst, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);

            // TODO: these rows generate errors because they are missing the
            //       branch and revision information generally expected; we
            //       want to create some data here so the page is "real"
            //       enough to be used (i.e. call create_content() ?)

// TODO only repeat if the parent did not exist, otherwise we assume the
//      parent created its own parent/children link already.
//printf("parent/children [%s]/[%s]\n", src.toUtf8().data(), dst.toUtf8().data());
            links::links::instance()->create_link(source, destination);
        }
    }

    // link the nodes together (on top of the parent/child links)
    // this is done as a second step so we're sure that all the source and
    // destination rows exist at the time we create the links
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        for(content_links_t::iterator l(d->f_links.begin());
                l != d->f_links.end(); ++l)
        {
//printf("developer link: [%s]/[%s]\n", l->f_source.key().toUtf8().data(), l->f_destination.key().toUtf8().data());
            links::links::instance()->create_link(l->f_source, l->f_destination);
        }
    }

    // attachments are pages too, only they require a valid parent to be
    // created and many require links to work (i.e. be assigned a type)
    // so we add them after the basic content and links
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        for(content_attachments_t::iterator a(d->f_attachments.begin());
                a != d->f_attachments.end(); ++a)
        {
            attachment_file file(f_snap);

            // attachment specific fields
            file.set_multiple(false);
            file.set_parent_cpath(a->f_path);
            file.set_field_name(a->f_field_name);
            file.set_attachment_owner(a->f_owner);
            file.set_attachment_type(a->f_type);
            file.set_creation_time(f_snap->get_start_date());
            file.set_update_time(f_snap->get_start_date());
            file.set_dependencies(a->f_dependencies);

            // post file fields
            file.set_file_name(a->f_field_name);
            file.set_file_filename(a->f_filename);
            //file.set_file_data(data);
            // TBD should we have an original MIME type defined by the
            //     user?
            //file.set_file_original_mime_type(QString const& mime_type);
            file.set_file_creation_time(f_snap->get_start_date());
            file.set_file_modification_time(f_snap->get_start_date());
            ++f_file_index; // this is more of a random number here!
            file.set_file_index(f_file_index);

            snap_child::post_file_t f;
            f.set_filename(a->f_filename);
            f_snap->load_file(f);
            file.set_file_data(f.get_data());

            // for images, also check the dimensions and if available
            // save them in there because that's useful for the <img>
            // tags (it is faster to load 8 bytes from Cassandra than
            // a whole attachment!)
            snap_image info;
            if(info.get_info(file.get_file().get_data()))
            {
                if(info.get_size() > 0)
                {
                    smart_snap_image_buffer_t buffer(info.get_buffer(0));
                    file.set_file_image_width(buffer->get_width());
                    file.set_file_image_height(buffer->get_height());
                    file.set_file_mime_type(buffer->get_mime_type());
                }
            }

            // user forces the MIME type (important for many files such as
            // JavaScript which otherwise come out with really funky types)
            if(!a->f_mime_type.isEmpty())
            {
                file.set_file_mime_type(a->f_mime_type);
            }

            // ready, create the attachment
            create_attachment(file, snap_version::SPECIAL_VERSION_SYSTEM_BRANCH, "");

            // here the data buffer gets freed!
        }
    }

    // allow other plugins to add their own stuff dynamically
    // (this mechanism is working only comme-ci comme-ca since all
    // the other plugins should anyway have workable defaults; however,
    // once in a while, defaults are not enough; for example the shorturl
    // needs to generate a shorturl, there is no real default other than:
    // that page has no shorturl.)
    f_updating = true;
    for(content_block_map_t::iterator d(f_blocks.begin());
            d != f_blocks.end(); ++d)
    {
        QString const path(d->f_path);
        if(path.startsWith(site_key))
        {
            // TODO: we may want to have a better way to choose the language
            path_info_t ipath;
            ipath.set_path(path);
            links::link_info info(get_name(SNAP_NAME_CONTENT_PAGE_TYPE), true, ipath.get_key(), ipath.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info child_info;
            if(link_ctxt->next_link(child_info))
            {
                // should always be true because all pages have a type
                QString const type_key(child_info.key());
                int const pos(type_key.indexOf("/types/taxonomy/system/content-types/"));
                create_content(ipath, d->f_owner, type_key.mid(pos + 37));
            }
        }
        // else -- if the path doesn't start with site_key we've got a problem

        path_info_t ipath;
        ipath.set_path(d->f_path);
        path_info_t::status_t status(ipath.get_status());
        status.set_working(path_info_t::status_t::working_t::NOT_WORKING);
        ipath.set_status(status);
    }
    f_updating = false;

    // we're done with that set of data, release it from memory
    f_blocks.clear();
}




/** \brief Register the resetstatus action.
 *
 * This function registers this plugin as supporting the "resetstatus"
 * actions.
 *
 * This can be used by an administrator to force a reset of all the statuses
 * of all the nodes (usually only necessary for developers although once in
 * a while it could happen that a page never gets reset properly.)
 *
 * \param[in,out] actions  The list of supported actions where we add ourselves.
 */
void content::on_register_backend_action(server::backend_action_map_t& actions)
{
    actions[get_name(SNAP_NAME_CONTENT_RESETSTATUS)] = this;
}


/** \brief Process various backend tasks.
 *
 * Content backend processes:
 *
 * \li Reset the status of pages that somehow got a working status
 *     but that status never got reset.
 *
 * \li Check new attachements as those files may be or include viruses.
 */
void content::on_backend_action(QString const& action)
{
    if(action == get_name(SNAP_NAME_CONTENT_RESETSTATUS))
    {
        backend_action_reset_status();
    }
}


void content::backend_action_reset_status()
{
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());

    QtCassandra::QCassandraRowPredicate row_predicate;
    // process 100 in a row
    row_predicate.setCount(100);
    for(;;)
    {
        content_table->clearCache();
        uint32_t const count(content_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more lists to process
            break;
        }
        QtCassandra::QCassandraRows const rows(content_table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            path_info_t ipath;
            ipath.set_path(QString::fromUtf8(o.key().data()));
            if(content_table->row(ipath.get_key())->exists(get_name(SNAP_NAME_CONTENT_STATUS)))
            {
                // do not use the normal interface, force any normal (something) to normal (normal)
                QtCassandra::QCassandraValue status(content_table->row(ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_STATUS))->value());
                if(status.nullValue())
                {
                    int32_t s(static_cast<unsigned char>(static_cast<int>(path_info_t::status_t::state_t::NORMAL))
                            + static_cast<unsigned char>(static_cast<int>(path_info_t::status_t::working_t::NOT_WORKING) * 256));
                    status.setInt32Value(s);
                }
                else
                {
                    int32_t s(status.int32Value());
                    if((s & 0xFF) == static_cast<int>(path_info_t::status_t::state_t::NORMAL)
                    && (s >> 8) != static_cast<int>(path_info_t::status_t::working_t::NOT_WORKING))
                    {
                        s = static_cast<unsigned char>(static_cast<int>(path_info_t::status_t::state_t::NORMAL))
                          + static_cast<unsigned char>(static_cast<int>(path_info_t::status_t::working_t::NOT_WORKING)) * 256;
                        status.setInt32Value(s);
                        content_table->row(ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_STATUS))->setValue(status);
                    }
                }
            }
        }
    }
}


/** \brief Process various backend tasks.
 *
 * Content backend processes:
 *
 * \li Reset the status of pages that somehow got a working status
 *     but that status never got reset.
 *
 * \li Check new attachements as those files may be or include viruses.
 */
void content::on_backend_process()
{
    backend_process_status();
    backend_process_files();
}


/** \brief Check whether a working process never reset its status.
 *
 * As the database is being worked on, the status of a page changes while
 * it gets processed. Unfortunately, once in while a page process breaks
 * and its status does not get restored as expected.
 *
 * The status handling saves the URI of the pages that get a status with
 * a working process in the processing table. The URI does not get deleted
 * for speed. This backend checks the pages, verifies the status and how
 * long it was set to a working state (if such is still the case) and
 * resets the working state to path_info_t::status_t::NOT_WORKING if the
 * working status was on for over 10 minutes.
 *
 * \note
 * A process that takes over 10 minutes can always update the date once a
 * minute or so to avoid getting erased by this backend. At this point the
 * 10 minutes was chosen somewhat arbitrarily and we may want to adjust
 * that with time and even possibly offer the administrator to change that
 * number for one's website.
 */
void content::backend_process_status()
{
    SNAP_LOG_TRACE() << "backend_process: Content status auto adjustments.";

    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraTable::pointer_t processing_table(get_processing_table());

    // any page with this start date or less gets its processing state
    // reset by this backend; we may want the 10 minutes to be saved in
    // a site parameter so the administrator can tweak it...
    int64_t start_date(f_snap->get_start_date() - 10 * 60 * 1000000);

    QtCassandra::QCassandraRowPredicate row_predicate;
    // process 100 in a row
    row_predicate.setCount(100);
    for(;;)
    {
        processing_table->clearCache();
        uint32_t const count(processing_table->readRows(row_predicate));
        if(count == 0)
        {
            // no more lists to process
            break;
        }
        QtCassandra::QCassandraRows const rows(processing_table->rows());
        for(QtCassandra::QCassandraRows::const_iterator o(rows.begin());
                o != rows.end(); ++o)
        {
            path_info_t ipath;
            ipath.set_path(QString::fromUtf8(o.key().data()));
            if(content_table->exists(ipath.get_key())
            && content_table->row(ipath.get_key())->exists(get_name(SNAP_NAME_CONTENT_STATUS_CHANGED)))
            {
                int64_t const last_changed(content_table->row(ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_STATUS_CHANGED))->value().int64Value());
                if(last_changed < start_date)
                {
                    // we are done with that page since we just reset the
                    // working status as expected so drop it (we do that first
                    // so in case it gets re-created in between, we will reset
                    // again later)
                    processing_table->dropRow(ipath.get_key(), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());

                    // it has been more than 10 minutes, reset the state
                    path_info_t::status_t status(ipath.get_status());
                    status.set_status(static_cast<path_info_t::status_t::status_type>(content_table->row(ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_STATUS))->value().uint32Value()));
                    if(status.get_state() == path_info_t::status_t::state_t::CREATE)
                    {
                        // a create failed, set it to normal...
                        // (should we instead set it to hidden?)
                        status.set_state(path_info_t::status_t::state_t::NORMAL);
                    }
                    status.set_working(path_info_t::status_t::working_t::NOT_WORKING);
                    ipath.set_status(status);
                }
            }
            else
            {
                // the row was deleted in between... or something of
                // the sort, just ignore that entry altogether
                processing_table->dropRow(ipath.get_key(), QtCassandra::QCassandraValue::TIMESTAMP_MODE_DEFINED, QtCassandra::QCassandra::timeofday());
            }
        }
    }
}


/** \brief Process new attachments to make sure they are secure.
 *
 * As user upload new files to the server, we want to have them
 * processed in different ways. This backend process does part of
 * that work and allows other plugins to check files out to make
 * sure they are fine.
 *
 * Type of processes we are expecting to run against files:
 *
 * \li The Anti-Virus plugin checks that the file is not viewed as a
 *     virus using external tools such as clamscan. This is expected
 *     to be checked within the check_attachment_security() signal.
 *
 * \li The JavaScript plugin checks the syntax of all JavaScript files.
 *     It also minimizes them and save that minimized version.
 *
 * \li The Layout plugin checks the syntax of all the CSS files and
 *     it also minimizes them and save that minimized version.
 *
 * \li The layout plugin tries to fully load all Images, play movies,
 *     etc. to make sure that the files are valid. If that process
 *     fails, then the file is marked as invalid.
 *
 * When serving a file that is an attachment, plugins that own those
 * files are given a chance to serve the attachment themselves. If
 * they do, then the default code does not get used at all. This allows
 * plugins such as the JavaScript plugin to send their compressed and
 * minimized version of the file instead of the source version.
 *
 * \warning
 * This function generates two signals: check_attachment_security()
 * and process_attachment(). If your plugin can check the file for
 * security reason, implement the check_attachment_security(). In
 * all other cases, use the process_attachment(). It is important to
 * do that work in the right function because attempting to load a
 * virus or some other bad file may cause havoc on the server.
 *
 * \todo
 * The security checks may need to be re-run on all the files once
 * in a while since brand new viruses may not be detected when they
 * first get uploaded. One signal on that one could be to count the
 * number of time a file gets uploaded, if the counter increases
 * outregiously fast, it is probably not a good sign.
 *
 * \todo
 * When the process finds content that is considered non secure,
 * send an email to the content owner.
 */
void content::backend_process_files()
{
    SNAP_LOG_TRACE() << "backend_process: Content file processing (check for viruses, etc.)";

    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    QtCassandra::QCassandraRow::pointer_t new_row(files_table->row(get_name(SNAP_NAME_CONTENT_FILES_NEW)));
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(100); // should this be a parameter?
    column_predicate.setIndex(); // behave like an index
    for(;;)
    {
        new_row->clearCache();
        new_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const new_cells(new_row->cells());
        if(new_cells.isEmpty())
        {
            break;
        }
        // handle one batch
        for(QtCassandra::QCassandraCells::const_iterator nc(new_cells.begin());
                nc != new_cells.end();
                ++nc)
        {
            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QtCassandra::QCassandraCell::pointer_t new_cell(*nc);
            if(!new_cell->value().nullValue())
            {
                QByteArray file_key(new_cell->columnKey());

                QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(file_key));
                QtCassandra::QCassandraColumnRangePredicate reference_column_predicate;
                reference_column_predicate.setStartColumnName(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE));
                reference_column_predicate.setEndColumnName(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE) + QString(";"));
                reference_column_predicate.setCount(100);
                reference_column_predicate.setIndex(); // behave like an index
                bool first(true); // load the image only once for now
                permission_flag secure;
                for(;;)
                {
                    file_row->clearCache();
                    file_row->readCells(reference_column_predicate);
                    QtCassandra::QCassandraCells const content_cells(file_row->cells());
                    if(content_cells.isEmpty())
                    {
                        break;
                    }
                    // handle one batch
                    for(QtCassandra::QCassandraCells::const_iterator cc(content_cells.begin());
                            cc != content_cells.end();
                            ++cc)
                    {
                        // get the email from the database
                        // we expect empty values once in a while because a dropCell() is
                        // not exactly instantaneous in Cassandra
                        QtCassandra::QCassandraCell::pointer_t content_cell(*cc);
                        if(!content_cell->value().nullValue())
                        {
                            QByteArray attachment_key(content_cell->columnKey().data() + (strlen(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2),
                                     static_cast<int>(content_cell->columnKey().size() - (strlen(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)) + 2)));

                            if(first)
                            {
                                first = false;

                                attachment_file file(f_snap);
                                if(!load_attachment(attachment_key, file, true))
                                {
                                    signed char const sflag(CONTENT_SECURE_UNDEFINED);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(QString("Attachment could not be loaded."));

                                    // TODO generate an email about the error...
                                }
                                else
                                {
                                    check_attachment_security(file, secure, false);

                                    // always save the secure flag
                                    signed char const sflag(secure.allowed() ? CONTENT_SECURE_SECURE : CONTENT_SECURE_INSECURE);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->setValue(sflag);
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE_LAST_CHECK))->setValue(f_snap->get_start_date());
                                    file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURITY_REASON))->setValue(secure.reason());

                                    if(secure.allowed())
                                    {
                                        // only process the attachment further if it is
                                        // considered secure
                                        process_attachment(file_key, file);
                                    }
                                }
                            }
                            if(!secure.allowed())
                            {
                                // TODO: warn the author that his file was
                                //       quanranteened and will not be served
                                //...sendmail()...
                            }
                        }
                    }
                }
            }
            // we're done with that file, remove it from the list of new files
            new_row->dropCell(new_cell->columnKey());
        }
    }
}


/** \fn void check_attachment_security(attachment_file const& file, permission_flag& secure, bool const fast)
 * \brief Check whether the attachment is considered secure.
 *
 * Before processing an attachment further we want to know whether it is
 * secure. This event allows different plugins to check the security of
 * each file.
 *
 * Once a process decides that a file is not secure, the secure flag is
 * false and it cannot be reset back to true.
 *
 * \param[in] file  The file being processed.
 * \param[in,out] secure  Whether the file is secure.
 * \param[in] fast  If true only perform fast checks (i.e. not the virus check).
 */


/** \brief Check the attachment for one thing or another.
 *
 * The startup function generates a compressed version of the file using
 * gzip as the compression mode.
 *
 * \param[in] key  The key of the file in the files table.
 * \param[in] file  The file being processed.
 */
bool content::process_attachment_impl(QByteArray const& file_key, attachment_file const& file)
{
    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    QtCassandra::QCassandraRow::pointer_t file_row(files_table->row(file_key));
    if(!file_row->exists(get_name(SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED)))
    {
        QString compressor_name("gzip");
        QByteArray compressed_file(compression::compress(compressor_name, file.get_file().get_data(), 100, false));
        if(compressor_name == "gzip")
        {
            // compression succeeded
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_DATA_GZIP_COMPRESSED))->setValue(compressed_file);
            uint64_t const compressed_size(compressed_file.size());
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED))->setValue(compressed_size);
        }
        else
        {
            // no better when compressed, mark such with a size of zero
            uint64_t const empty_size(0);
            file_row->cell(get_name(SNAP_NAME_CONTENT_FILES_SIZE_GZIP_COMPRESSED))->setValue(empty_size);
        }
    }

    return true;
}


/** \brief Add a javascript to the page.
 *
 * This function adds a javascript and all of its dependencies to the page.
 * If the script was already added, either immediately or as a dependency
 * of another script, then nothing more happens.
 *
 * \param[in,out] doc  The XML document receiving the javascript.
 * \param[in] name  The name of the script.
 */
void content::add_javascript(QDomDocument doc, QString const& name)
{
    // TBD: it may make sense to move to the javascript plugin since it now
    //      can include the content plugin; the one advantage would be that
    //      the get_name() from the JavaScript plugin would then make use
    //      of the "local" SNAP_NAME_JAVASCRIPT_...
    if(f_added_javascripts.contains(name))
    {
        // already added, we're done
        return;
    }
    f_added_javascripts[name] = true;

    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    if(!files_table->exists("javascripts"/*javascript::get_name(javascript::SNAP_NAME_JAVASCRIPT_ROW)--incorrect dependency*/))
    {
        // absolutely no JavaScripts available!
        f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "JavaScript Not Found",
                "JavaScript \"" + name + "\" could not be read for inclusion in your HTML page.",
                "A JavaScript was requested in the \"files\" table before it was inserted under /js/...");
        NOTREACHED();
    }
    QtCassandra::QCassandraRow::pointer_t javascript_row(files_table->row("javascripts"/*javascript::get_name(javascript::SNAP_NAME_JAVASCRIPT_ROW)*/));

    // TODO: at this point I read all the entries with "name_..."
    //       we'll want to first check with the user's browser and
    //       then check with "any" as the browser name if no specific
    //       script is found
    //
    //       Also the following loop does NOT handle dependencies in
    //       a full tree to determine what would be best; instead it
    //       makes uses of the latest and if a file does not match
    //       the whole process fails even if by not using the latest
    //       would have worked
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(10); // small because we are really only interested by the first 1 unless marked as insecure
    column_predicate.setIndex(); // behave like an index
    column_predicate.setStartColumnName(name + "`"); // start/end keys are reversed
    column_predicate.setEndColumnName(name + "_");
    column_predicate.setReversed(); // read the last first
    for(;;)
    {
        javascript_row->clearCache();
        javascript_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const cells(javascript_row->cells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        QMapIterator<QByteArray, QtCassandra::QCassandraCell::pointer_t> c(cells);
        c.toBack();
        while(c.hasPrevious())
        {
            c.previous();

            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QtCassandra::QCassandraCell::pointer_t cell(c.value());
            QtCassandra::QCassandraValue const file_md5(cell->value());
            if(file_md5.nullValue())
            {
                // cell is invalid?
                SNAP_LOG_ERROR("invalid JavaScript MD5 for \"")(name)("\", it is empty");
                continue;
            }
            QByteArray const key(file_md5.binaryValue());
            if(!files_table->exists(key))
            {
                // file does not exist?!
                // TODO: we probably want to report that problem
                SNAP_LOG_ERROR("JavaScript for \"")(name)("\" could not be found with its MD5");
                continue;
            }
            QtCassandra::QCassandraRow::pointer_t row(files_table->row(key));
            if(!row->exists(get_name(SNAP_NAME_CONTENT_FILES_SECURE)))
            {
                // secure field missing?! (file was probably deleted)
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" does not have a ")(get_name(SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            QtCassandra::QCassandraValue const secure(row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->value());
            if(secure.nullValue())
            {
                // secure field missing?!
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" has an empty ")(get_name(SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            signed char const sflag(secure.signedCharValue());
            if(sflag == CONTENT_SECURE_INSECURE)
            {
                // not secure
#ifdef DEBUG
                SNAP_LOG_DEBUG("JavaScript named \"")(name)("\" is marked as being insecure");
#endif
                continue;
            }

            // we want to get the full URI to the script
            // (WARNING: the filename is only the name used for the very first
            //           upload the very first time that file is loaded and
            //           different websites may have used different filenames)
            //
            // TODO: allow for remote paths by checking a flag in the file
            //       saying "remote" (i.e. to use Google Store and alike)
            QtCassandra::QCassandraColumnRangePredicate references_column_predicate;
            references_column_predicate.setCount(1);
            references_column_predicate.setIndex(); // behave like an index
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const start_ref(QString("%1::%2").arg(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(site_key));
            references_column_predicate.setStartColumnName(start_ref);
            references_column_predicate.setEndColumnName(start_ref + QtCassandra::QCassandraColumnPredicate::last_char);

            row->clearCache();
            row->readCells(references_column_predicate);
            QtCassandra::QCassandraCells const ref_cells(row->cells());
            if(ref_cells.isEmpty())
            {
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" has no reference back to ")(site_key);
                continue;
            }
            // the key of this cell is the path we want to use to the file
            QtCassandra::QCassandraCell::pointer_t ref_cell(*ref_cells.begin());
            QtCassandra::QCassandraValue const ref_string(ref_cell->value());
            if(ref_string.nullValue()) // bool true cannot be empty
            {
                SNAP_LOG_ERROR("file referenced as JavaScript \"")(name)("\" has an invalid reference back to ")(site_key)(" (empty)");
                continue;
            }

            // file exists and is considered secure

            // we want to first add all dependencies since they need to
            // be included first, so there is another sub-loop for that
            // note that all of those must be loaded first but the order
            // we read them as does not matter
            QtCassandra::QCassandraColumnRangePredicate dependencies_column_predicate;
            dependencies_column_predicate.setCount(100);
            dependencies_column_predicate.setIndex(); // behave like an index
            QString start_dep(QString("%1:").arg(get_name(SNAP_NAME_CONTENT_FILES_DEPENDENCY)));
            dependencies_column_predicate.setStartColumnName(start_dep + ":");
            dependencies_column_predicate.setEndColumnName(start_dep + ";");
            for(;;)
            {
                row->clearCache();
                row->readCells(dependencies_column_predicate);
                QtCassandra::QCassandraCells const dep_cells(row->cells());
                if(dep_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator dc(dep_cells.begin());
                        dc != dep_cells.end();
                        ++dc)
                {
                    // get the email from the database
                    // we expect empty values once in a while because a dropCell() is
                    // not exactly instantaneous in Cassandra
                    QtCassandra::QCassandraCell::pointer_t dep_cell(*dc);
                    QtCassandra::QCassandraValue const dep_string(dep_cell->value());
                    if(!dep_string.nullValue())
                    {
                        snap_version::dependency dep;
                        if(dep.set_dependency(dep_string.stringValue()))
                        {
                            // TODO: add version and browser tests
                            QString const& dep_name(dep.get_name());
                            QString const& dep_namespace(dep.get_namespace());
                            if(dep_namespace == "css")
                            {
                                add_css(doc, dep_name);
                            }
                            else if(dep_namespace.isEmpty() || dep_namespace == "javascript")
                            {
                                add_javascript(doc, dep_name);
                            }
                            else
                            {
                                f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "Invalid Dependency",
                                        QString("JavaScript dependency \"%1::%2\" has a non-supported namespace.").arg(dep_namespace).arg(name),
                                        QString("The namespace is expected to be \"javascripts\" (or empty,) or \"css\"."));
                                NOTREACHED();
                            }
                        }
                        // else TBD -- we checked when saving that darn string
                        //             so failures should not happen here
                    }
                    // else TBD -- error if empty? (should not happen...)
                }
            }

            // TBD: At this point we get a bare name, no version, no browser.
            //      This means the loader will pick the latest available
            //      version with the User Agent match. This may not always
            //      be desirable though.
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "Adding JavaScript [" << name << "] [" << ref_cell->columnName().mid(start_ref.length() - 1) << "]";
//#endif
            QDomNodeList metadata(doc.elementsByTagName("metadata"));
            QDomNode javascript_tag(metadata.at(0).firstChildElement("javascript"));
            if(javascript_tag.isNull())
            {
                javascript_tag = doc.createElement("javascript");
                metadata.at(0).appendChild(javascript_tag);
            }
            QDomElement script_tag(doc.createElement("script"));
            script_tag.setAttribute("src", ref_cell->columnName().mid(start_ref.length() - 1));
            script_tag.setAttribute("type", "text/javascript");
            script_tag.setAttribute("charset", "utf-8");
            javascript_tag.appendChild(script_tag);
            return; // we're done since we found our script and added it
        }
    }

    f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "JavaScript Not Found",
            "JavaScript \"" + name + "\" was not found. Was it installed?",
            "The named JavaScript was not found in the \"javascripts\" row of the \"files\" table.");
    NOTREACHED();
}


/** \brief Add a CSS to the page.
 *
 * This function adds a CSS and all of its dependencies to the page.
 * If the CSS was already added, either immediately or as a dependency
 * of another CSS, then nothing more happens.
 *
 * \param[in,out] doc  The XML document receiving the CSS.
 * \param[in] name  The name of the script.
 */
void content::add_css(QDomDocument doc, QString const& name)
{
    if(f_added_css.contains(name))
    {
        // already added, we're done
        return;
    }
    f_added_css[name] = true;

    QtCassandra::QCassandraTable::pointer_t files_table(get_files_table());
    if(!files_table->exists("css"))
    {
        // absolutely no CSS available!
        f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "CSS Not Found",
                "CSS \"" + name + "\" could not be read for inclusion in your HTML page.",
                "A CSS was requested in the \"files\" table before it was inserted under /css/...");
        NOTREACHED();
    }
    QtCassandra::QCassandraRow::pointer_t css_row(files_table->row("css"));

    // TODO: at this point I read all the entries with "name_..."
    //       we'll want to first check with the user's browser and
    //       then check with "any" as the browser name if no specific
    //       file is found
    //
    //       Also the following loop does NOT handle dependencies in
    //       a full tree to determine what would be best; instead it
    //       makes uses of the latest and if a file does not match
    //       the whole process fails even if by not using the latest
    //       would have worked
    QtCassandra::QCassandraColumnRangePredicate column_predicate;
    column_predicate.setCount(10); // small because we are really only interested by the first 1 unless marked as insecure
    column_predicate.setIndex(); // behave like an index
    column_predicate.setStartColumnName(name + "`"); // start/end keys are reversed
    column_predicate.setEndColumnName(name + "_");
    column_predicate.setReversed(); // read the last first
    for(;;)
    {
        css_row->clearCache();
        css_row->readCells(column_predicate);
        QtCassandra::QCassandraCells const cells(css_row->cells());
        if(cells.isEmpty())
        {
            break;
        }
        // handle one batch
        QMapIterator<QByteArray, QtCassandra::QCassandraCell::pointer_t> c(cells);
        c.toBack();
        while(c.hasPrevious())
        {
            c.previous();

            // get the email from the database
            // we expect empty values once in a while because a dropCell() is
            // not exactly instantaneous in Cassandra
            QtCassandra::QCassandraCell::pointer_t cell(c.value());
            QtCassandra::QCassandraValue const file_md5(cell->value());
            if(file_md5.nullValue())
            {
                // cell is invalid?
                SNAP_LOG_ERROR("invalid CSS MD5 for \"")(name)("\", it is empty");
                continue;
            }
            QByteArray const key(file_md5.binaryValue());
            if(!files_table->exists(key))
            {
                // file does not exist?!
                // TODO: we probably want to report that problem
                SNAP_LOG_ERROR("CSS for \"")(name)("\" could not be found with its MD5");
                continue;
            }
            QtCassandra::QCassandraRow::pointer_t row(files_table->row(key));
            if(!row->exists(get_name(SNAP_NAME_CONTENT_FILES_SECURE)))
            {
                // secure field missing?! (file was probably deleted)
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" does not have a ")(get_name(SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            QtCassandra::QCassandraValue const secure(row->cell(get_name(SNAP_NAME_CONTENT_FILES_SECURE))->value());
            if(secure.nullValue())
            {
                // secure field missing?!
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has an empty ")(get_name(SNAP_NAME_CONTENT_FILES_SECURE))(" field");
                continue;
            }
            signed char const sflag(secure.signedCharValue());
            if(sflag == CONTENT_SECURE_INSECURE)
            {
                // not secure
#ifdef DEBUG
                SNAP_LOG_DEBUG("CSS named \"")(name)("\" is marked as being insecure");
#endif
                continue;
            }

            // we want to get the full URI to the CSS file
            // (WARNING: the filename is only the name used for the very first
            //           upload the very first time that file is loaded and
            //           different websites may have used different filenames)
            //
            // TODO: allow for remote paths by checking a flag in the file
            //       saying "remote" (i.e. to use Google Store and alike)
            QtCassandra::QCassandraColumnRangePredicate references_column_predicate;
            references_column_predicate.setCount(1);
            references_column_predicate.setIndex(); // behave like an index
            QString const site_key(f_snap->get_site_key_with_slash());
            QString const start_ref(QString("%1::%2").arg(get_name(SNAP_NAME_CONTENT_FILES_REFERENCE)).arg(site_key));
            references_column_predicate.setStartColumnName(start_ref);
            references_column_predicate.setEndColumnName(start_ref + QtCassandra::QCassandraColumnPredicate::last_char);

            row->clearCache();
            row->readCells(references_column_predicate);
            QtCassandra::QCassandraCells const ref_cells(row->cells());
            if(ref_cells.isEmpty())
            {
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has no reference back to ")(site_key);
                continue;
            }
            // the key of this cell is the path we want to use to the file
            QtCassandra::QCassandraCell::pointer_t ref_cell(*ref_cells.begin());
            QtCassandra::QCassandraValue const ref_string(ref_cell->value());
            if(ref_string.nullValue()) // bool true cannot be empty
            {
                SNAP_LOG_ERROR("file referenced as CSS \"")(name)("\" has an invalid reference back to ")(site_key)(" (empty)");
                continue;
            }

            // file exists and is considered secure

            // we want to first add all dependencies since they need to
            // be included first, so there is another sub-loop for that
            // note that all of those must be loaded first but the order
            // we read them as does not matter
            QtCassandra::QCassandraColumnRangePredicate dependencies_column_predicate;
            dependencies_column_predicate.setCount(100);
            dependencies_column_predicate.setIndex(); // behave like an index
            QString start_dep(QString("%1::").arg(get_name(SNAP_NAME_CONTENT_FILES_DEPENDENCY)));
            dependencies_column_predicate.setStartColumnName(start_dep);
            dependencies_column_predicate.setEndColumnName(start_dep + QtCassandra::QCassandraColumnPredicate::last_char);
            for(;;)
            {
                row->clearCache();
                row->readCells(dependencies_column_predicate);
                QtCassandra::QCassandraCells const dep_cells(row->cells());
                if(dep_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator dc(dep_cells.begin());
                        dc != dep_cells.end();
                        ++dc)
                {
                    // get the email from the database
                    // we expect empty values once in a while because a dropCell() is
                    // not exactly instantaneous in Cassandra
                    QtCassandra::QCassandraCell::pointer_t dep_cell(*dc);
                    QtCassandra::QCassandraValue const dep_string(dep_cell->value());
                    if(!dep_string.nullValue())
                    {
                        snap_version::dependency dep;
                        if(dep.set_dependency(dep_string.stringValue()))
                        {
                            // TODO: add version and browser tests
                            QString const& dep_name(dep.get_name());
                            add_css(doc, dep_name);
                        }
                        // else TBD -- we checked when saving that darn string
                        //             so failures should not happen here
                    }
                    // else TBD -- error if empty? (should not happen...)
                }
            }

            // TBD: At this point we get a bare name, no version, no browser.
            //      This means the loader will pick the latest available
            //      version with the User Agent match. This may not always
            //      be desirable though.
//#ifdef DEBUG
//SNAP_LOG_TRACE() << "Adding CSS [" << name << "] [" << ref_cell->columnName().mid(start_ref.length() - 1) << "]";
//#endif
            QDomNodeList metadata(doc.elementsByTagName("metadata"));
            QDomNode css_tag(metadata.at(0).firstChildElement("css"));
            if(css_tag.isNull())
            {
                css_tag = doc.createElement("css");
                metadata.at(0).appendChild(css_tag);
            }
            QDomElement link_tag(doc.createElement("link"));
            link_tag.setAttribute("href", ref_cell->columnName().mid(start_ref.length() - 1));
            link_tag.setAttribute("type", "text/css");
            link_tag.setAttribute("rel", "stylesheet");
            css_tag.appendChild(link_tag);
            return; // we're done since we found our script and added it
        }
    }

    f_snap->die(snap_child::HTTP_CODE_NOT_FOUND, "CSS Not Found",
            "CSS \"" + name + "\" was not found. Was it installed?",
            "The named CSS was not found in the \"css\" row of the \"files\" table.");
    NOTREACHED();
}


/** \brief Handle the content specific links from a cloned page.
 *
 * This function repairs parent links.
 */
void content::repair_link_of_cloned_page(QString const& clone, snap_version::version_number_t branch_number, links::link_info const& source, links::link_info const& destination, bool const cloning)
{
    if(source.name() == get_name(SNAP_NAME_CONTENT_PARENT)
    && destination.name() == get_name(SNAP_NAME_CONTENT_CHILDREN))
    {
        // this is a special case as the cloned page parent is in most
        // cases not the same as the cloned page's parent page; for
        // example, if you put a page in the trashcan, the parent of
        // the new page is /trashcan/!
        path_info_t child;
        child.set_path(clone);
        path_info_t parent;
        child.get_parent(parent);
        links::link_info src(get_name(SNAP_NAME_CONTENT_PARENT), true, clone, branch_number);
        links::link_info dst(get_name(SNAP_NAME_CONTENT_CHILDREN), false, parent.get_key(), get_current_branch(parent.get_key(), true));
        links::links::instance()->create_link(src, dst);
    }
    else if(source.name() == get_name(SNAP_NAME_CONTENT_PAGE_TYPE)
    && destination.name() == get_name(SNAP_NAME_CONTENT_PAGE))
    {
        links::link_info src(get_name(SNAP_NAME_CONTENT_PAGE_TYPE), true, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    else if(!cloning
    && source.name() == get_name(SNAP_NAME_CONTENT_CHILDREN)
    && destination.name() == get_name(SNAP_NAME_CONTENT_PARENT))
    {
        // copy the children links only if we are not cloning
        links::link_info src(source.name(), false, clone, branch_number);
        links::links::instance()->create_link(src, destination);
    }
    // else -- ignore all others for now
}


/** \brief Get the page cloned.
 *
 * This signal is captured here because the links cannot work on
 * the cloned tree directly (the links.h header cannot include
 * the content.h file).
 *
 * So here we call functions on the links plugin to make it all
 * work. The good thing (side effect) is that all the links are
 * fixed by the time the other plugins page_cloned() function
 * gets called.
 *
 * \param[in] tree  The tree of nodes that were cloned.
 *
 * \return always true so other modules always receive the signal.
 */
bool content::page_cloned_impl(cloned_tree_t const& tree)
{
    links::links *link_plugin(links::links::instance());
    size_t const max_pages(tree.f_pages.size());
    for(size_t idx(0); idx < max_pages; ++idx)
    {
        cloned_page_t const& page(tree.f_pages[idx]);
        size_t const max_branches(page.f_branches.size());
        for(size_t branch(0); branch < max_branches; ++branch)
        {
            snap_version::version_number_t const b(page.f_branches[branch].f_branch);
            path_info_t source(page.f_source);
            path_info_t destination(page.f_destination);
            source.force_branch(b);
            destination.force_branch(b);
            link_plugin->adjust_links_after_cloning(source.get_branch_key(), destination.get_branch_key());
        }
    }

    // always return true
    return true;
}


/** \brief Copy a page to another location and additional features.
 *
 * This function is used to properly copy a page to another location.
 *
 * This feature is used by many others such as the "trash page" in which
 * case the page is "moved" to the trashcan. In that case, the existing
 * page is copied to the trashcan and the source is marked as deleted
 * (path_info_t::status_t::DELETED)
 *
 * It can also be used to simply clone a page to another location before
 * working on that clone (i.e. that way you can offer templates for
 * various type of pages...)
 *
 * \warning
 * This function DOES NOT verify that a page can be cloned the way you
 * are requesting the page to be cloned. In other words, as a programmer,
 * you can create a big mess. This can be necessary when a module takes
 * over another module data, however, for end users, it is very dangerous.
 * It is preferable that you call another function such as the move_page()
 * and trash_page() functions.
 *
 * \important
 * A clone is a copy which becomes its very own version of the page. In
 * other words it is a page in its own right and it does not behave like
 * a hard or soft link (i.e. if you edit the original, the copy is not
 * affected and vice versa.)
 *
 * \todo
 * At this point the destination MUST be non-existant which works for our
 * main purposes. However, to restore a previously deleted object, or
 * move a page back and forth between two paths, we need to be able to
 * overwrite the current destination. We should have a form of mode for
 * the clone function, mode which defines what we do in various 
 * "complex" situations.
 *
 * \todo
 * As we add a mode, we may want to offer a way to create a clone with
 * just the latest branch and not all the branches and revisions. At
 * this point we are limited to copying everything (which is good when
 * sending a page to the trashcan, but not so good when doing a "quick
 * clone".)
 *
 * \param[in] source  The source being cloned.
 * \param[in] destination  The destination where the source gets copied.
 *
 * \return true if the cloning worked smoothly, false otherwise
 *
 * \sa move_page()
 * \sa trash_page()
 */
bool content::clone_page(clone_info_t& source, clone_info_t& destination)
{

// WARNING: This function is NOT yet functional, I am still looking into how
//          to make the cloning work so it is not all incorrect

    struct sub_function
    {
        sub_function(content *content_plugin, clone_info_t& source, clone_info_t& destination, int64_t const start_date)
            : f_content_plugin(content_plugin)
            , f_source(source)
            , f_destination(destination)
            , f_start_date(start_date)
            , f_content_table(f_content_plugin->get_content_table())
            , f_branch_table(f_content_plugin->get_branch_table())
            , f_revision_table(f_content_plugin->get_revision_table())
            , f_clones(source, destination)
            //, f_result(true) -- auto-init
        {
        }

        void clone_tree()
        {
            // make sure the destination does not exist, if it does,
            // we cannot create the clone
            //
            // if the parent does not exist, then all the children won't
            // exist either so we can do that test just once at the top
            //
            // TODO: add support for that case (i.e. to overwrite page A
            //       with page B data; we may want to first move page A
            //       to the trashcan though, and then allow the overwrite
            //       if the destination is marked as "deleted")
            if(f_content_table->exists(f_destination.f_ipath.get_key()))
            {
                SNAP_LOG_ERROR("clone_page() called with a destination (")(f_destination.f_ipath.get_key())(") which already exists.");
                f_result = false;
                return;
            }

            // we can clone the parent most page as is, then we go through
            // the children, and the children of the children, etc.
            clone_page(f_source.f_ipath, f_destination.f_ipath);

            // now tell all the other plugins that we just cloned a page
            f_content_plugin->page_cloned(f_clones);
        }

        void clone_children(path_info_t source_parent, path_info_t destination_parent)
        {
            QString const source_key(source_parent.get_key());
            links::link_info info(get_name(SNAP_NAME_CONTENT_CHILDREN), false, source_key, source_parent.get_branch());
            QSharedPointer<links::link_context> link_ctxt(links::links::instance()->new_link_context(info));
            links::link_info child_info;
            while(link_ctxt->next_link(child_info))
            {
                path_info_t src, dst;
                src.set_path(child_info.key());
                destination_parent.get_child(dst, child_info.key().mid(source_key.length()));
                clone_page(src, dst);
            }
        }

        void clone_page(path_info_t source, path_info_t destination)
        {
            // setup the status using RAII
            path_info_t::status_t src_now(source.get_status());
            if(src_now.is_working())
            {
                // we cannot work on a page when another process is already
                // working on that page...
                SNAP_LOG_ERROR("clone_page() called with a source (")(source.get_key())(") which is being processed now (working: ")(static_cast<int>(src_now.get_working()))(").");
                f_result = false;
                return;
            }
            path_info_t::raii_status_t source_state(source, f_source.f_processing_state, f_source.f_done_state);

            // nothing to check for the destination,
            // at this point the current status would be undefined
            // (should be extended in the future though...)
            path_info_t::raii_status_t destination_state(destination, f_destination.f_processing_state, f_destination.f_done_state);

            // save the date when we cloned the page
            f_content_table->row(destination.get_key())->cell(get_name(SNAP_NAME_CONTENT_CLONED))->setValue(f_start_date);

            // the content table is just one row, we specialize it because
            // we can directly fix the branch/revision information (and that
            // makes it a lot easier and safer to manage the whole thing)
            copy_content(source, destination);

            // copy all branches and their revisions,
            //
            // the difference here is that we may have many branches and
            // thus many rows to copy; using the last_branch parameter we
            // can find all the branches with a simple sweap, then use the
            // dbutil copy function to copy the data
            //
            // Each branch has one or more revisions, these are copied at
            // the same time
            //
            // TODO: add support to only copy the current branches (current
            //       and working); or "the last few branches"
            cloned_page_t page;
            page.f_source = source;
            page.f_destination = destination;
            copy_branches(page);
            f_clones.f_pages.push_back(page);

            clone_children(source, destination);
        }

        void copy_content(path_info_t& source, path_info_t& destination)
        {
            QString revision_control(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL));
            QString current_branch_key(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_BRANCH_KEY));
            QString current_working_branch_key(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_BRANCH_KEY));
            QString current_revision_key(QString("::%1::").arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_REVISION_KEY)));
            QString current_working_revision_key(QString("::%1::").arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_CURRENT_WORKING_REVISION_KEY)));
            // copy the main row in the content table by hand because
            // otherwise we would have problems with the status and a
            // few other things; also that way we can immediately fix
            // the branch and revision URIs
            QtCassandra::QCassandraRow::pointer_t source_row(f_content_table->row(source.get_key()));
            QtCassandra::QCassandraRow::pointer_t destination_row(f_content_table->row(destination.get_key()));
            QtCassandra::QCassandraColumnRangePredicate column_predicate;
            column_predicate.setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
            column_predicate.setIndex(); // behave like an index
            for(;;)
            {
                source_row->clearCache();
                source_row->readCells(column_predicate);
                QtCassandra::QCassandraCells const source_cells(source_row->cells());
                if(source_cells.isEmpty())
                {
                    // done
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                        nc != source_cells.end();
                        ++nc)
                {
                    QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
                    QByteArray cell_key(source_cell->columnKey());
                    // ignore the status
                    if(strcmp(cell_key.data(), get_name(SNAP_NAME_CONTENT_STATUS)) != 0
                    && strcmp(cell_key.data(), get_name(SNAP_NAME_CONTENT_STATUS_CHANGED)) != 0
                    && strcmp(cell_key.data(), get_name(SNAP_NAME_CONTENT_CLONED)) != 0)
                    {
                        QString key(QString::fromAscii(cell_key.data()));
                        if(key.startsWith(revision_control)
                        && (key.endsWith(current_branch_key)
                         || key.endsWith(current_working_branch_key)
                         || key.contains(current_revision_key)
                         || key.contains(current_working_revision_key)))
                        {
                            QString uri(source_cell->value().stringValue());
                            if(uri.startsWith(source.get_key()))
                            {
                                // fix the key so it matches the destination properly
                                uri = destination.get_key() + uri.mid(source.get_key().length());
                                destination_row->cell(cell_key)->setValue(uri);
                            }
                            else
                            {
                                // TODO: verify that this is not actually an error?
                                destination_row->cell(cell_key)->setValue(source_cell->value());
                            }
                        }
                        else
                        {
                            // anything else gets copied as is for now
                            destination_row->cell(cell_key)->setValue(source_cell->value());
                        }
                    }
                }
            }
        }

        void copy_branches(cloned_page_t& page)
        {
            // WARNING: Do not even remotely try to use a row predicate
            //          along the setStartRowName() and setEndRowName()
            //          functions because rows are NOT sorted using their
            //          key as is. Instead they use an MD5 checksum which
            //          is completely different.

            QString const source_key(page.f_source.get_key());
            QString const destination_key(page.f_destination.get_key());

            // retrieve the last branch (inclusive)
            QString last_branch_key(QString("%1::%2")
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_BRANCH)));
            QtCassandra::QCassandraValue last_branch_value(f_content_table->row(source_key)->cell(last_branch_key)->value());
            snap_version::version_number_t last_branch;
            if(last_branch_value.nullValue())
            {
                // some assumption, the last branch should always be defined
                last_branch = 1;
            }
            else
            {
                last_branch = last_branch_value.uint32Value();
            }

            QString const links_namespace(QString("%1::").arg(links::get_name(links::SNAP_NAME_LINKS_NAMESPACE)));
            QByteArray const links_bytearray(links_namespace.toAscii());

            // all the names end with '#' and the <branch> number
            // some branches may not exist (partial copy, branch zero)
            for(snap_version::version_number_t branch(0); branch <= last_branch; ++branch)
            {
                QString const source_uri(f_content_plugin->generate_branch_key(source_key, branch));
                QString const destination_uri(f_content_plugin->generate_branch_key(destination_key, branch));
                if(f_branch_table->exists(source_uri)
                && f_branch_table->row(source_uri)->exists(get_name(SNAP_NAME_CONTENT_CREATED)))
                {
                    cloned_branch_t cloned_branch;
                    cloned_branch.f_branch = branch;

                    //dbutils::copy_row(f_branch_table, source_uri, f_branch_table, destination_uri);
                    // Handle our own copy to avoid copying the links because
                    // it could cause all sorts of weird side effects (i.e.
                    // wrong parent, wrong children to cite only those two...)
                    QtCassandra::QCassandraRow::pointer_t source_row(f_branch_table->row(source_uri));
                    QtCassandra::QCassandraRow::pointer_t destination_row(f_branch_table->row(destination_uri));
                    QtCassandra::QCassandraColumnRangePredicate column_predicate;
                    column_predicate.setCount(1000); // we have to copy everything also it is likely very small (i.e. 10 fields...)
                    column_predicate.setIndex(); // behave like an index
                    for(;;)
                    {
                        source_row->clearCache();
                        source_row->readCells(column_predicate);
                        QtCassandra::QCassandraCells const source_cells(source_row->cells());
                        if(source_cells.isEmpty())
                        {
                            // done
                            break;
                        }
                        // handle one batch
                        for(QtCassandra::QCassandraCells::const_iterator nc(source_cells.begin());
                                nc != source_cells.end();
                                ++nc)
                        {
                            QtCassandra::QCassandraCell::pointer_t source_cell(*nc);
                            QByteArray cell_key(source_cell->columnKey());
                            // ignore all links
                            if(!cell_key.startsWith(links_bytearray))
                            {
                                // anything else gets copied as is for now
                                destination_row->cell(source_cell->columnKey())->setValue(source_cell->value());
                            }
                        }
                    }

                    // copy all revisions
                    //
                    // this is very similar to the branch copy, only it uses
                    // the revision table and the last revision information
                    // for that branch
                    copy_revisions(page, cloned_branch);

                    page.f_branches.push_back(cloned_branch);

                    // link both pages together in this branch
                    {
                        // note: we do not need a specific revision when
                        //       creating a link, however, we do need a
                        //       specific branch so we create a new path
                        //       info with the right branch, but leave
                        //       the revision to whatever it is by default
                        path_info_t si;
                        bool source_unique(false);
                        si.set_path(page.f_source.get_key());
                        si.force_branch(branch);
                        char const *clone_name(get_name(SNAP_NAME_CONTENT_CLONE));
                        links::link_info link_source(clone_name, source_unique, si.get_key(), si.get_branch());

                        path_info_t di;
                        bool destination_unique(true);
                        di.set_path(page.f_destination.get_key());
                        di.force_branch(branch);
                        char const *original_page_name(get_name(SNAP_NAME_CONTENT_ORIGINAL_PAGE));
                        links::link_info link_destination(original_page_name, destination_unique, di.get_key(), di.get_branch());

                        links::links::instance()->create_link(link_source, link_destination);
                    }
                }
            }
        }

        void copy_revisions(cloned_page_t& page, cloned_branch_t& cloned_branch)
        {
            // TODO: add support to only copy the current revisions
            //       (current and working, or a few latest revisions)
            QString const source_key(page.f_source.get_key());
            QString const destination_key(page.f_destination.get_key());

            // retrieve the last revision (inclusive)
            // we have to use a predicate because there may be various
            // languages for each branch; so we have a loop per
            // branch/language and then an inner loop for each revision
            QString last_revision_key(QString("%1::%2::%3")
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL))
                    .arg(get_name(SNAP_NAME_CONTENT_REVISION_CONTROL_LAST_REVISION))
                    .arg(cloned_branch.f_branch));

            QtCassandra::QCassandraColumnRangePredicate column_predicate;
            column_predicate.setCount(10000); // 4 bytes per entry + row name of under 100 bytes, that's 1Mb max.
            column_predicate.setIndex(); // behave like an index
            column_predicate.setStartColumnName(last_revision_key); // no language (fully neutral) is a valid entry
            column_predicate.setEndColumnName(last_revision_key + "|"); // languages are limited to letters
            QtCassandra::QCassandraRow::pointer_t revision_row(f_content_table->row(source_key));
            for(;;)
            {
                revision_row->clearCache();
                revision_row->readCells(column_predicate);
                QtCassandra::QCassandraCells const new_cells(revision_row->cells());
                if(new_cells.isEmpty())
                {
                    break;
                }
                // handle one batch
                for(QtCassandra::QCassandraCells::const_iterator nc(new_cells.begin());
                        nc != new_cells.end();
                        ++nc)
                {
                    // verify the entry is valid
                    QtCassandra::QCassandraCell::pointer_t last_revision_cell(*nc);
                    if(!last_revision_cell->value().nullValue())
                    {
                        // the revision number is the cell value
                        // we need the row key to extract the language
                        QString column_name(last_revision_cell->columnName());
                        QString locale;
                        if(last_revision_key != column_name)
                        {
                            int const pos(column_name.lastIndexOf(":"));
                            if(pos < 0)
                            {
                                throw snap_logic_exception(QString("somehow the revision column_name \"%1\" does not include at least one ':'.")
                                            .arg(column_name));
                            }
                            // extract the locale (language name)
                            locale = column_name.mid(pos + 1);
                        }
                        snap_version::version_number_t last_revision(last_revision_cell->value().uint32Value());

                        // all the revision names end with:
                        //    '#' <locale> '/' <branch> '.' <revision>
                        //
                        // some revisions may not exist (partial copy)
                        for(snap_version::version_number_t revision(0); revision <= last_revision; ++revision)
                        {
                            QString const source_uri(f_content_plugin->generate_revision_key(source_key, cloned_branch.f_branch, revision, locale));
                            QString const destination_uri(f_content_plugin->generate_revision_key(destination_key, cloned_branch.f_branch, revision, locale));

                            if(f_revision_table->exists(source_uri)
                            && f_revision_table->row(source_uri)->exists(get_name(SNAP_NAME_CONTENT_CREATED)))
                            {
                                dbutils::copy_row(f_revision_table, source_uri, f_revision_table, destination_uri);

                                cloned_branch.f_revisions.push_back(revision);
                            }
                        }
                    }
                }
            }
        }

        bool result() const
        {
            return f_result;
        }

    private:
        content *                                   f_content_plugin;
        clone_info_t                                f_source;
        clone_info_t                                f_destination;
        int64_t const                               f_start_date;
        QtCassandra::QCassandraTable::pointer_t     f_content_table;
        QtCassandra::QCassandraTable::pointer_t     f_branch_table;
        QtCassandra::QCassandraTable::pointer_t     f_revision_table;
        cloned_tree_t                               f_clones;
        controlled_vars::tbool_t                    f_result;
    };

    sub_function f(this, source, destination, f_snap->get_start_date());
    f.clone_tree();
    return f.result();
}


/** \brief Move a page from one URI to another.
 *
 * This function moves the source page to the destination page. The source
 * is then marked as deleted.
 *
 * At this point the destination page must not exist yet.
 *
 * \note
 * Since the page does not get deleted, we do not make a copy in the
 * trashcan even though the source page ends up being marked as deleted.
 *
 * \param[in] ipath_source  The path to the source page being moved.
 * \param[in] ipath_destination  The path to the destination page.
 *
 * \return true if the move succeeds.
 *
 * \sa clone_page()
 * \sa trash_page()
 */
bool content::move_page(path_info_t& ipath_source, path_info_t& ipath_destination)
{
    // is the page deletable? (and thus movable?)
    //
    // (administrative pages, those created from content.xml, are nearly
    // all marked as not deletable by default!)
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue prevent_delete(content_table->row(ipath_source.get_key())->cell(get_name(SNAP_NAME_CONTENT_PREVENT_DELETE))->value());
    if(!prevent_delete.nullValue() && prevent_delete.signedCharValue())
    {
        f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Forbidden Move",
                "Sorry. this page is marked as undeletable and as such it cannot be moved.",
                QString("User tried to move page \"%1\", which is locked (marked as undeletable).").arg(ipath_source.get_key()));
        NOTREACHED();
    }

    // setup the clone parameters
    clone_info_t source;
    source.f_ipath = ipath_source;
    source.f_processing_state.set_state(path_info_t::status_t::state_t::NORMAL);
    source.f_processing_state.set_working(path_info_t::status_t::working_t::CLONING);
    source.f_done_state.set_state(path_info_t::status_t::state_t::DELETED);

    clone_info_t destination;
    destination.f_ipath = ipath_destination;
    destination.f_processing_state.set_state(path_info_t::status_t::state_t::CREATE);
    destination.f_processing_state.set_working(path_info_t::status_t::working_t::CREATING);
    destination.f_done_state = ipath_source.get_status();

    return clone_page(source, destination);
}


/** \brief Put the specified page in the trashcan.
 *
 * This function "deletes" a page by making a copy of it in the trashcan.
 *
 * The original page remains as DELETED for a while. After that while it
 * gets 100% deleted from Cassandra.
 *
 * The pages in the trashcan can be restored at a later time. The time
 * pages are kept in the trashcan is controlled by the website
 * administrator. It can be very short (1 day) or very long (forever).
 *
 * \param[in] ipath  The path to move to the trash.
 *
 * \return true if the cloning worked as expected.
 *
 * \sa clone_page()
 * \sa move_page()
 */
bool content::trash_page(path_info_t& ipath)
{
    // is the page deletable?
    //
    // (administrative pages, those created from content.xml, are nearly
    // all marked as not deletable by default!)
    QtCassandra::QCassandraTable::pointer_t content_table(get_content_table());
    QtCassandra::QCassandraValue prevent_delete(content_table->row(ipath.get_key())->cell(get_name(SNAP_NAME_CONTENT_PREVENT_DELETE))->value());
    if(!prevent_delete.nullValue() && prevent_delete.signedCharValue())
    {
        f_snap->die(snap_child::HTTP_CODE_FORBIDDEN, "Forbidden Removal",
                "Sorry. This page is marked as undeletable.",
                QString("User tried to delete page \"%1\", which is locked.").arg(ipath.get_key()));
        NOTREACHED();
    }

    // create a destination path in the trashcan
    QString trashcan_path("trashcan");

    // path can be changed by administrator
    QtCassandra::QCassandraValue trashcan_path_value(f_snap->get_site_parameter(get_name(SNAP_NAME_CONTENT_TRASHCAN)));
    if(!trashcan_path_value.nullValue())
    {
        // administrators can move the trashcan around up until something
        // gets deleted
        trashcan_path = trashcan_path_value.stringValue();
    }

    // make sure that path exists
    if(!content_table->exists(trashcan_path))
    {
        path_info_t trashcan_ipath;

        trashcan_ipath.set_path(trashcan_path);
        trashcan_ipath.force_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
        trashcan_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));

        // TODO: would we have a language attached to the trashcan?
        //       (certainly because the title should change depending on
        //       the language, right?)
        trashcan_ipath.force_locale("xx");

        // TODO: the owner is the first person who deletes something on the
        //       website; that's probably wrong!
        create_content(trashcan_ipath, get_name(SNAP_NAME_CONTENT_PRIMARY_OWNER), "system-page");

        // save the creation date, title, and description
        QtCassandra::QCassandraTable::pointer_t revision_table(get_revision_table());
        QtCassandra::QCassandraRow::pointer_t revision_row(revision_table->row(trashcan_ipath.get_revision_key()));
        int64_t const start_date(f_snap->get_start_date());
        revision_row->cell(get_name(SNAP_NAME_CONTENT_CREATED))->setValue(start_date);
        // TODO: add support for translation
        QString const title("Trashcan");
        revision_row->cell(get_name(SNAP_NAME_CONTENT_TITLE))->setValue(title);
        QString const empty_string;
        revision_row->cell(get_name(SNAP_NAME_CONTENT_BODY))->setValue(empty_string);
    }

    // new page goes under a randomly generated number
    trashcan_path += "/";
    trashcan_path += f_snap->get_unique_number();

    // setup the clone parameters
    clone_info_t source;
    source.f_ipath = ipath;
    source.f_processing_state.set_state(path_info_t::status_t::state_t::NORMAL);
    source.f_processing_state.set_working(path_info_t::status_t::working_t::REMOVING);
    source.f_done_state.set_state(path_info_t::status_t::state_t::DELETED);

    clone_info_t destination;
    destination.f_ipath.set_path(trashcan_path);
    destination.f_ipath.force_branch(snap_version::SPECIAL_VERSION_SYSTEM_BRANCH);
    destination.f_ipath.force_revision(static_cast<snap_version::basic_version_number_t>(snap_version::SPECIAL_VERSION_FIRST_REVISION));
    destination.f_ipath.force_locale("xx"); // TBD: should the language be set as... maybe the page being deleted?
    destination.f_processing_state.set_state(path_info_t::status_t::state_t::CREATE);
    destination.f_processing_state.set_working(path_info_t::status_t::working_t::CREATING);
    destination.f_done_state.set_state(path_info_t::status_t::state_t::HIDDEN);

    return clone_page(source, destination);
}



SNAP_PLUGIN_END()

// vim: ts=4 sw=4 et

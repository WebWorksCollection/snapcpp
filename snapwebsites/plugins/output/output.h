// Snap Websites Server -- handle the basic display of the website content
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
#pragma once

#include "../layout/layout.h"
#include "../path/path.h"
#include "../javascript/javascript.h"

#include <controlled_vars/controlled_vars_ptr_no_init.h>

namespace snap
{
namespace output
{


//enum name_t
//{
//    SNAP_NAME_OUTPUT_ACCEPTED
//};
//char const *get_name(name_t name) __attribute__ ((const));


class output_exception : public snap_exception
{
public:
    output_exception(char const *       what_msg) : snap_exception("output", what_msg) {}
    output_exception(std::string const& what_msg) : snap_exception("output", what_msg) {}
    output_exception(QString const&     what_msg) : snap_exception("output", what_msg) {}
};

class output_exception_invalid_content_xml : public output_exception
{
public:
    output_exception_invalid_content_xml(char const *       what_msg) : output_exception(what_msg) {}
    output_exception_invalid_content_xml(std::string const& what_msg) : output_exception(what_msg) {}
    output_exception_invalid_content_xml(QString const&     what_msg) : output_exception(what_msg) {}
};







class output : public plugins::plugin, public path::path_execute, public layout::layout_content, public javascript::javascript_dynamic_plugin
{
public:
                        output();
                        ~output();

    static output *     instance();
    virtual QString     description() const;
    virtual int64_t     do_update(int64_t last_updated);

    void                on_bootstrap(snap_child *snap);
    virtual bool        on_path_execute(content::path_info_t& ipath);
    virtual void        on_generate_main_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);
    void                on_generate_page_content(content::path_info_t& ipath, QDomElement& page, QDomElement& body, QString const& ctemplate);

    static QString      phone_to_uri(QString const phone);

    // dynamic javascript property support
    virtual int         js_property_count() const;
    virtual QVariant    js_property_get(QString const& name) const;
    virtual QString     js_property_name(int index) const;
    virtual QVariant    js_property_get(int index) const;

private:
    void                content_update(int64_t variables_timestamp);

    typedef std::map<QString, bool> compression_extensions_map_t;

    zpsnap_child_t                  f_snap;
    compression_extensions_map_t    f_compression_extensions;
};


} // namespace output
} // namespace snap
// vim: ts=4 sw=4 et

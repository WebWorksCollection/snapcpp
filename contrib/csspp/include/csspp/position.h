#ifndef CSSPP_POSITION_H
#define CSSPP_POSITION_H
// CSS Preprocessor
// Copyright (C) 2015-2017  Made to Order Software Corp.
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

#include "csspp/csspp.h"

#include <memory>

namespace csspp
{

typedef int         line_t;

class position
{
public:
    typedef std::shared_ptr<position>   pointer_t;

                        position(std::string const & filename, line_t page = 1, line_t line = 1);
    position &          operator = (position const & rhs);

    void                next_line();
    void                next_page();

    std::string const & get_filename() const;
    line_t              get_page() const;
    line_t              get_line() const;
    line_t              get_total_line() const;

private:
    std::string         f_filename;
    line_t              f_page = 1;
    line_t              f_line = 1;
    line_t              f_total_line = 1;
};

} // namespace csspp
#endif
// #ifndef CSSPP_POSITION_H

// Local Variables:
// indent-tabs-mode: nil
// tab-width: 4
// End:

// vim: ts=4 sw=4 et

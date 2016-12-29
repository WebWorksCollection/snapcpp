#pragma once

#include <stdexcept>
#include <string>
#include <QString>
#include <QStringList>


namespace libexcept
{

class exception_base_t
{
public:
    static int const            STACK_TRACE_DEPTH = 20;

                                exception_base_t();
    virtual                     ~exception_base_t() {}

    QStringList const &         get_stack_trace() const { return f_stack_trace; }

private:
    QStringList                 f_stack_trace;

    void                        collect_stack_trace(int stack_track_depth = STACK_TRACE_DEPTH);
};


class exception_t : public std::runtime_error, public exception_base_t
{
public:
    exception_t( const QString&     what );
    exception_t( const std::string& what );
    exception_t( const char*        what );

    virtual const char* what() const throw() override;
};


}
// namespace libexcept

// vim: ts=4 sw=4 et
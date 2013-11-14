# - Try to find QtCassandra
#
# Once done this will define
#
# QTCASSANDRA_FOUND        - System has QtCassandra
# QTCASSANDRA_INCLUDE_DIR  - The QtCassandra include directories
# QTCASSANDRA_LIBRARY      - The libraries needed to use QtCassandra (none)
# QTCASSANDRA_DEFINITIONS  - Compiler switches required for using QtCassandra (none)

get_property( 3RDPARTY_INCLUDED GLOBAL PROPERTY 3RDPARTY_INCLUDED )
if( 3RDPARTY_INCLUDED )
	set( QTCASSANDRA_INCLUDE_DIRS
			${libQtCassandra_SOURCE_DIR}/include
			${libQtCassandra_BINARY_DIR}/include
			)
	set( QTCASSANDRA_LIBRARIES QtCassandra )
else()
	find_path( QTCASSANDRA_INCLUDE_DIR QtCassandra/QCassandra.h
			   PATHS $ENV{QTCASSANDRA_INCLUDE_DIR}
			   PATH_SUFFIXES QtCassandra
			 )
	find_library( QTCASSANDRA_LIBRARY QtCassandra
				PATHS $ENV{QTCASSANDRA_LIBRARY}
			 )
	set( QTCASSANDRA_INCLUDE_DIRS ${QTCASSANDRA_INCLUDE_DIR} )
	set( QTCASSANDRA_LIBRARIES    ${QTCASSANDRA_LIBRARY}     )
	mark_as_advanced( QTCASSANDRA_INCLUDE_DIR QTCASSANDRA_LIBRARY )
endif()

include( FindPackageHandleStandardArgs )
# handle the QUIETLY and REQUIRED arguments and set QTCASSANDRA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args( QtCassandra DEFAULT_MSG QTCASSANDRA_INCLUDE_DIR QTCASSANDRA_LIBRARY )

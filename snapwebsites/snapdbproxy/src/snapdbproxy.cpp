/*
 * Text:
 *      snapwebsites/snapdbproxy/snapdbproxy.cpp
 *
 * Description:
 *      Proxy database access for two main reasons:
 *
 *      1. keep connections between this computer and the database
 *         computer open (i.e. opening remote TCP connections taken
 *         "much" longer than opening local connections.)
 *
 *      2. remove threads being forced on us by the C/C++ driver from
 *         cassandra (this causes problems with the snapserver that
 *         uses fork() to create the snap_child processes.)
 *
 * License:
 *      Copyright (c) 2016 Made to Order Software Corp.
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

// ourselves
//
#include "snapdbproxy.h"

// our lib
//
#include "log.h"
#include "qstring_stream.h"
#include "dbutils.h"

// 3rd party libs
//
#include <QtCore>
#include <QtCassandra/QCassandra.h>
#include <advgetopt/advgetopt.h>

// system (C++)
//
#include <algorithm>
#include <iostream>
#include <sstream>

namespace
{
    const std::vector<std::string> g_configuration_files; // Empty

    const advgetopt::getopt::option g_snapdbproxy_options[] =
    {
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "Usage: %p [-<opt>]",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            nullptr,
            nullptr,
            "where -<opt> is one or more of:",
            advgetopt::getopt::argument_mode_t::help_argument
        },
        {
            'c',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "config",
            "/etc/snapwebsites/snapdbproxy.conf",
            "Configuration file to initialize snapdbproxy.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "connect",
            nullptr,
            "Define the address and port of the snapcommunicator service (i.e. 127.0.0.1:4040).",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "debug",
            nullptr,
            "Start the snapdbproxy in debug mode.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "help",
            nullptr,
            "show this help output",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            'l',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "logfile",
            nullptr,
            "Full path to the snapdbproxy logfile.",
            advgetopt::getopt::argument_mode_t::optional_argument
        },
        {
            'n',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "nolog",
            nullptr,
            "Only output to the console, not a log file.",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "server-name",
            nullptr,
            "Define the name of the server this service is running on.",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE,
            "snapdbproxy",
            nullptr,
            "The address and port information to listen on (defined in /etc/snapwebsites/snapinit.xml).",
            advgetopt::getopt::argument_mode_t::required_argument
        },
        {
            '\0',
            advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
            "version",
            nullptr,
            "show the version of the snapdb executable",
            advgetopt::getopt::argument_mode_t::no_argument
        },
        {
            '\0',
            0,
            nullptr,
            nullptr,
            nullptr,
            advgetopt::getopt::argument_mode_t::end_of_options
        }
    };


}
//namespace


void snapdbproxy_timer::process_timeout()
{
    f_snapdbproxy->process_timeout();
}




/** \class snapdbproxy
 * \brief Class handling the proxying of the database requests and answers.
 *
 * This class is used to proxy messages from our other parts and send
 * these messages to the Cassandra cluster. Once we get an answer from
 * Cassandra, we then send the results back to the client.
 *
 * The application makes use of threads the process each incoming
 * message and send replies. That way multiple clients can all be
 * services "simultaneously."
 */


/** \brief The instance of the snapdbproxy.
 *
 * This is the instance of the snapdbproxy. The variable where the pointer
 * is kept.
 */
snapdbproxy::pointer_t                    snapdbproxy::g_instance;


/** \brief Initializes a snapdbproxy object.
 *
 * This function parses the command line arguments, reads configuration
 * files, setup the logger.
 *
 * It also immediately executes a --help or a --version command line
 * option and exits the process if these are present.
 *
 * \param[in] argc  The number of arguments in the argv array.
 * \param[in] argv  The array of argument strings.
 */
snapdbproxy::snapdbproxy(int argc, char * argv[])
    : f_opt( argc, argv, g_snapdbproxy_options, g_configuration_files, nullptr )
    , f_session( QtCassandra::QCassandraSession::create() )
{
    // --help
    if( f_opt.is_defined( "help" ) )
    {
        usage(advgetopt::getopt::status_t::no_error);
        snap::NOTREACHED();
    }

    // --version
    if(f_opt.is_defined("version"))
    {
        std::cerr << SNAPWEBSITES_VERSION_STRING << std::endl;
        exit(1);
        snap::NOTREACHED();
    }

    // read the configuration file
    //
    f_config.read_config_file( f_opt.get_string("config").c_str() );

    // --debug
    f_debug = f_opt.is_defined("debug");

    // --server-name (mandatory)
    f_server_name = f_opt.get_string("server-name").c_str();

    // --connect (mandatory)
    tcp_client_server::get_addr_port(f_opt.get_string("connect").c_str(), f_communicator_addr, f_communicator_port, "tcp");

    // --snapdbproxy (mandatory)
    tcp_client_server::get_addr_port(f_opt.get_string("snapdbproxy").c_str(), f_snapdbproxy_addr, f_snapdbproxy_port, "tcp");

    // setup the logger: --nolog, --logfile, or config file log_config
    //
    if(f_opt.is_defined("nolog"))
    {
        snap::logging::configure_console();
    }
    else if(f_opt.is_defined("logfile"))
    {
        snap::logging::configure_logfile( QString::fromUtf8(f_opt.get_string("logfile").c_str()) );
    }
    else
    {
        if(f_config.contains("log_config"))
        {
            // use .conf definition when available
            f_log_conf = f_config["log_config"];
        }
        snap::logging::configure_conffile(f_log_conf);
    }

    if( f_debug )
    {
        // Force the logger level to DEBUG
        // (unless already lower)
        //
        snap::logging::reduce_log_output_level(snap::logging::log_level_t::LOG_LEVEL_DEBUG);
    }

    // from config file only
    if(f_config.contains("cassandra_host_list"))
    {
        f_cassandra_host_list = f_config[ "cassandra_host_list" ];
        if(f_cassandra_host_list.isEmpty())
        {
            throw snap::snapwebsites_exception_invalid_parameters("cassandra_host_list cannot be empty.");
        }
    }
    if(f_config.contains("cassandra_port"))
    {
        bool ok(false);
        f_cassandra_port = f_config["cassandra_port"].toInt(&ok);
        if(!ok
        || f_cassandra_port < 0
        || f_cassandra_port > 65535)
        {
            throw snap::snapwebsites_exception_invalid_parameters("cassandra_port to connect to Cassandra must be defined between 0 and 65535.");
        }
    }

    // offer the user to setup the maximum number of pending connections
    // from services that want to connect to Cassandra (this is only
    // the maximum number of "pending" connections and not the total
    // number of acceptable connections)
    //
    if(f_config.contains("max_pending_connections"))
    {
        QString const max_connections(f_config["max_pending_connections"]);
        if(!max_connections.isEmpty())
        {
            bool ok;
            f_max_pending_connections = max_connections.toInt(&ok);
            if(!ok)
            {
                SNAP_LOG_FATAL("invalid max_pending_connections, a valid number was expected instead of \"")(max_connections)("\".");
                exit(1);
            }
            if(f_max_pending_connections < 1)
            {
                SNAP_LOG_FATAL("max_pending_connections must be positive, \"")(max_connections)("\" is not valid.");
                exit(1);
            }
        }
    }

    // make sure there are no standalone parameters
    if( f_opt.is_defined( "--" ) )
    {
        std::cerr << "error: unexpected parameter found on daemon command line." << std::endl;
        usage(advgetopt::getopt::status_t::error);
        snap::NOTREACHED();
    }
}


/** \brief Clean up the snap dbproxy object.
 *
 * This function is used to do some clean up of the snapdbproxy
 * environment.
 */
snapdbproxy::~snapdbproxy()
{
}


/** \brief Print out this server usage and exit.
 *
 * This function calls the advanced option library to have it print
 * out the list of acceptable command line options.
 */
void snapdbproxy::usage(advgetopt::getopt::status_t status)
{
    f_opt.usage( status, "snapdbproxy" );
    exit(1);
}


/** \brief Retrieve the server name.
 *
 * This function returns a copy of the server name. Since the constructor
 * defines the server name, it is available at all time after that.
 *
 * \return The server name.
 */
QString snapdbproxy::server_name() const
{
    return f_server_name;
}


/** \brief Start the Snap! Communicator and wait for events.
 *
 * This function initializes the snapdbproxy object further and then
 * listens for events.
 *
 * This specific daemon listens for two sets of events:
 *
 * \li Events sent via the snapcommunicator system; mainly used to
 *     REGISTER this as a server; tell the snapinit service that we
 *     are running; and accept a STOP to quit the application
 * \li New network connections to process Cassandra CQL commands.
 */
void snapdbproxy::run()
{
    // Stop on these signals, log them, then terminate.
    //
    signal( SIGCHLD, snapdbproxy::sighandler );
    signal( SIGSEGV, snapdbproxy::sighandler );
    signal( SIGBUS,  snapdbproxy::sighandler );
    signal( SIGFPE,  snapdbproxy::sighandler );
    signal( SIGILL,  snapdbproxy::sighandler );
    signal( SIGTERM, snapdbproxy::sighandler );
    signal( SIGINT,  snapdbproxy::sighandler );
    signal( SIGQUIT, snapdbproxy::sighandler );

    // ignore console signals
    //
    signal( SIGTSTP,  SIG_IGN );
    signal( SIGTTIN,  SIG_IGN );
    signal( SIGTTOU,  SIG_IGN );

    // initialize the communicator and its connections
    //
    f_communicator = snap::snap_communicator::instance();

    // create a listener
    //
    // Note that the listener changes its priority to 30 in order to
    // make sure that it gets called first in case multiple events
    // arrive simultaneously.
    //
    f_listener = std::make_shared<snapdbproxy_listener>(this, f_snapdbproxy_addr.toUtf8().data(), f_snapdbproxy_port, f_max_pending_connections, true, false);
    f_communicator->add_connection(f_listener);

    // create a messager to communicate with the Snap Communicator process
    // and snapinit as required
    //
    f_messenger = std::make_shared<snapdbproxy_messenger>(this, f_communicator_addr.toUtf8().data(), f_communicator_port);
    f_communicator->add_connection(f_messenger);

    // create a timer, it will immediately kick in and attempt a connection
    // to Cassandra; if it fails, it will continue to tick until it works.
    //
    f_timer = std::make_shared<snapdbproxy_timer>(this);
    f_communicator->add_connection(f_timer);

    // now run our listening loop
    //
    f_communicator->run();
}


/** \brief A static function to capture various signals.
 *
 * This function captures unwanted signals like SIGSEGV and SIGILL.
 *
 * The handler logs the information and then the service exists.
 * This is done mainly so we have a chance to debug problems even
 * when it crashes on a remote server.
 *
 * \warning
 * The signals are setup after the construction of the snapdbproxy
 * object because that is where we initialize the logger.
 *
 * \param[in] sig  The signal that was just emitted by the OS.
 */
void snapdbproxy::sighandler( int sig )
{
    QString signame;
    bool show_stack_output(true);
    switch( sig )
    {
    case SIGSEGV:
        signame = "SIGSEGV";
        break;

    case SIGBUS:
        signame = "SIGBUS";
        break;

    case SIGFPE:
        signame = "SIGFPE";
        break;

    case SIGILL:
        signame = "SIGILL";
        break;

    case SIGTERM:
        signame = "SIGTERM";
        show_stack_output = false;
        break;

    case SIGINT:
        signame = "SIGINT";
        show_stack_output = false;
        break;

    case SIGQUIT:
        signame = "SIGQUIT";
        show_stack_output = false;
        break;

    default:
        signame = "UNKNOWN";
        break;

    }

    if(show_stack_output)
    {
        snap::snap_exception_base::output_stack_trace();
    }
    SNAP_LOG_FATAL("Fatal signal caught: ")(signame);

    // Exit with error status
    //
    ::exit( 1 );
    snap::NOTREACHED();
}


/** \brief Process a message received from Snap! Communicator.
 *
 * This function gets called whenever the Snap! Communicator sends
 * us a message. This includes the READY and HELP commands, although
 * the most important one is certainly the STOP command.
 *
 * \param[in] message  The message we just received.
 */
void snapdbproxy::process_message(snap::snap_communicator_message const & message)
{
    SNAP_LOG_TRACE("received messager message [")(message.to_message())("] for ")(f_server_name);

    QString const command(message.get_command());

// TODO: use a switch statement

    if(command == "CASSANDRASTATUS")
    {
        snap::snap_communicator_message reply;
        reply.reply_to(message);
        reply.set_command(f_session->isConnected() ? "CASSANDRAREADY" : "NOCASSANDRA");
        f_messenger->send_message(reply);
        return;
    }

    if(command == "LOG")
    {
        // logrotate just rotated the logs, we have to reconfigure
        //
        SNAP_LOG_INFO("Logging reconfiguration.");
        snap::logging::reconfigure();
        return;
    }

    if(command == "STOP")
    {
        // Someone is asking us to leave (probably snapinit)
        //
        stop(false);
        return;
    }
    if(command == "QUITTING")
    {
        // If we received the QUITTING command, then somehow we sent
        // a message to Snap! Communicator, which is already in the
        // process of quitting... we should get a STOP too, but we
        // can just quit ASAP too
        //
        stop(true);
        return;
    }

    if(command == "READY")
    {
        f_ready = true;

        // Snap! Communicator received our REGISTER command
        //
        if(f_session->isConnected())
        {
            cassandra_ready();
        }
        return;
    }

    if(command == "HELP")
    {
        // Snap! Communicator is asking us about the commands that we support
        //
        snap::snap_communicator_message reply;
        reply.set_command("COMMANDS");

        // list of commands understood by service
        //
        reply.add_parameter("list", "CASSANDRASTATUS,HELP,LOG,QUITTING,READY,STOP,UNKNOWN");

        f_messenger->send_message(reply);
        return;
    }

    if(command == "UNKNOWN")
    {
        // we sent a command that Snap! Communicator did not understand
        //
        SNAP_LOG_ERROR("we sent unknown command \"")(message.get_parameter("command"))("\" and probably did not get the expected result.");
        return;
    }

    // unknown command is reported and process goes on
    //
    SNAP_LOG_ERROR("unsupported command \"")(command)("\" was received on the connection with Snap! Communicator.");
    {
        snap::snap_communicator_message reply;
        reply.set_command("UNKNOWN");
        reply.add_parameter("command", command);
        f_messenger->send_message(reply);
    }
}


/** \brief Call whenever a new connection was received.
 *
 * This function adds a new connection to the snapdbproxy daemon. A
 * connection is a blocking socket handled by a thread.
 *
 * \param[in] s  The socket the connection threads becomes the owner of.
 */
void snapdbproxy::process_connection(int const s)
{
    // only the main process calls this function so we can take the time
    // to check the f_connections vector and remove dead threads
    //
    {
        size_t idx(f_connections.size());
        while(idx > 0)
        {
            --idx;

            if(!f_connections[idx]->is_running())
            {
                // thread exited, remove from vector so that the
                // vector does not grow forever
                //
                f_connections.erase(f_connections.begin() + idx);
            }
        }
    }

    if(!f_session->isConnected())
    {
        no_cassandra();
    }

    // create one thread per connection
    //
    // TODO: look into having either worker threads, or at least a pool
    //       that we need around
    //
    // The snapdbproxy_thread constructor is expected to start the thread
    // although it may fail; if it does fail, we avoid adding the thread
    // to the f_connections vector; that way the socket gets closed
    // (the only case where the socket does not get closed is and
    // std::bad_alloc exception which we do not capture here.)
    //
    snapdbproxy_thread::pointer_t thread(std::make_shared<snapdbproxy_thread>(f_session, s, f_cassandra_host_list, f_cassandra_port));
    if(thread && thread->is_running())
    {
        f_connections.push_back(thread);
    }
}


/** \brief Attempt to connect to the Cassandra cluster.
 *
 * This function calls connect() in order to create a network connection
 * between this computer and a Cassandra node. Later the driver may
 * connect to additional nodes to better balance work load.
 *
 * \note
 * Since attempts to connect to Cassandra are blocking, we probably want
 * to move this timer processing to a thread instead.
 */
void snapdbproxy::process_timeout()
{
    try
    {
        // connect to Cassandra
        //
        // The Cassandra C/C++ driver is responsible to actually create
        // "physical" connections to any number of nodes so we do not
        // need to monitor those connections.
        //
        f_session->connect( f_cassandra_host_list, f_cassandra_port ); // throws on failure!

        // the connection succeeded, turn off the timer we do not need
        // it for now...
        //
        f_timer->set_enable(false);

        // reset that flag!
        //
        f_no_cassandra_sent = false;

        cassandra_ready();
    }
    catch(std::runtime_error const &)
    {
        // the connection failed, keep the timeout enabled and try again
        // on the next tick
        //
        // TODO: increase the timeout delay so we do not swamp the
        //       network with useless attempts

        no_cassandra();
    }
}


/** \brief Send a NOCASSANDRA message.
 *
 * Let snapcommunicator and other services know that we do not
 * have a connection to Cassandra. Computers running snap.cgi should
 * react by not connecting to this computer since snapserver will not
 * work in that case.
 *
 * Obviously, if we cannot find a Cassandra node, we probably
 * have another bigger problem and snapcommunicator is probably
 * not connected to anyone else either...
 */
void snapdbproxy::no_cassandra()
{
    if(!f_no_cassandra_sent)
    {
        f_no_cassandra_sent = true;
        snap::snap_communicator_message cmd;
        cmd.set_command("NOCASSANDRA");
        cmd.set_service(".");
        f_messenger->send_message(cmd);
    }

    // make sure the timer is on when we do not have a Cassandra connection
    //
    f_timer->set_enable(true);
}


void snapdbproxy::cassandra_ready()
{
    if(f_ready)
    {
        // let other services know when cassandra is (finally) ready
        //
        snap::snap_communicator_message cmd;
        cmd.set_command("CASSANDRAREADY");
        cmd.set_service(".");
        f_messenger->send_message(cmd);
    }
}


/** \brief Called whenever we receive the STOP command or equivalent.
 *
 * This function makes sure the snaplock exits as quickly as
 * possible.
 *
 * \li Marks the messager as done.
 * \li UNREGISTER from snapcommunicator.
 * \li Remove the listener.
 *
 * \note
 * If the g_messager is still in place, then just sending the
 * UNREGISTER is enough to quit normally. The socket of the
 * g_messager will be closed by the snapcommunicator server
 * and we will get a HUP signal. However, we get the HUP only
 * because we first mark the messager as done.
 *
 * \param[in] quitting  Set to true if we received a QUITTING message.
 */
void snapdbproxy::stop(bool quitting)
{
    SNAP_LOG_INFO("Stopping server.");

    if(f_messenger)
    {
        f_messenger->mark_done();

        // unregister if we are still connected to the messager
        // and Snap! Communicator is not already quitting
        //
        if(!quitting)
        {
            snap::snap_communicator_message cmd;
            cmd.set_command("UNREGISTER");
            cmd.add_parameter("service", "snapdbproxy");
            f_messenger->send_message(cmd);
        }
    }

    // also remove the listener, we will not accept any more
    // database commands...
    //
    if(f_communicator)
    {
        f_communicator->remove_connection(f_listener);
        f_listener.reset(); // TBD: in snapserver I do not reset these pointers...
    }
}


// vim: ts=4 sw=4 et

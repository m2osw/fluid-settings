// Copyright (c) 2011-2022  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/project/fluid-settings
// contact@m2osw.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

/** \file
 * \brief The implementation of the fluid-settings client.
 *
 * This file is the implementation of the fluid_settings_connection class
 * giving you easy access the fluid-settings daemon.
 *
 * This gives your tools and services access to the fluid-settings values.
 */

// self
//
#include    "fluid-settings/fluid_settings_connection.h"

#include    "fluid-settings/exception.h"


// snaplogger
//
#include    <snaplogger/message.h>


// last include
//
#include    <snapdev/poison.h>



namespace fluid_settings
{



namespace
{



/** \brief Options to handle the fluid settings connection.
 *
 * The fluid settings client connects to the communicator daemon and
 * then sends and receives messages through your messenger dispatcher.
 *
 * The dispatcher understood commands is grown by calling the
 *
 * \code
 *     add_fluid_settings_commands()
 * \endcode
 *
 * and the 
 *
 * \code
 *     add_communicator_commands()
 * \endcode
 *
 * Anything that can be handled internally is done transparently. You can
 * also implement various virtual functions that get called on various
 * events. To avoid you having to call base class functions, these are
 * new functions defined by the fluid-settings environment.
 */
advgetopt::option const g_options[] =
{
    // FLUID SETTINGS OPTIONS
    //
    advgetopt::define_option(
          advgetopt::Name("fluid-settings-timeout")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_COMMAND_LINE
            , advgetopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE
            , advgetopt::GETOPT_FLAG_CONFIGURATION_FILE
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::EnvironmentVariableName("FLUID_SETTINGS_TIMEOUT")
        , advgetopt::DefaultValue("10s")
        , advgetopt::Validator("duration")
        , advgetopt::Help("define the communicator daemon connection type as a scheme (cd://, cdu://, cds://, cdb://) along an \"address:port\" or \"/socket/path\".")
    ),

    // END
    //
    advgetopt::end_options()
};







class fluid_settings_timer
    : public ed::timer
{
public:
    typedef std::shared_ptr<fluid_settings_timer>      pointer_t;

    fluid_settings_timer(fluid_settings_connection * fs, std::int64_t timeout_us)
        : timer(timeout_us)
        , f_fluid_settings(fs)
    {
    }

    virtual ~fluid_settings_timer() override
    {
    }

    void process_timeout()
    {
        f_fluid_settings->msg_fluid_timeout();
    }

private:
    fluid_settings_timer(fluid_settings_timer const &) = delete;
    fluid_settings_timer & operator = (fluid_settings_timer const &) = delete;

    fluid_settings_connection *        f_fluid_settings = nullptr;
};


}
// no name namespace




/** \brief Initializes the fluid_settings_connection sub-class.
 *
 * This function makes sure the fluid-settings are going to work. However,
 * other functions need to be called in the right order to complete the
 * initialization process.
 *
 * By default, the fluid_settings_connection listens to your advgetopt options that
 * were marked with this flag:
 *
 * \code
 * advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION
 * \endcode
 *
 * Other options are left alone. If you want to listen to options from
 * another service (i.e. get the firewall daemon IP and port information)
 * then you will want to call the add_watch() with the correct name
 * (maybe "firewall::listen"). It is not currently possible to define such
 * a name in the list of advgetopt::option_info objects.
 *
 * Whenever something changes, you get your implementation of the
 * fluid_settings_changed() function called. You may also get a few
 * other functions called in case a different type of event occurs:
 *
 * \li fluid_settings_changed()
 *
 * A value changed or was received for the first time.
 *
 * \li fluid_failed()
 *
 * An error occurs in the fluid-settings daemon; the event dispatcher
 * message includes a parameter called "message" with the error spelled out.
 *
 * \li fluid_settings_options()
 *
 * When you request a complete list of all the available settings, this
 * function gets called with that list. These are all the possible parameter
 * names as found in the fluid-settings .ini definition files. To get the
 * values, you have to use the get_settings_value() or the add_watch().
 *
 * \li service_status() -- whenever the STATUS message is sent to us, this
 * function is called in your messenger; it tells you about a service which
 * status just changed--i.e. the service was now up or down; this
 * fluid-settings client implementation sends a message to the fluid-settings
 * daemon so you will get this called at least once; this is at the time
 * the fluid-settings registers all the parameters you are interested in
 *
 * This class expects that you create at least two objects in your daemon:
 *
 * * a \em messenger capturing messages from the communicator daemon, and
 * * a \em server managing the whole thing (i.e. other connections, data,
 *   what to reply with, etc.)
 *
 * Your messenger constructor would look something like this:
 *
 * \code
 *     messenger::messenger(server * parent, advgetopt::getopt const & opts)
 *         : fluid_settings_connection(opts, "my_service")
 *         , f_parent(parent)
 *         , f_dispatcher(std::make_shared<ed::dispatcher<messenger>>(this, g_dispatcher_messages))
 *     {
 *         set_dispatcher(f_dispatcher);
 *         add_fluid_settings_commands();
 *         f_dispatcher->add_communicator_commands();
 *     #ifdef _DEBUG
 *         f_dispatcher->set_trace();
 *     #endif
 *     }
 * \endcode
 *
 * \warning
 * The add_fluid_settings_commands() function must be called before the
 * dispatcher add_communicator_commands() which adds a catch all match at
 * the end of the list preventing futher command from being added.
 *
 * And your server, to ensure everything works, initializes the messenger
 * in this way:
 *
 * \code
 *     server::server()
 *         : f_opts(g_options_environment) // partial initialization
 *         , f_messenger(std::make_shared<messenger>(this, f_opts, "<service-name>")
 *     {
 *         snaplogger::add_logger_options(f_opts);
 *         f_opts.finish_parsing(argc, argv);
 *         if(!snaplogger::process_logger_options(
 *                   f_opts
 *                 , "/etc/<service-name>/logger"
 *                 , std::cout
 *                 , false))
 *         {
 *             // exit on any error
 *             throw advgetopt::getopt_exit("logger options generated an error.", 1);
 *         }
 *         f_messenger->process_fluid_settings_options();
 *     }
 * \endcode
 *
 * \note
 * The fluid_settings_connection constructor accepts one advgetopt::getopt references.
 * That object includes a vector of advgetopt::option_info objects. Those
 * objects have flags, one of which is:
 * \code
 * advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION
 * \endcode
 * \note
 * The fluid_settings_connection will get that list and listen to those parameters.
 * Note that it is up to you to also mark such parameters as accessible
 * via the command line, the environment variables and configuration
 * files.
 * \note
 * There is no default dynamic configuration options, so you must make
 * sure that you mark those you want to listen on clearly. Note that
 * should correspond to those parameters defined under:
 * \code
 *     /var/share/fluid-settings/definitions/<name>.ini
 * \endcode
 * \note
 * The fluid-settings does not attempt to match the definitions with your
 * options until you try to get/set/listen to such values.
 *
 * \param[in] command_line_opts  The options on your command line.
 * \param[in] fluild_settings_opts  The options fluid-settings updates.
 * \param[in] service_name  Your service name to register with the communicator
 * daemon and receive replies from other services.
 */
fluid_settings_connection::fluid_settings_connection(
          advgetopt::getopt & opts
        , std::string const & service_name)
    : communicator(opts, service_name)
    , f_opts(opts)
{
    f_opts.parse_options_info(g_options, true);

    init_watches();
}


fluid_settings_connection::~fluid_settings_connection()
{
}


void fluid_settings_connection::init_watches()
{
    advgetopt::option_info::map_by_name_t const & options(f_opts.get_options());
    for(auto const & o : options)
    {
        if(o.second->has_flag(advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION))
        {
            add_watch(o.second->get_name());
        }
    }
}


/** \brief Add the Fluid Settings commands to your dispatcher.
 *
 * When you create your messenger, you want to include a dispatcher with
 * messages that your support. This will not include any of the commands
 * handled by the communicator or fluid settings daemons. This very
 * function adds the fluid settings commands. To also include the
 * communicator commands, call the add_communicator_commands() as
 * defined in the low level dispatcher (which we may move at some
 * point because these should probably be inside the communicator
 * class).
 *
 * \warning
 * This function has to be called before the add_communicator_commands().
 */
void fluid_settings_connection::add_fluid_settings_commands()
{
    ed::dispatcher::pointer_t d(get_dispatcher());
    if(d == nullptr)
    {
        throw fluid_settings_implementation_error("your fluid settings messenger is missing its dispatcher");
    }

    // our own have to make use of dynamic std::function() through std::bind()
    // because the dispatcher does not directly link to use or something that
    // could be used for the purpose
    //
    d->add_matches({
        DISPATCHER_MATCH("FLUID_SETTINGS_DEFAULT_VALUE", &fluid_settings_connection::msg_fluid_default_value),
        DISPATCHER_MATCH("FLUID_SETTINGS_DELETED",       &fluid_settings_connection::msg_fluid_deleted),
        DISPATCHER_MATCH("FLUID_SETTINGS_ERROR",         &fluid_settings_connection::msg_fluid_error),
        DISPATCHER_MATCH("FLUID_SETTINGS_OPTIONS",       &fluid_settings_connection::msg_fluid_options),
        DISPATCHER_MATCH("FLUID_SETTINGS_REGISTERED",    &fluid_settings_connection::msg_fluid_registered),
        DISPATCHER_MATCH("FLUID_SETTINGS_UPDATED",       &fluid_settings_connection::msg_fluid_updated),
        DISPATCHER_MATCH("FLUID_SETTINGS_VALUE",         &fluid_settings_connection::msg_fluid_value),
        DISPATCHER_MATCH("FLUID_SETTINGS_VALUE_UPDATED", &fluid_settings_connection::msg_fluid_value_updated),
        DISPATCHER_MATCH("STATUS",                       &fluid_settings_connection::msg_fluid_status),
    });
}


/** \brief Process command line options understood by the fluid-settings.
 *
 * This function processes the command line options that the fluid-settings
 * and the communicator client support. It first checks the communicator
 * client options using the following:
 *
 * \code
 *     process_communicatord_options();
 * \endcode
 *
 * Then it checks its own options.
 */
void fluid_settings_connection::process_fluid_settings_options()
{
    // first make sure we process the communicator daemon options
    //
    process_communicatord_options();
}


void fluid_settings_connection::unregister_fluid_settings(bool quitting)
{
    communicator::unregister_communicator(quitting);
}


/** \brief Request for the value of a specific setting.
 *
 * This function sends a message to the fluid-settings to request for
 * a specific value. Once we receive the value, the fluid_settings_changed()
 * function gets called and you can capture the value that way.
 *
 * The name of the parameter to retrieve can include a namespace. If the
 * namespace is not specified, we prepend our service name (i.e. we do as
 * if you wanted to retrieve one of your own parameters).
 *
 * \note
 * In other words, this function is asynchronous.
 *
 * \param[in] name  The name of the parameter to retrieve.
 */
void fluid_settings_connection::get_settings_value(std::string const & name)
{
    ed::message msg;
    msg.set_command("FLUID_SETTINGS_GET");
    msg.set_service("fluid_settings");
    msg.add_parameter("name", qualify_name(name));
    msg.add_parameter("cache", "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_all_values(std::string const & name)
{
    ed::message msg;
    msg.set_command("FLUID_SETTINGS_GET");
    msg.set_service("fluid_settings");
    msg.add_parameter("name", qualify_name(name));
    msg.add_parameter("all", "true");
    msg.add_parameter("cache", "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_value_with_priority(std::string const & name, priority_t priority)
{
    ed::message msg;
    msg.set_command("FLUID_SETTINGS_GET");
    msg.set_service("fluid_settings");
    msg.add_parameter("name", qualify_name(name));
    msg.add_parameter("priority", priority);
    msg.add_parameter("cache", "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_default_value(std::string const & name)
{
    ed::message msg;
    msg.set_command("FLUID_SETTINGS_GET");
    msg.set_service("fluid_settings");
    msg.add_parameter("name", qualify_name(name));
    msg.add_parameter("default_value", "true");
    msg.add_parameter("cache", "no;reply");
    send_message(msg);
}


void fluid_settings_connection::add_watch(std::string const & name)
{
    std::string watch(qualify_name(name));

    auto const result(f_watches.insert(watch));
    if(result.second
    && f_registered)
    {
        // new watch, register it
        //
        listen(watch);
    }
}


std::string fluid_settings_connection::qualify_name(std::string const & name)
{
    // already include one or more namespaces?
    //
    std::string::size_type const pos(name.find(':'));
    if(pos == std::string::npos)
    {
        return service_name() + "::" + name;
    }

    return name;
}


/** \brief The fluid-settings service is unavailable.
 *
 * Whenever we send a message, we may receive this message back. If the
 * concerned service is "fluid_settings", then this function generates
 * a call to the fluid_settings_changed() with status set to:
 *
 * \code
 * fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNAVAILABLE
 * \endcode
 *
 * The function also sends a log message about the issue.
 *
 * You may overload this function if you need to know of the availability of
 * another service than the fluid_settings_connection. In that case, make sure to
 * call this version as well:
 *
 * \code
 *     void messenger::msg_service_unavailable(ed::message & msg)
 *     {
 *         fluid_settings_connection::msg_service_unavailable(msg);
 *
 *         ...your own code here...
 *     }
 * \endcode
 *
 * \param[in] msg  The service unavailable message.
 */
void fluid_settings_connection::msg_service_unavailable(ed::message & msg)
{
    communicator::msg_service_unavailable(msg);

    if(!msg.has_parameter("destination_service"))
    {
        return;
    }

    // are we concerned about this message?
    //
    std::string const service(msg.get_parameter("destination_service"));
    if(service != "fluid_settings")
    {
        return;
    }

    // this is recoverable in the sense that the fluid-settings service
    // should auto-restart and be available again _soon_
    //
    SNAP_LOG_RECOVERABLE_ERROR
        << "fluid_settings service is not currently available."
        << SNAP_LOG_SEND;

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNAVAILABLE
        , std::string()
        , std::string());
}


void fluid_settings_connection::service_status(std::string const & service, std::string const & status)
{
    snapdev::NOT_USED(service, status);

    // do nothing, we already handled the message in msg_status()
}


void fluid_settings_connection::fluid_settings_changed(fluid_settings_status_t status, std::string const & name, std::string const & value)
{
    snapdev::NOT_USED(status, name, value);

    // do nothing, we call this function from all the others handling the
    // fluid-settings messages
}


void fluid_settings_connection::fluid_settings_options(advgetopt::string_list_t const & list)
{
    snapdev::NOT_USED(list);
}


/** \brief Handle a default value message.
 *
 * This message is received whenever someone requested the default value
 * of a setting. It will call your fluid_settings_changed() function with the
 * status set to:
 *
 * \code
 * fluid_settings_status_t::FLUID_SETTINGS_STATUS_DEFAULT
 * \endcode
 *
 * \param[in] msg  The fluid-settings message with the default value.
 */
void fluid_settings_connection::msg_fluid_default_value(ed::message & msg)
{
    if(!msg.has_parameter("name")
    || !msg.has_parameter("value"))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \"name\" or a \"value\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_DEFAULT
        , msg.get_parameter("name")
        , msg.get_parameter("value"));
}


void fluid_settings_connection::msg_fluid_deleted(ed::message & msg)
{
    if(!msg.has_parameter("name"))
    {
        SNAP_LOG_ERROR
            << "reply to DELETE command did not include a \"name\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_DELETED
        , msg.get_parameter("name")
        , std::string());
}


void fluid_settings_connection::msg_fluid_error(ed::message & msg)
{
    SNAP_LOG_ERROR
        << "an error occurred in fluid-settings: "
        << msg.to_string()
        << SNAP_LOG_SEND;

    // let the user (of fluid_settings_connection) know
    //
    fluid_failed(msg);
}


void fluid_settings_connection::fluid_failed(ed::message & msg)
{
    snapdev::NOT_USED(msg);
}


void fluid_settings_connection::msg_fluid_options(ed::message & msg)
{
    if(!msg.has_parameter("options"))
    {
        SNAP_LOG_ERROR
            << "reply to FLUID_SETTINGS_LIST command did not include an \"options\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    advgetopt::string_list_t options;
    advgetopt::split_string(msg.get_parameter("options"), options, {","});

    fluid_settings_options(options);
}


void fluid_settings_connection::msg_fluid_registered(ed::message & msg)
{
    if(msg.has_parameter("message"))
    {
        SNAP_LOG_WARNING
            << "registration of this listener generated a warning: \""
            << msg.get_parameter("message")
            << "\"."
            << SNAP_LOG_SEND;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_REGISTERED
        , std::string()
        , std::string());
}


void fluid_settings_connection::msg_fluid_updated(ed::message & msg)
{
    if(!msg.has_parameter("name"))
    {
        SNAP_LOG_ERROR
            << "reply to SET command did not include a \"name\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_UPDATED
        , msg.get_parameter("name")
        , std::string());
}


void fluid_settings_connection::msg_fluid_value(ed::message & msg)
{
    if(!msg.has_parameter("name")
    || !msg.has_parameter("value"))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \"name\" or a \"value\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_VALUE
        , msg.get_parameter("name")
        , msg.get_parameter("value"));
}


void fluid_settings_connection::msg_fluid_value_updated(ed::message & msg)
{
    if(!msg.has_parameter("name"))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \"name\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    if(msg.has_parameter("value"))
    {
        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_NEW_VALUE
            , msg.get_parameter("name")
            , msg.get_parameter("value"));
    }
    else if(msg.has_parameter("error"))
    {
        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNDEFINED
            , msg.get_parameter("name")
            , std::string());
    }
}


void fluid_settings_connection::msg_fluid_status(ed::message & msg)
{
    if(!msg.has_parameter("status")
    || !msg.has_parameter("service"))
    {
        return;
    }

    std::string const service(msg.get_parameter("service"));
    std::string const status(msg.get_parameter("status"));

    if(service == "fluid_settings")
    {
        f_registered = status == "up";
        if(f_registered)
        {
            listen(snapdev::join_strings(f_watches, ","));
        }
    }

    service_status(service, status);
}


void fluid_settings_connection::listen(std::string const & watches)
{
    ed::message msg;
    msg.set_command("FLUID_SETTINGS_LISTEN");
    msg.set_service("fluid_settings");
    msg.add_parameter("names", watches);
    msg.add_parameter("cache", "no;reply");
    send_message(msg);
}



} // fluid_settings namespace
// vim: ts=4 sw=4 et

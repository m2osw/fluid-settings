// Copyright (c) 2011-2024  Made to Order Software Corp.  All Rights Reserved
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
#include    "fluid-settings/names.h"


// eventdispatcher
//
#include    <eventdispatcher/names.h>


// communicatord
//
#include    <communicatord/names.h>


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
            , advgetopt::GETOPT_FLAG_REQUIRED
            , advgetopt::GETOPT_FLAG_SHOW_SYSTEM>())
        , advgetopt::EnvironmentVariableName("FLUID_SETTINGS_TIMEOUT")
        , advgetopt::DefaultValue("10s")
        , advgetopt::Validator("duration")
        , advgetopt::Help("How long it can take before we assume that fluid-settings is not available.")
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
 * This function makes sure the fluid-settings work seemlessly in your
 * messenger client. However, other functions need to be called in the
 * right order to properly complete the initialization process. This
 * includes functions from the communicatord::communicator class which
 * this class derive from.
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
 * message includes two parameters: "message" with the error spelled out
 * (can be displayed to a human) and "command" which names the command
 * in error. The message command itself may either be UNKNOWN, we send
 * a command that the daemon does not know anything about or INVALID,
 * in which case the daemon knows about the command, but it was not
 * used properly (i.e. the parameters in the message sent were not
 * considered valid).
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
 *         f_messenger->automatic_watch_initialization();
 *     }
 * \endcode
 *
 * The call to the automatic_watch_initialization() function allows for the
 * options marked with the dynamic flag to be watched without you having to
 * call the add_watch() directly. Note that function is in most cases
 * expected to be called after the process_fluid_settings_options() function.
 * This way the user can override the fluid-settings options on the command
 * line or their configuration file. At the same time, you may define some
 * parameters as not available on the command line, environment variables,
 * or configuration files so you can limit the overriding capabilities.
 *
 * Note that you can load the fluid-settings definition files to define the
 * options found in your f_opts. This is quite practical since it means you
 * do not have to define those parameters in two places. To do so, you can
 * explicitly call the option parser:
 *
 * \code
 * f_opts.parse_options_from_file(filename, 1, 1);
 * \endcode
 *
 * or properly link your .ini file in the advgetopt share folder:
 *
 * \code
 * /usr/share/advgetopt/options/<filename>.ini
 * \endcode
 *
 * In most cases, you want to create a softlink from one to the other. We
 * propose that you install your .ini files in the fluid installation
 * directory:
 *
 * \code
 * /usr/share/fluid-settings/definitions/<filename>.ini
 * \endcode
 *
 * And then create a softlink in the advgetopt options directory. Then the
 * file will automatically be loaded when the f_opts objects is initialized.
 *
 * \code
 * /usr/share/advgetopt/options/<filename>.ini -> /usr/share/fluid-settings/definitions/<filename>.ini
 * \endcode
 *
 * The dynamic configuration flag is no set automatically. The fluid-settings
 * files do not require it (the files under
 * `/usr/share/fluid-settings/definitions`) for the daemon to function as
 * expected. However, for the automatic loading mechanism to work in clients,
 * you want to make sure that it is specified in the `allowed=...` parameter
 * like so:
 *
 * \code
 * allowed=command-line,environment-variable,configuration-file,dynamic-configuration
 * \endcode
 *
 * In this example, the flags allow parameters to appear on the command line,
 * environment variables, configuration files, and in fluid-settings.
 *
 * It is possible to dynamically add watch lists. This is done by the
 * sitter daemon because the sitter may support plugins which also have
 * parameters that can be handled by the fluid-settings.
 *
 * Also, if you want to listen to parameters defined by other services, then
 * your only way to listen to them is to add them dynamically:
 *
 * \code
 * f_messenger->add_watch("firewall::uri");
 * \endcode
 *
 * In many cases, the dynamic use of the f_opts each time you need the value
 * is the best way to go, especially if you just need a string.
 *
 * \code
 * std::string const param(f_opts.get_string("my-parameter"));
 * ...use param as you see fit...
 *
 * -- or if the value has no default defined in the .ini --
 *
 * std::string param("some default");
 * if(g_opts.is_defined("my-parameter"))
 * {
 *     param = f_opts.get_string("my-parameter");
 * }
 * ...use param as you see fit...
 * \endcode
 *
 * However, if your parameter requires a complex parsing process first,
 * then caching the data once parsed is a good idea. You can clear that
 * cache when the value changes. To listen for changes, implement the
 * fluid_settings_changed() function.
 *
 * \code
 * void messenger::fluid_settings_changed(
 *       fluid_settings::fluid_settings_status_t status
 *     , std::string const & name
 *     , std::string const & value)
 * {
 *     switch(status)
 *     {
 *     case fluid_settings::fluid_settings_status_t::FLUID_SETTINGS_STATUS_NEW_VALUE:
 *         ...do something here...
 *         break;
 *
 *     default:
 *         // ignore other statuses
 *         break;
 *
 *     }
 * }
 * \endcode
 *
 * \note
 * For a more complete example of the fluid_settings_changed() function,
 * look at the cli/client.cpp file. All the statuses are handled in that
 * one. Just keep in mind that in many cases you do not need this function.
 *
 * The fluid_settings_changed() function is also useful if the change has
 * to be applied immediately and it would not otherwise happen for a while.
 * This function is called immediately when a message is received. In
 * other words, you can react very quickly to parameters changed by
 * fluid-settings.
 *
 * The values are sent a first time when you first register with the
 * fluid-settings daemon. They then get sent each time they get updated,
 * which is most likelihood is a rare event in a production system. Note
 * that this means you receive all the values some time later. That affects
 * the way you want to initialize your daemons. For that purpose, the
 * fluid_settings_connection has a special event which lets you know once
 * all the watched values were received and thus when it is safe for your
 * daemon to finish up its initialization.
 *
 * \note
 * Note that you receive one message per parameter updated.
 * This means you may wake up 10 times if 10 parameters are changed in a
 * row. This will change in the future once we have the BEGIN + COMMIT
 * feature implemented in fluid-settings. Until that change happens, you
 * may want to consider using a "signal". Make sure you change the same
 * set of parameters each time and the last one works as the trigger.
 * \note
 * For example, if you receive coordinates (x, y, z), you can react when
 * the z parameter is updated and ignore the updates of x and y knowing
 * that the update of the z parameter will arrive last once all three
 * parameters are properly set.
 *
 * \param[in] opts  The options on your command line.
 * \param[in] service_name  Your service name to register with the communicator
 * daemon and receive replies from other services.
 *
 * \sa automatic_watch_initialization()
 */
fluid_settings_connection::fluid_settings_connection(
          advgetopt::getopt & opts
        , std::string const & service_name)
    : communicator(opts, service_name)
    , f_opts(opts)
{
    f_opts.parse_options_info(g_options, true);
}


fluid_settings_connection::~fluid_settings_connection()
{
}


/** \brief Automatically initialize the list of parameters to watch.
 *
 * This function goes through your list of watches as defined in your f_opts
 * object and add them to this fluid settings connection.
 *
 * The process goes through the list of options found in the f_opts you
 * supplied to the constructor and performs to checks on those options:
 *
 * \li Is the option already defined, if so, do not add it as a watch; this
 * means options defined on the command line, in an environment variable,
 * or a configuration file prior to calling this function are not going to
 * be handled by the fluid-settings daemon
 *
 * \li The option is marked as a "dynamic-configuration" option. This is the
 * way fluid-settings is authorized to add this option to the list of watches.
 *
 * If you call this function early on (in generally, before making the
 * `f_opts.finish_parsing(argc, argv);` call), then the `is_defined()`
 * flag will always be false meaning that the fluid-settings will take
 * over if defined, whether the parameter was set on the command line or
 * not. This is true even if the fluid-settings parameter only has a
 * default value. If the fluid-settings has no default and is not currently
 * set, then the command line, environment variable, or configuration file
 * value will be kept.
 */
void fluid_settings_connection::automatic_watch_initialization()
{
    advgetopt::option_info::map_by_name_t const & options(f_opts.get_options());
    for(auto const & o : options)
    {
        if(!o.second->is_defined()
        && o.second->has_flag(advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION))
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

    // for some messages, we have to make use of a dynamic std::function()
    // with std::bind() because we need to use the callback priority
    //
    d->add_matches({
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_default_value, &fluid_settings_connection::msg_fluid_default_value),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_deleted,       &fluid_settings_connection::msg_fluid_deleted),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_options,       &fluid_settings_connection::msg_fluid_options),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_registered,    &fluid_settings_connection::msg_fluid_registered),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_updated,       &fluid_settings_connection::msg_fluid_updated),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_value,         &fluid_settings_connection::msg_fluid_value),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_value_updated, &fluid_settings_connection::msg_fluid_value_updated),
        DISPATCHER_MATCH(g_name_fluid_settings_cmd_fluid_settings_ready,         &fluid_settings_connection::msg_fluid_ready),

        ed::define_match(
              ed::Expression(communicatord::g_name_communicatord_cmd_status)
            , ed::Callback(std::bind(&fluid_settings_connection::msg_status, this, std::placeholders::_1))
            , ed::MatchFunc(&ed::one_to_one_callback_match)
            , ed::Priority(ed::dispatcher_match::DISPATCHER_MATCH_CALLBACK_PRIORITY)
        ),
        ed::define_match(
              ed::Expression(ed::g_name_ed_cmd_invalid)
            , ed::Callback(std::bind(&fluid_settings_connection::msg_fluid_error, this, std::placeholders::_1))
            , ed::MatchFunc(&ed::one_to_one_callback_match)
            , ed::Priority(ed::dispatcher_match::DISPATCHER_MATCH_CALLBACK_PRIORITY)
        ),
        ed::define_match(
              ed::Expression(ed::g_name_ed_cmd_unknown)
            , ed::Callback(std::bind(&fluid_settings_connection::msg_fluid_error, this, std::placeholders::_1))
            , ed::MatchFunc(&ed::one_to_one_callback_match)
            , ed::Priority(ed::dispatcher_match::DISPATCHER_MATCH_CALLBACK_PRIORITY)
        ),
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


bool fluid_settings_connection::is_registered() const
{
    return f_registered;
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
    msg.set_command(g_name_fluid_settings_cmd_fluid_settings_get);
    msg.set_service(g_name_fluid_settings_service_fluid_settings);
    msg.add_parameter(g_name_fluid_settings_param_name, qualify_name(name));
    msg.add_parameter(communicatord::g_name_communicatord_param_cache, "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_all_values(std::string const & name)
{
    ed::message msg;
    msg.set_command(g_name_fluid_settings_cmd_fluid_settings_get);
    msg.set_service(g_name_fluid_settings_service_fluid_settings);
    msg.add_parameter(g_name_fluid_settings_param_name, qualify_name(name));
    msg.add_parameter(g_name_fluid_settings_param_all, g_name_fluid_settings_value_true);
    msg.add_parameter(communicatord::g_name_communicatord_param_cache, "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_value_with_priority(std::string const & name, priority_t priority)
{
    ed::message msg;
    msg.set_command(g_name_fluid_settings_cmd_fluid_settings_get);
    msg.set_service(g_name_fluid_settings_service_fluid_settings);
    msg.add_parameter(g_name_fluid_settings_param_name, qualify_name(name));
    msg.add_parameter(g_name_fluid_settings_param_priority, priority);
    msg.add_parameter(communicatord::g_name_communicatord_param_cache, "no;reply");
    send_message(msg);
}


void fluid_settings_connection::get_settings_default_value(std::string const & name)
{
    ed::message msg;
    msg.set_command(g_name_fluid_settings_cmd_fluid_settings_get);
    msg.set_service(g_name_fluid_settings_service_fluid_settings);
    msg.add_parameter(g_name_fluid_settings_param_name, qualify_name(name));
    msg.add_parameter(g_name_fluid_settings_param_default_value, g_name_fluid_settings_value_true);
    msg.add_parameter(communicatord::g_name_communicatord_param_cache, "no;reply");
    send_message(msg);
}


void fluid_settings_connection::add_watch(std::string const & name)
{
    std::string const watch(qualify_name(name));

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

    if(!msg.has_parameter(g_name_fluid_settings_param_destination_service))
    {
        return;
    }

    // are we concerned about this message?
    //
    std::string const service(msg.get_parameter(g_name_fluid_settings_param_destination_service));
    if(service != g_name_fluid_settings_service_fluid_settings)
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
    // fluid-settings messages and give a chance to the user messenger to
    // override this function
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
    if(!msg.has_parameter(g_name_fluid_settings_param_name)
    || !msg.has_parameter(g_name_fluid_settings_param_value))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \""
            << g_name_fluid_settings_param_name
            << "\" or a \""
            << g_name_fluid_settings_param_value
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_DEFAULT
        , msg.get_parameter(g_name_fluid_settings_param_name)
        , msg.get_parameter(g_name_fluid_settings_param_value));
}


void fluid_settings_connection::msg_fluid_deleted(ed::message & msg)
{
    if(!msg.has_parameter(g_name_fluid_settings_param_name))
    {
        SNAP_LOG_ERROR
            << "reply to DELETE command did not include a \""
            << g_name_fluid_settings_param_name
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_DELETED
        , msg.get_parameter(g_name_fluid_settings_param_name)
        , std::string());
}


void fluid_settings_connection::msg_fluid_error(ed::message & msg)
{
    SNAP_LOG_ERROR
        << "an error occurred in the fluid-settings daemon: "
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
    if(!msg.has_parameter(g_name_fluid_settings_param_options))
    {
        SNAP_LOG_ERROR
            << "reply to "
            << g_name_fluid_settings_cmd_fluid_settings_list
            << " command did not include an \""
            << g_name_fluid_settings_param_options
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    advgetopt::string_list_t options;
    advgetopt::split_string(msg.get_parameter(g_name_fluid_settings_param_options), options, {","});

    fluid_settings_options(options);
}


void fluid_settings_connection::msg_fluid_registered(ed::message & msg)
{
    if(msg.has_parameter(ed::g_name_ed_param_message))
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
    if(!msg.has_parameter(g_name_fluid_settings_param_name))
    {
        SNAP_LOG_ERROR
            << "reply to SET command did not include a \""
            << g_name_fluid_settings_param_name
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_UPDATED
        , msg.get_parameter(g_name_fluid_settings_param_name)
        , std::string());
}


void fluid_settings_connection::msg_fluid_value(ed::message & msg)
{
    if(!msg.has_parameter(g_name_fluid_settings_param_name)
    || !msg.has_parameter(g_name_fluid_settings_param_value))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \""
            << g_name_fluid_settings_param_name
            << "\" or a \""
            << g_name_fluid_settings_param_value
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    fluid_settings_changed(
          fluid_settings_status_t::FLUID_SETTINGS_STATUS_VALUE
        , msg.get_parameter(g_name_fluid_settings_param_name)
        , msg.get_parameter(g_name_fluid_settings_param_value));
}


void fluid_settings_connection::msg_fluid_value_updated(ed::message & msg)
{
    if(!msg.has_parameter(g_name_fluid_settings_param_name))
    {
        SNAP_LOG_ERROR
            << "reply to GET command did not include a \""
            << g_name_fluid_settings_param_name
            << "\" parameter."
            << SNAP_LOG_SEND;
        return;
    }

    if(msg.has_parameter(g_name_fluid_settings_param_value))
    {
        // get the name and value from the message
        //
        std::string const & name(msg.get_parameter(g_name_fluid_settings_param_name));
        std::string const & value(msg.get_parameter(g_name_fluid_settings_param_value));

        // if the option exists in f_opts
        //
        std::string opt_name(name);
        std::string const intro(service_name() + "::");
        if(name.substr(0, intro.length()) == intro)
        {
            opt_name = name.substr(intro.length());
        }
        advgetopt::option_info::map_by_name_t const & options(f_opts.get_options());
        auto const it(options.find(opt_name));
        if(it != options.end()
        && it->second->has_flag(advgetopt::GETOPT_FLAG_DYNAMIC_CONFIGURATION))
        {
            // the option exists and it is a DYNAMIC option so update it
            // automatically
            //
            f_opts.add_option_from_string(
                  it->second
                , value
                , "--fluid-settings--"
                , advgetopt::string_list_t()
                , advgetopt::option_source_t::SOURCE_DYNAMIC);
        }

        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_NEW_VALUE
            , name
            , value);
    }
    else if(msg.has_parameter(g_name_fluid_settings_param_error))
    {
        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_UNDEFINED
            , msg.get_parameter(g_name_fluid_settings_param_name)
            , std::string());
    }
}


void fluid_settings_connection::msg_fluid_ready(ed::message & msg)
{
    if(msg.has_parameter(g_name_fluid_settings_param_error))
    {
        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_READY
            , std::string()
            , msg.get_parameter(g_name_fluid_settings_param_error));
    }
    else
    {
        fluid_settings_changed(
              fluid_settings_status_t::FLUID_SETTINGS_STATUS_READY
            , std::string()
            , std::string());
    }
}


void fluid_settings_connection::msg_status(ed::message & msg)
{
    if(!msg.has_parameter(communicatord::g_name_communicatord_param_status)
    || !msg.has_parameter(communicatord::g_name_communicatord_param_service))
    {
        return;
    }

    std::string const status(msg.get_parameter(communicatord::g_name_communicatord_param_status));
    std::string const service(msg.get_parameter(communicatord::g_name_communicatord_param_service));

    if(service == g_name_fluid_settings_service_fluid_settings)
    {
        f_registered = status == communicatord::g_name_communicatord_value_up;
        if(f_registered)
        {
            if(f_watches.empty())
            {
                // if there are no parameters to watch, we're ready now
                // (i.e. all the parameters may have been specified on
                // the command line or a .conf file in which case it does
                // not become dynamic fluid settings)
                //
                fluid_settings_changed(
                      fluid_settings_status_t::FLUID_SETTINGS_STATUS_READY
                    , std::string()
                    , std::string());
            }
            else
            {
                listen(snapdev::join_strings(f_watches, ","));
            }
        }
    }

    service_status(service, status);
}


void fluid_settings_connection::listen(std::string const & watches)
{
    ed::message msg;
    msg.set_command(g_name_fluid_settings_cmd_fluid_settings_listen);
    msg.set_service(g_name_fluid_settings_service_fluid_settings);
    msg.add_parameter(g_name_fluid_settings_param_names, watches);
    msg.add_parameter(communicatord::g_name_communicatord_param_cache, "no;reply");
    send_message(msg);
}


void fluid_settings_connection::ready(ed::message & msg)
{
    snapdev::NOT_USED(msg);

    // get the status of fluid-settings and if UP start listening to
    // the given parameter(s) -- see the client::msg_status() function
    //
    ed::message reply;
    reply.set_command(communicatord::g_name_communicatord_cmd_service_status);
    reply.add_parameter(communicatord::g_name_communicatord_param_service, g_name_fluid_settings_service_fluid_settings);
    send_message(reply);

    // the ready() function is not overridden in the communicator class
    // so the following would call the default which generates a warning
    //
    //communicator::ready(msg);
}


} // fluid_settings namespace
// vim: ts=4 sw=4 et

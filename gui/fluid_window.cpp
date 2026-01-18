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
 * \brief Top window for the GUI version of Fluid Settings.
 *
 * This file is the implementation of the GUI version of the Fluid Settings.
 * It is primarily intended to be used by administrators to manage their
 * website settings remotely in a GUI environment, making it easy and
 * efficient to handle a large network.
 */

// self
//
#include    "fluid_window.h"

#include    "fluid-settings/version.h"


// advgetopt
//
#include    <advgetopt/exception.h>


// snaplogger
//
#include    <snaplogger/message.h>
#include    <snaplogger/options.h>


// Qt
//
#include    <QSettings>


// snapdev
//
#include    <snapdev/stringize.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{



advgetopt::option const g_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("live")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("permanently listen to changes.")
    ),
    advgetopt::define_option(
          advgetopt::Name("sleepy")
        , advgetopt::Flags(advgetopt::all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
            , advgetopt::GETOPT_FLAG_REQUIRED>())
        , advgetopt::Help("manually check for changes.")
    ),
    advgetopt::end_options()
};


advgetopt::group_description const g_group_descriptions[] =
{
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_COMMANDS)
        , advgetopt::GroupName("command")
        , advgetopt::GroupDescription("Commands:")
    ),
    advgetopt::define_group(
          advgetopt::GroupNumber(advgetopt::GETOPT_FLAG_GROUP_OPTIONS)
        , advgetopt::GroupName("option")
        , advgetopt::GroupDescription("Options:")
    ),
    advgetopt::end_groups()
};



#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings-gui",
    .f_group_name = "fluid-settings",
    .f_options = g_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = nullptr,
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>]\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = FLUID_SETTINGS_VERSION_STRING,
    .f_license = "GNU GPL v3",
    .f_copyright = "Copyright (c) 2022-"
                   SNAPDEV_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = g_group_descriptions,
};
#pragma GCC diagnostic pop



}
// no name namespace




/** \brief Initialize the Fluid window.
 *
 * This function initializes the main window of the GUI application managing
 * the Fluis Settings of a network.
 */
FluidWindow::FluidWindow(int argc, char * argv[], QApplication & app)
    : QMainWindow()
    , communicator_connection(f_opts, "fluid_settings_gui")
    , f_application(app) // Note: &f_application == qApp
    , f_opts(g_options_environment)
    , f_communicator(ed::communicator::instance())
{
    snaplogger::add_logger_options(f_opts);
    f_opts.finish_parsing(argc, argv);
    if(!snaplogger::process_logger_options(f_opts, "/etc/fluid-settings/logger"))
    {
        throw advgetopt::getopt_exit("invalid logger options", 1);
    }
    process_communicator_options();

    setup_qt_connection();

    setWindowIcon(QIcon(":/icons/logo.png"));

    setupUi(this);

    // TODO: We do not receive this event...
    //
    //       Note that I tried to NOT remove the f_qt_connection in the close
    //       event and it did not help, plus I think it should happen before
    //       because the about to quit is expected to be used to prevent the
    //       quit if something warrants it (i.e. something not saved yet)
    //
    //connect(
    //      &f_application
    //    , &QApplication::aboutToQuit
    //    , this
    //    , &FluidWindow::on_about_to_quit);

    QSettings const settings(this);
    restoreGeometry(settings.value("geometry", saveGeometry()).toByteArray());
    restoreState(settings.value("state", saveState()).toByteArray());
    f_project_splitter->restoreState(settings.value("projectSplitterState", f_project_splitter->saveState()).toByteArray());
    f_variable_splitter->restoreState(settings.value("variableSplitterState", f_variable_splitter->saveState()).toByteArray());
}


FluidWindow::~FluidWindow()
{
}


void FluidWindow::setup_qt_connection()
{
    f_qt_connection = std::make_shared<ed::qt_connection>();
    f_communicator->add_connection(f_qt_connection);
}


int FluidWindow::run()
{
    SNAP_LOG_VERBOSE
        << "Starting communicator loop"
        << SNAP_LOG_SEND;

    f_communicator->run();
    return 0;
}


void FluidWindow::on_about_to_quit()
{
    QSettings settings(this);
    settings.setValue("geometry", saveGeometry());
    settings.setValue("state", saveState());
    settings.setValue("projectSplitterState", f_project_splitter->saveState());
    settings.setValue("variableSplitterState", f_variable_splitter->saveState());
}


void FluidWindow::on_action_quit_triggered()
{
    close();
}


void FluidWindow::closeEvent(QCloseEvent * event)
{
    f_communicator->debug_connections();
    f_communicator->set_show_connections(true);

    // TODO: until the on_about_to_quit() works, we call this explicitly
    //
    on_about_to_quit();

    QMainWindow::closeEvent(event);

    f_communicator->remove_connection(f_qt_connection);
    f_qt_connection.reset();

    unregister_communicator(false);

    f_communicator->log_connections();
}



// vim: ts=4 sw=4 et

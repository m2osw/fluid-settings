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
 * \brief This tool is used to install settings definitions.
 *
 * The fluid settings reads settings definitions from one specific directory.
 * This command will install fluid settings definitions files in that
 * location.
 *
 * Usage:
 *
 *     install-fluid-settings-definitions <source>
 */

// self
//
#include    "fluid-settings/settings.h"

#include    "fluid-settings/version.h"


// advgetopt
//
#include    <advgetopt/exception.h>


// eventdispatcher
//
#include    <eventdispatcher/signal_handler.h>


// libexcept
//
#include    <libexcept/file_inheritance.h>


// snapdev
//
#include    <snapdev/pathinfo.h>


// boost
//
#include    <boost/preprocessor/stringize.hpp>


// C++
//
#include    <iostream>


// C
//
#include    <unistd.h>
#include    <limits.h>


// last include
//
#include    <snapdev/poison.h>



namespace
{


advgetopt::option const g_command_line_options[] =
{
    advgetopt::define_option(
          advgetopt::Name("symlink")
        , advgetopt::ShortName('s')
        , advgetopt::Flags(advgetopt::standalone_all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
              >())
        , advgetopt::Help("create a symbolic link instead of copying the file.")
    ),
    advgetopt::define_option(
          advgetopt::Name("verbose")
        , advgetopt::ShortName('v')
        , advgetopt::Flags(advgetopt::standalone_all_flags<
              advgetopt::GETOPT_FLAG_GROUP_OPTIONS
              >())
        , advgetopt::Help("display what the tool does.")
    ),
    advgetopt::define_option(
          advgetopt::Name("--")
        , advgetopt::Flags(advgetopt::command_flags<
              advgetopt::GETOPT_FLAG_MULTIPLE
            , advgetopt::GETOPT_FLAG_DEFAULT_OPTION
            , advgetopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR>())
        , advgetopt::Help("<fluid settings filename>")
    ),
    advgetopt::end_options()
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
constexpr advgetopt::options_environment const g_options_environment =
{
    .f_project_name = "fluid-settings",
    .f_group_name = "fluid-settings",
    .f_options = g_command_line_options,
    .f_options_files_directory = nullptr,
    .f_environment_variable_name = "INSTALL_FLUID_SETTINGS_DEFINITIONS",
    .f_environment_variable_intro = nullptr,
    .f_section_variables_name = nullptr,
    .f_configuration_files = nullptr,
    .f_configuration_filename = nullptr,
    .f_configuration_directories = nullptr,
    .f_environment_flags = advgetopt::GETOPT_ENVIRONMENT_FLAG_PROCESS_SYSTEM_PARAMETERS,
    .f_help_header = "Usage: %p [-<opt>] <settings-definitions filename>\n"
                     "where -<opt> is one or more of:",
    .f_help_footer = "%c",
    .f_version = FLUID_SETTINGS_VERSION_STRING,
    .f_license = "GNU GPL v3",
    .f_copyright = "Copyright (c) 2022-"
                   BOOST_PP_STRINGIZE(UTC_BUILD_YEAR)
                   " by Made to Order Software Corporation -- All Rights Reserved",
    .f_build_date = UTC_BUILD_DATE,
    .f_build_time = UTC_BUILD_TIME,
    .f_groups = nullptr,
};
#pragma GCC diagnostic pop


}
// no name namespace




int main(int argc, char *argv[])
{
    ed::signal_handler::create_instance();
    libexcept::verify_inherited_files();

    try
    {
        advgetopt::getopt opts(g_options_environment, argc, argv);
        bool const verbose(opts.is_defined("verbose"));
        bool const symlink(opts.is_defined("symlink"));

        std::string path(fluid_settings::settings::get_default_path());
        if(path.empty())
        {
            std::cerr << "error: somehow the default fluid settings definitions path is empty.\n";
            return 1;
        }

        if(path.back() != '/')
        {
            path += '/';
        }

        // TODO: use stat() and make sure it is a directory
        //       (add functionality to snapdev/pathinfo.h)
        //
        if(access(path.c_str(), R_OK | X_OK) != 0)
        {
            std::cerr
                << opts.get_program_name()
                << ": could not access \""
                << path
                << "\". Does that directory exist?\n";
            return 1;
        }

        std::size_t const max(opts.size("--"));
        if(max == 0)
        {
            std::cerr
                << opts.get_program_name()
                << ": no files specified. Try again with at least one input filename.\n";
            return 1;
        }
        int exit_code(0);
        for(std::size_t i(0); i < max; ++i)
        {
            std::string source(opts.get_string("--", i));

            // TODO: use stat() instead and verify that the file is
            //       a regular file
            //       (add functionality to snapdev/pathinfo.h)
            //
            if(access(source.c_str(), R_OK) != 0)
            {
                std::string const s2(source + ".ini");
                if(access(s2.c_str(), R_OK) != 0)
                {
                    std::cerr
                        << opts.get_program_name()
                        << ": cannot access \""
                        << source
                        << "\".\n";
                    exit_code = 1;
                    continue;
                }
                source = s2;
            }

            if(verbose)
            {
                std::cout
                    << opts.get_program_name()
                    << ": copy \""
                    << source
                    << "\" to \""
                    << path
                    << "\".\n";
            }

            // TODO: consider using `install` instead of `cp`/`ln`
            //       and offer the backup capability here
            //
            if(symlink)
            {
                std::string destination(path);
                destination += snapdev::pathinfo::basename(source);
                char real[PATH_MAX + 1];
                if(realpath(source.c_str(), real) == nullptr)
                {
                    std::cerr
                        << opts.get_program_name()
                        << ": could not determine real path of source \""
                        << source
                        << "\".\n";
                    continue;
                }
                real[PATH_MAX] = '\0';
                std::string real_source(real);
                std::string cmd("ln -s \"");
                cmd += real_source;
                cmd += "\" \"";
                cmd += destination;
                cmd += "\"";
                if(unlink(destination.c_str()))
                {
                    if(errno != ENOENT)
                    {
                        std::cerr
                            << opts.get_program_name()
                            << ": could not delete existing destination file \""
                            << destination
                            << "\" before creating symbolic link to \""
                            << source
                            << "\".\n";
                        continue;
                    }
                }
                if(system(cmd.c_str()) != 0)
                {
                    std::cerr
                        << opts.get_program_name()
                        << ": could not link file \""
                        << source
                        << "\" to \""
                        << path
                        << "\".\n";
                    exit_code = 1;
                }
            }
            else
            {
                std::string cmd("cp \"");
                cmd += source;
                cmd += "\" \"";
                cmd += path;
                cmd += "\"";
                if(system(cmd.c_str()) != 0)
                {
                    std::cerr
                        << opts.get_program_name()
                        << ": could not copy file \""
                        << source
                        << "\" to \""
                        << path
                        << "\".\n";
                    exit_code = 1;
                }
            }
        }

        return exit_code;
    }
    catch(advgetopt::getopt_exit const & e)
    {
        return e.code();
    }
    catch(std::exception const & e)
    {
        std::cerr
            << "error: an exception occurred: "
            << e.what()
            << "\n";
    }

    return 1;
}



// vim: ts=4 sw=4 et

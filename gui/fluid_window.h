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
 * \brief Top window for the GUI version of Fluid Settings.
 *
 * This file is the declaration of the GUI version of the Fluid Settings.
 */

// window as generated by Qt
//
#include    "ui_fluid_settings.h"


// communicatord
//
#include    <communicatord/communicatord.h>


// eventdispatcher
//
#include    <eventdispatcher/communicator.h>
#include    <eventdispatcher/logrotate_udp_messenger.h>
#include    <eventdispatcher/qt_connection.h>




class FluidWindow
    : public QMainWindow
    , private Ui::FluidWindow
    , public communicatord::communicator
{
private:
    Q_OBJECT

public:
                                    FluidWindow(int argc, char * argv[], QApplication & app);
    virtual                         ~FluidWindow();

    int                             run();

protected:
    virtual void                    closeEvent(QCloseEvent * event) override;

private slots:
    void                            on_action_quit_triggered();

private:
    void                            setup_qt_connection();
    void                            on_about_to_quit();

    QApplication &                  f_application;
    advgetopt::getopt               f_opts;
    ed::communicator::pointer_t     f_communicator = ed::communicator::pointer_t();
    ed::connection::pointer_t       f_qt_connection = ed::connection::pointer_t();
};



// vim: ts=4 sw=4 et

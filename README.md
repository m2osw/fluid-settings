
# Introduction

The Fluid Settings project is a three part environment:

* A Daemon which manages the settings. We consider that there has to be one
  instance in your entire system, but for high availability and data
  redundance, you can have more. They will auto-sync. each other.
* A library to seemlessly use the settings; you can just write C++ code.
* A command line, CUI, GUI set of tools, so it is possible to test and write
  scripts and also manually manipulate your settings.

This is extremely useful as many settings are shared between many services
on all your computers and by default it makes it really difficult to manage
those settings if defined in static configuration files on hundred of
diffenrent computers.

The Fluid Settings are instead managed by a service and thus centralized.
Also changing a setting at any point in time allows for any users of that
setting to hear about the change _instantaneously_ and thus act on it
quickly. For example, if you change the IP address of a computer, you can
very quickly update that IP address in the settings and whichever process
using the old IP can automatically switch to the new IP.


# Settings

## Jira Entry

Our Jira has an [issue about this feature (SNAP-745)](https://jira.m2osw.com/browse/SNAP-745),
which has many definitions about the Fluid Settings.

## Defining One Value

Each setting is defined by a _name_ and it can be assigned one _value_.

Both are complex, multi-value items, which are defined below.

### Settings Names

The name (or key) of a Fluid Setting is defined by three parameters:
a Namespace, a Name, and the Priority.

    <namespace>::<name>::<priority>

#### The Namespace

The name starts with one or more Namespaces.

Defining at least one Namespace for a value is mandatory. In most cases,
it is the name of the concerned application (i.e. "fluid-settings").

You may have more than one Namespace if you have sub-sections or sub-tools
in your project and you want to clearly separate the settings of each
section/tool.

The Fluid Settings (advgetopt, really) parser accepts any number of colons
between each name, but the correct (canonicalized) convension is exactly
two colons.

#### The Actual Name

The actual name of the setting. For example, if you need an IP address, you
could have a parameter named "ip".

    fluid-settings::ip

**Note:** As far as the user is concerned, name is really all the namespaces
and the name. The priority is sent as a separate value, though.

The name is further constrained by the fact that it has to be letters,
underscores, and digits only:

    [A-Za-z_][A-Za-z_0-9]

To make it easier, the parser allows for dashes, but all dashes are
replaced to underscores in the Fluid Settings.

#### The Priority

To simulate the advgetopt `<name>.d` sub-directory, we have a priority
which we add to the name. In ASCII, we make sure to always have
exactly two digis (so it starts with 00 and ends with 99).

When a Fluid Setting service receives a value update without a priority, it
uses the default priority, which is 50.

The application is expected to define its default values using the priority
of 0. This can be setup at the time an application is installed. These
values are not expected to be changed except when an application gets
updated.

Other applications are expected to use a priority between 1 and 49 to
define their own default for your application. The administrator may
still replace those settings using the priority of 50.

Some parameters (very few), may need to be forced to a specific value
without giving a chance to the administrator to overwrite that value.
In that case, your application may use a priority between 51 and 99.

**Note:** The priority used by other applications just need to be specific
to that one application. So we could end up having conflicts. Just make sure
to update those applications so all can have a distinct priority.

### Settings Value

The value includes multiple parameters because we want to attach other
values than just the user value to each variable. Specifically, we want
to have a field that defines the timestamp when the value was last updated.
This is very important as we run multiple instances of the Fluid Settings.

To handle this, we use the JSON syntax. A field name, written between double
quotes, a colon, then a value. Each value separated by a comma. The whole
written between curly brackets. This is written on a single line in the file.

Here we show one value on multiple lines for clarity:

    {
        "value": <the actualy value>,
        "timestamp": <time in ns>
    }

#### "value"

This is the actual variable value. It can be set to true, false, a number,
a string. We do not support arrays or objects as the value. If you want an
array of values, you have to use a string for that purpose and parse that
string on your own.

#### "timestamp"

The time when the value was last set.

> TBD: This can be the time from the computer that sent the new value
> (assuming your computers are all in sync. time wise, like with ntp).
> Otherwise, the receiving computer's time gets used.

When the Fluid Setting service receives a new value (`PUT`), its timestamp
is compared against the timestamp of the existing value with the same
key (namespace, name, priority). If the new value has a timestamp larger than
the existing value timestamp, then it gets saved. If the timestamp is smaller
or equal, it gets dropped.

If no value exists yet (with that exact key) then the timestamp is ignore
and that new value always gets saved.

## Value Definitions

The Fluid Settings is not a data store like a Redis database where you can
add any value or delete old values. Instead, all the values that can be
defined in a given namespace are clearly defined by each project using the
Fluid Settings.

The definitions use a simple `.ini` file.

    [<namespace>::<name>]
    validator=<validator-name>
    default=<default value>
    help=<description string>
    no-arguments
    multiple
    required

The name must include at least one `<namespace>::`. It can include several,
as required.

The validator allows us to verify that the values are considered valid. If it
doesn't match the type of data accepted by that validator, then the value is
refused and an error is sent back to the sender. We use the advgetopt
validator feature and extend that library as required as per our needs.

**Note:** Since we use the advgetopt library, it is possible to change the
syntax with names such as:

    <namespace>::<name>::type=...

This is actually what happens in memory once all the data was read from
these files.

### Fluid Settings and Definitions

The definitions for a given service will reside on the computer running that
service which may not be the same as the Fluid Setting computer. For that
reason, we have to share our settings between all the systems using the
Fluid Settings service.

When a service registers with the Fluid Settings, it sends a Murmur3 code
which the Fluid Settings uses to know whether that service's settings changed.
If so, then the service sends its definitions to Fluid Settings. Otherwise,
it assumes that whatever file it already has is up to date and it just goes
on.

### Unknown Values

Since a definition may not have made it to the Fluid Settings service yet,
we have a way to handle unknown values which gives the service a way to
listen or change values even when the Fluid Settings doesn't yet know
about it (if the service asks for that special feature).

Once the value is finally known (via a Definitions transfer), the Fluid
Settings will let the connectors know that it is now available and it can
be read and written. If a value already exists, it can be verified for
validity.


# Implementation

## Daemon

The settings can stay in memory while being managed by the Fluid Settings
Daemon. So it loads the settings once on startup and then only writes to
the settings to file whenever a change occurs.

This is very similar to having a service such as Redis or even Cassandra.

The default network connection uses the `snapcommunicator` to make the
fluid-settings seemlessly available through your entire network.

### Slow Write

The write to disk only happens after a small amount of time (5 seconds by
default). That way if multiple changes arrive back to back, we can avoid
some I/O.

**TODO:** In a future version, we probably want to write to a journal so
as to be able to replay in case of a crash and the write did not yet
occur. This can be complex to determine the date and time when the last
data was received and saved.

### Fail Safe Feature

In order to support a fail safe feature, the data has to be replicated
between two or three live services. The controller can be in charge of
replicating the SET commands to all the Daemons or we can have a master
at some point (I don't personally like the idea of a master). The daemons
should also verify that they all agree on the current values and the
values need to be assigned a timestamp so we can chose to keep only the
very last ones.

The first implementation will be a `PUT` to one Daemon and then that Daemon
is responsible to tell the other two about the new value(s).

### HTTP Extension

The Fluid Service is accessible using HTTP requests with a `GET` (retrieve
a value) or a `PUT` (modify a value). Since values can't be added or deleted,
there is no support for POST or DELETE.

In other words, we have a basic REST API.

TODO: this extension requires us to have the `edhttp` project implemented
(at least the HTTP/1.1 version).

**Note:** The `GET` is not available through the standard snapcommunicator
connection. Instead, we have a `LISTEN` command which a standard REST API
would not be able to offer.

**Note 2:** The _HTTP Extension_ is useful if we want to implement a
browser based user interface (a replacement to `snapmanager`).


### Websocket Extension

Later, we could also consider having a Websocket. This is where we connect
once and have push events instead of having to pull for the latest value(s).
This better replicates the snapcommunicator `LISTEN` capability.

TBD: HTTP2/3 probably also have a push feature so we may instead want to
use those and not yet another feature.


## Library

The library is the interface used to access the Fluid Settings Daemon from
other daemons and command line tools that want to make use of various
settings.

It has two main features: set and listen to settings. There
is no "get" since a setting can change at any time. The fact that you have
to listen creates only a small hurdle, which is that you have to wait for
the data to come in, so instead of a common linear process as you would
normally have, you create your controller connection send a request about
the settings you want to listen to, and sit until you receive the current
values for those settings. Once you have all the settings you need you can
proceed with your normal daemon or tool tasks.


## Command Line Tool (CLI)

In order to allow for scripts to tweak the settings and for you to be able
to manually check the current settings status, we offer a command line tool
which does the listening and also allows you to change values.

The CLI tool supports these command line options:

* `--delete | -D <setting name>` -- delete the user defined value of
  the named setting; this restore the value's default.
* `--set | -s <setting name> <new value>` -- set the user defined value of
  the named setting to the new value.
* `--get | -g <setting name>` -- get the user defined value of the named
  setting and print that value in the console.
* `--list-services` -- print the list of services that have fluid settings;
  this is a list of the first "namespace" section of a fluid setting option.
* `--list-options <service>` -- print the list of options a specific service
  supports.


## Interactive Tool (CUI)

This tool can opens a window in your console and allows you to go through
the list of settings and tweak them as required. If some values get updated
through other means and are currently being displayed, then the change also
happens in this window.


## GUI Tool

### Desktop GUI

We can also have a GUI Tool which requests the entire tree of preferences
and displays it. Then you can edit the values directly from that window.

### Web-based GUI

We can also have a GUI Tool that runs as a web-based service. This could
be `snapmanager` redesigned to work with Fluid Settings.


## Other Tools

The tools folder has a few tools one can use on the command line.

* Settings Monitor

  This tool can be used to detect whenever a variable changes.

* List Settings

  This tool lists the settings found on a computer. This is a direct read
  of fluid definition files found locally.


# Dependencies

The project makes use of the follow Snap! C++ contribs:

* advgetopt
* cppthread (for mutexes)
* eventdispatcher
* libaddr
* libexcept
* snapcatch2
* snapcmakemodules
* snapdev
* snaplogger


# License

The project is covered by the GPL 3.0 license.


# Bugs

Submit bug reports and patches on
[github](https://github.com/m2osw/fluid-settings/issues).


_This file is part of the [snapcpp project](https://snapwebsites.org/)._

vim: ts=4 sw=4 et

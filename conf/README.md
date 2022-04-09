
# Fluid Settings Configurations

This sub-directory is where administrators create settings files to
overwrite the defaults.

The filename is expected to be something like `50-fluid-settings.conf`
where `50` is a priority. Other projects save their defaults either
before (can be overwritten by admins) or after (should not be overwritten
by admins).

To get a list of these filenames, use the `--configuration-filenames`
command line option. Some tools may not support any configuration
files in which case the tool will show such a message in the output.


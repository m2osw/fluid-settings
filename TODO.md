
# Fluid-Settings Missing Elements

* write unit tests (especially since now we have the reporter language)
* test support for replication capabilities
* on install, a client of fluid-settings may place a file under
  `/usr/share/fluid-settings/definitions/...` for the server to pick up;
  only the server may be running on a separate computer so the client needs
  to send its definitions to the server in that case; without the definitions
  we are missing the valid variable names and default values
* verify that a transfer occurs on startup of each fluid-settings; right now
  this is one way, we need to make it a clear two way street:
  - when A starts, send A's data to B, C, D... **[done]**
  - when B detects A connecting, B sends its data to A **[missing]**
* consider adding a BEGIN and COMMIT / ROLLBACK; that way we can change
  multiple values in an atomic manner; also that way we can avoid the save()
  call & `NEW_VALUE` events until we get the COMMIT
* look into why the `VALUE_CHANGED` message is used to communicate between
  fluid settings daemons (maybe because it's a fluid settings specific channel?)
* implement the GUI
* add necessary to have authentication
  - some systems should only be given read access
  - some systems should not be given any access


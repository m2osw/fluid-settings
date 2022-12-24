
# Fluid-Settings Missing Elements

* test support for replication capabilities
* verify that a transfer occurs on startup of each fluid-settings; right now
  this is one way, we need to make it a clear two way street:
  - when A starts, send A's data to B, C, D... **[done]**
  - when B detects A connecting, B sends its data to A **[missing]**
* consider adding a BEGIN and COMMIT / ROLLBACK; that way we can change
  multiple values in an atomic manner; also that way we can avoid the save()
  call & `NEW_VALUE` events until we get the COMMIT
* Implement the GUI
* add necessary to have authentication
  - some systems should only be given read access
  - some systems should not be given any access


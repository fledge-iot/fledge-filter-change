==============================
FogLAMP "change" Filter plugin
==============================

A filter used to only send information about an asset onwards when a
particular datapoint within that asset changes by more than a configured
percentage. Data is sent for a period of time before and after the
change in the monitored value. The amount of data to send before and
after the change is configured in milliseconds, with a value for the
pre-change tiem and one for the post-change time.

It is possible to define a rate at which readings should be sent
regardless of the monitored value changing. This provides an average o
the values of the period defined, e.g. send a 1 minute average of the
values every minute.

This filter only operates on a single asset, all other assets are passed
through the filter unaltered.

Configuration Items
-------------------

asset
  The asset to monitor and control with this filter. This asset is both
  the asset that is used to look for changes and also the only asset whose
  data is affected by the triggered or non-triggered state of this filter.

trigger
  The datapoint within the asset that is used to trigger the sending of
  data at full rate. This datapoint may be either a numeric value or a
  string. If it is a string then a change of value of the defined change
  percentage or greater will trigger the sending of data. If the value is
  a string then any change in value will trigger the sending of the data.

change
  The percentage change required for a numeric value change to trigger
  the sendign of data. If this value is set to 0 then any change in the
  trigger value will be enough to trigger the sending of data.

preTrigger
  The number of milliseconds worth of data before the change that triggers
  the sending of data will be sent.

postTrigger
  The number if milliseconds after a change that triggered the sending
  of data will be sent. If there is a subsequent change while the data
  is being sent then this period will be reset and the the sending of
  data will recommence.

rate
  The rate at which to send averages if a change does not trigger full
  rate data. This is defined as a number of averages for a period defined
  in the rateUnit, e.g. 4 per hour.

rateUnit
  The unit associated with the average rate above. This may be one of
  "per second", "per minute", "per hour" or "per day".


Build
-----
To build FogLAMP "rate" C++ filter plugin:

.. code-block:: console

  $ mkdir build
  $ cd build
  $ cmake ..
  $ make

- By default the FogLAMP develop package header files and libraries
  are expected to be located in /usr/include/foglamp and /usr/lib/foglamp
- If **FOGLAMP_ROOT** env var is set and no -D options are set,
  the header files and libraries paths are pulled from the ones under the
  FOGLAMP_ROOT directory.
  Please note that you must first run 'make' in the FOGLAMP_ROOT directory.

You may also pass one or more of the following options to cmake to override 
this default behaviour:

- **FOGLAMP_SRC** sets the path of a FogLAMP source tree
- **FOGLAMP_INCLUDE** sets the path to FogLAMP header files
- **FOGLAMP_LIB sets** the path to FogLAMP libraries
- **FOGLAMP_INSTALL** sets the installation path of Random plugin

NOTE:
 - The **FOGLAMP_INCLUDE** option should point to a location where all the FogLAMP 
   header files have been installed in a single directory.
 - The **FOGLAMP_LIB** option should point to a location where all the FogLAMP
   libraries have been installed in a single directory.
 - 'make install' target is defined only when **FOGLAMP_INSTALL** is set

Examples:

- no options

  $ cmake ..

- no options and FOGLAMP_ROOT set

  $ export FOGLAMP_ROOT=/some_foglamp_setup

  $ cmake ..

- set FOGLAMP_SRC

  $ cmake -DFOGLAMP_SRC=/home/source/develop/FogLAMP  ..

- set FOGLAMP_INCLUDE

  $ cmake -DFOGLAMP_INCLUDE=/dev-package/include ..
- set FOGLAMP_LIB

  $ cmake -DFOGLAMP_LIB=/home/dev/package/lib ..
- set FOGLAMP_INSTALL

  $ cmake -DFOGLAMP_INSTALL=/home/source/develop/FogLAMP ..

  $ cmake -DFOGLAMP_INSTALL=/usr/local/foglamp ..

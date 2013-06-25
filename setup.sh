#!/bin/bash

# Or use use devtag
DEV=/dev/ttyUSB0

# Set SIRF NMEA mode
gpsctl  -n -s 4800 $DEV
#stty -F $DEV  ospeed 4800 

# Set time
gps_simple -t $DEV


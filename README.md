gps_simple -- A very simple utility to read gps information
===========================================================

Author
-------					
Robert Olsson <robert@herjulf.se>


Abstract
--------
A very minimal program to extract NMEA messages from USB or serial port
connected gps units. No dependency's could be linked statically to be
independent shared libraries. Can be used to set time on systems lacking
realtime clocks.


Copyright
---------
Open-Source via GPL


Introduction
------------
gps needs first to be setup i NMEA mode so ASCII text can be read from the 
gps. This is different from the binary format. When gps is in NMEA mode 
gps output can be seen i.e via a terminal program. Program is ported 
to devtag simple USB naming by Jens Låås.


Usage
------
# To set gps in NMEA mode
gpsctl  -n -s 4800 /dev/ttyUSB0 

# Get velocity
gps_simple -v /dev/ttyUSB0 
Speed=  1.04

# Get time
gps_simple -t /dev/ttyUSB0 
20110821 18:54.19

# Set time
gps_simple -t /dev/ttyUSB0 
20110821 18:54.19


A typical use is to run gps_simple -s /dev/gps at system boot and also at 
some period interval from cron daemon.


Tested
------
BU-353 USB Sirf III gps [on RPi].






/*
 * gps_simple 
 * A lightweight parse for NMEA time and satellite fix sentences 
 *
 * Copyright Robert Olsson Uppsala, Sweden, robert@herjulf.se
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>

#include "devtag-allinone.h"

#define BUFLEN  124
#define VERSION "1.2 2013-11-01"
#define KNOT_TO_KMPH 1.852 


int bp;
int debug = 0;
int set_time = 0;
int time_mode = 0;
int velocity_mode = 0;
int position_mode = 0;

void usage (void)
{
  printf("\nnmea_light  [-h ] [-t ] [-s] [-v] [-p] nmea-input \n");
  printf("Version %s ", VERSION);
  printf("Options:\n");
  printf(" -h   help\n");
  printf(" -t   get time\n");
  printf(" -s   set time\n");
  printf(" -v   get velocity\n");
  printf(" -p   get position\n");
  printf(" -d   debug\n");
}

/* mktime from linux kernel */

/*
 *  linux/kernel/time.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  This file contains the interface functions for the various
 *  time related system calls: time, stime, gettimeofday, settimeofday,
 *			       adjtime
 */


/* Converts Gregorian date to seconds since 1970-01-01 00:00:00.
 * Assumes input in normal date format, i.e. 1980-12-31 23:59:59
 * => year=1980, mon=12, day=31, hour=23, min=59, sec=59.
 *
 * [For the Julian calendar (which was used in Russia before 1917,
 * Britain & colonies before 1752, anywhere else before 1582,
 * and is still in use by some communities) leave out the
 * -year/100+year/400 terms, and add 10.]
 *
 * This algorithm was first published by Gauss (I think).
 *
 * WARNING: this function will overflow on 2106-02-07 06:28:16 on
 * machines were long is 32-bit! (However, as time_t is signed, we
 * will already get problems at other places on 2038-01-19 03:14:08)
 */
unsigned long
our_mktime(const unsigned int year0, const unsigned int mon0,
       const unsigned int day, const unsigned int hour,
       const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int) (mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long)
		  (year/4 - year/100 + year/400 + 367*mon/12 + day) +
		  year*365 - 719499
	    )*24 + hour /* now have hours */
	  )*60 + min /* now have minutes */
	)*60 + sec; /* finally seconds */
}



void nmew_time(const unsigned int year, const unsigned int mon, const unsigned int day, 
	       const unsigned int hour, const unsigned int min, const unsigned int sec )
{
  struct timeval tv, *tvp;

  tvp = &tv;

  
  tvp->tv_sec = our_mktime(year, mon, day, hour, min, sec);
  tvp->tv_usec = 0;
  
  if( tvp->tv_sec == -1) {
    perror("mktime");
    exit(-1);
  }
  
  if(set_time) {
    if(  settimeofday(tvp, NULL) ) {
      perror("setimeofday");
      exit(-1);
    }
  }
  
  if(!set_time) {
    printf("%02d%02d%02d ", year, mon, day);
    printf("%02d:%02d.%02d\n", hour, min, sec);
  }
  exit(1);
}


/* externs for getopt */

extern  int     optind;
extern  char    *optarg;


struct {
  int   fd;         /* file descriptor */
  short events;     /* requested events */
  short revents;    /* returned events */
} pollfd;


int main(int ac, char **av)
{
  float longi, lat, course, speed;
  char foo[6], buf[BUFLEN], ns, ew;
  char valid;
  int op, try, res, maxtry = 20;
  unsigned int year, mon, day, hour, min, sec;
  int done = 0;
  int fd;

  while ((op = getopt(ac, av,"dhtsvp")) != EOF)
    switch (op)  {
      /* specify an alternate server */
      
    case 't':       time_mode=1;
      break;
      
    case 's':       time_mode=1; set_time=1;
      break;
      
    case 'v':       velocity_mode=1;
      break;

    case 'p':       position_mode=1;
      break;

    case 'd':       debug=1;
      break;


    case 'h':       usage();
      exit(1);
      
    default: 
      break;
    }
 
  if (optind+1 > ac) {
    usage();
    exit(-1);
  }

  fd = open(devtag_get(av[optind]), O_RDWR | O_NOCTTY | O_NONBLOCK);

  for(try = 0; !done && try < maxtry; try++) {

    bp = 0;

    while(bp < BUFLEN) {
      int n, ok;
      char b;
      struct pollfd pfd;
      
      pfd.fd = fd;
      pfd.events = POLLIN;
      ok = poll( &pfd, 1, -1 );
      
      if ( ok < 0 ) 
	continue;
      
      n = read(fd, &b, 1);
      
      if( n < 0)
	continue;
     
      buf[bp++] = b;
      if(b == '\n') 
	break;
    }

#if 0
    if(!time_mode && strncmp("$GPGSA", buf, 6) == 0 ) {
      printf("%s", buf);
      exit(1);
    }
#endif

      if(strncmp("$GPRMC", buf, 6) == 0 ) {
      
      if(debug)
	printf("%s", buf);
      
      res = sscanf(buf, 
		   "$GPRMC,%2d%2d%2d.%3c,%1c,%f,%1c,%f,%1c,%f,%f,%2d%2d%2d",
		   &hour, &min, &sec, foo, &valid, &lat, &ns,  &longi, &ew, &speed, &course, &day, &mon, &year);
      
      if(res == 14 && valid == 'A') {
	
	if( velocity_mode) {
	  printf("Speed=%6.2f\n", speed * KNOT_TO_KMPH);
	  done = 1;
	}

	if( position_mode) {

	  if(ns == 'S')
	    lat = -lat;

	  if(ew == 'W')
	    longi = -longi;
	  
	  printf("LAT=%09.4f LON=%09.4f\n", lat, longi);
	  done = 1;
	}

	if(time_mode) {
	  year += 2000; 
	  nmew_time(year, mon, day, hour, min, sec );
	  done = 1;
	}
      }
    }    
  }
  if(!done)
    printf("error no valid NMEA answer\n");
  exit(-1);
}


/*
We get: $GPGSA,A,3,21,18,03,22,19,27,07,08,26,16,24,,1.7,0.8,1.5*36

$GPGSA,A,3,04,05,,09,12,,,24,,,,,2.5,1.3,2.1*39

Where:
     GSA      Satellite status
     A        Auto selection of 2D or 3D fix (M = manual) 
     3        3D fix - values include: 1 = no fix
                                       2 = 2D fix
                                       3 = 3D fix
     04,05... PRNs of satellites used for fix (space for 12) 
     2.5      PDOP (dilution of precision) 
     1.3      Horizontal dilution of precision (HDOP) 
     2.1      Vertical dilution of precision (VDOP)
     *39      the checksum data, always begins with *

*/


/*

We get: $GPRMC,080642.000,A,5951.0512,N,01736.8681,E,0.10,345.68,140307,,*0D      

$GPRMC,hhmmss.ss,A,llll.ll,a,yyyyy.yy,a,x.x,x.x,ddmmyy,x.x,a*hh



GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W*6A

Where:
     RMC          Recommended Minimum sentence C
     123519       Fix taken at 12:35:19 UTC
     A            Status A=active or V=Void.
     4807.038,N   Latitude 48 deg 07.038' N
     01131.000,E  Longitude 11 deg 31.000' E
     022.4        Speed over the ground in knots
     084.4        Track angle in degrees True
     230394       Date - 23rd of March 1994
     003.1,W      Magnetic Variation
     *6A          The checksum data, always begins with *


RMC  = Recommended Minimum Specific GPS/TRANSIT Data

1    = UTC of position fix
2    = Data status (V=navigation receiver warning)
3    = Latitude of fix
4    = N or S
5    = Longitude of fix
6    = E or W
7    = Speed over ground in knots
8    = Track made good in degrees True
9    = UT date
10   = Magnetic variation degrees (Easterly var. subtracts from true course)
11   = E or W
12   = Checksum


From Wikipedia.

Decimal degrees (DD) express latitude and longitude geographic coordinates as 
decimal fractions and are used in many geographic information systems (GIS), 
web mapping applications such as Google Maps, and GPS devices. Decimal degrees 
are an alternative to using degrees, minutes, and seconds (DMS). As with latitude 
and longitude, the values are bounded by ±90° and ±180° respectively.

Positive latitudes are north of the equator, negative latitudes are south of the 
equator. Positive longitudes are east of Prime Meridian, negative longitudes are 
west of the Prime Meridian. Latitude and longitude are usually expressed in that 
sequence, latitude before longitude.

Bunda Tanzania gps,

$GPRMC,090409.000,A,0201.7783,S,03351.5142,E,0.53,98.53,011113,,*26
$GPGGA,090410.000,0201.7782,S,03351.5138,E,1,09,0.9,1199.2,M,-15.6,M,,0000*52
$GPGSA,A,3,29,22,12,25,31,14,15,21,18,,,,1.6,0.9,1.3*38
$GPRMC,090410.000,A,0201.7782,S,03351.5138,E,0.78,276.26,011113,,*1B

*/

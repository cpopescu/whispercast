/* IP2Location.h
 *
 * Copyright (C) 2005-2007 IP2Location.com  All Rights Reserved.
 *
 * http://www.ip2location.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef HAVE_IP2LOCATION_H
#define HAVE_IP2LOCATION_H

#include <stdlib.h>
#ifndef __APPLE__
#include <malloc.h>
#endif

#include <whisperlib/common/math/imath.h>

#define API_VERSION   2.1.3
#define MAX_IPV4_RANGE  4294967295U
#define MAX_IPV6_RANGE  "340282366920938463463374607431768211455"
#define IPV4 0
#define IPV6 1

#define  COUNTRYSHORT 0x0001
#define  COUNTRYLONG  0x0002
#define  REGION       0x0004
#define  CITY         0x0008
#define  ISP          0x0010
#define  LATITUDE     0x0020
#define  LONGITUDE    0x0040
#define  DOMAIN       0x0080
#define  ZIPCODE      0x0100
#define  TIMEZONE     0x0200
#define  NETSPEED     0x0400
#define  ALL          COUNTRYSHORT | COUNTRYLONG | REGION | CITY | ISP | LATITUDE | LONGITUDE | DOMAIN | ZIPCODE | TIMEZONE | NETSPEED

#define  DEFAULT	     0x0001
#define  NO_EMPTY_STRING 0x0002
#define  NO_LEADING      0x0004
#define  NO_TRAILING     0x0008

#define INVALID_IPV6_ADDRESS "INVALID IPV6 ADDRESS"
#define INVALID_IPV4_ADDRESS "INVALID IPV4 ADDRESS"
#define  NOT_SUPPORTED "This parameter is unavailable for selected data file. Please upgrade the data file."


struct IP2Location {
	FILE *filehandle;
	uint8 databasetype;
	uint8 databasecolumn;
	uint8 databaseday;
	uint8 databasemonth;
	uint8 databaseyear;
	uint32 databasecount;
	uint32 databaseaddr;
	uint32 ipversion;
};

struct IP2LocationRecord {
	char *country_short;
	char *country_long;
	char *region;
	char *city;
	char *isp;
	float latitude;
	float longitude;
	char *domain;
	char *zipcode;
	char *timezone;
	char *netspeed;
};

struct StringList {
	char* data;
	struct StringList* next;
};

IP2Location *IP2Location_open(const char *db);
uint32 IP2Location_close(IP2Location *loc);
IP2LocationRecord*IP2Location_get_country_short(IP2Location *loc,
                                                const char* ip);
IP2LocationRecord* IP2Location_get_country_long(IP2Location *loc,
                                                const char* ip);
IP2LocationRecord* IP2Location_get_region(IP2Location *loc,
                                          const char* ip);
IP2LocationRecord* IP2Location_get_city (IP2Location *loc,
                                         const char* ip);
IP2LocationRecord* IP2Location_get_isp(IP2Location *loc,
                                       const char* ip);
IP2LocationRecord* IP2Location_get_latitude(IP2Location *loc,
                                            const char* ip);
IP2LocationRecord* IP2Location_get_longitude(IP2Location *loc,
                                             const char* ip);
IP2LocationRecord* IP2Location_get_domain(IP2Location *loc,
                                          const char* ip);
IP2LocationRecord* IP2Location_get_zipcode(IP2Location *loc,
                                           const char* ip);
IP2LocationRecord* IP2Location_get_timezone(IP2Location *loc,
                                            const char* ip);
IP2LocationRecord* IP2Location_get_netspeed(IP2Location *loc,
                                            const char* ip);
IP2LocationRecord* IP2Location_get_all(IP2Location *loc,
                                       const char* ip);
void IP2Location_free_record(IP2LocationRecord *record);

#endif

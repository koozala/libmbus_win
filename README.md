# libmbus_win: M-Bus Library and Tools for Windows

## Why MBus?

I was looking for an easy way to connect to a bunch of MBus meters. 

## libmbus_win

The aim of this project is to provide a native Windows library and a toolset for interacting with MBus devices. 

Native Windows was a strict requirement. Installation must be as simple as possible.
The Visual Studio development environment is also a strict requirement.

This software is based on libmbus, an existing implementation provided by the Swedish company Raditex Control AB (http://www.rscada.se/). 

The changes made to the software are motivated by the desire to minimize the number of changes and tweaks to the development environment. Only native libraries should be used, which are universally available. I also made the code more type-safe in accordance with the default settings of the VS compiler.

## Visual Studio settings

Will be described at a different time.

## Licensing

This project is based on the software libmbus, found at https://github.com/rscada/libmbus. The original is licensed under a BSD-3 license, which is also included in this project. 

This port is licensed under the same BSD-3 license.

## Copyright

Modifications
Copyright (c) 2019, EDV-Beratung Vogt GmbH, Germany

Original 
Copyright (c) 2010-2012, Raditex Control AB.
All rights reserved.

## Code changes

General changes:
- Removed the mbus/ prefix from the imports. VC allows file organisation on the solution level, so I tried to simplify. Not sure if this was successful, might redo in the future.
- "#define __PRETTY_FUNCTION__ ..." introduced. 
- Add getopt
- Add unistd.h from Stackoverflow - not sure what this means regarding licenes. Is this even allowed? My acknowledgements go to user AShelly.
- Support for serial communication dropped

mbus-tcp.c:
- Heavy changes to accomodate for the windows implementation of sockets. Too many to list in detail. Now we are using the native implementation.

mbus-protocol.h:
- #include winsock

In file mbus_protocol.c:
- I made the downtyping from size_t to u_char explicit by introducing an explicit type conversion. In addition, I pushed the downcasting to an higher level of the execution stack with the effect that information is preserved as long as possible in the execution sequence. It's functionally equal.
- mbus_data_int_encode: Changed the type of index to size_t 
- mbus_data_float_decode: changed the type of exponent to "long"; made the downcasting in the return statement explicit.
- changed the use of strcpy to strcpy_s 
- mbus_frame_internal_pack: changed int to size_t for index
- mbus_hex_dump: adjusted for the differences to the windows version of gmtime
- mbus_data_variable_record_xml: adjusted to gmtime; removed unused variable; 
- mbus_frame_data: added initialization just to get rid of a compiler warning
- mbus_frame_get_secondary_address: add variable initialization, in this case it feels necessary to have it
- changed strncpy to strncpy_s
- Most of those changes where made simply to remove the warnings of the compiler. The underlying assumption is that a more compliant implementation would be more secure, safer, more "standard" (which leaves less room for interpretation). This assumption might be wrong but in this case, I guess the MBus specification is fundamentally flawed and everybody who comes close would burn in hell. 
- Unfortunately, this would include me. So I take personal risk when being clever.
- I deeply resent burning in hell. 
- Maybe the world has gotten a little bit better.

***
Original README.md:
***

# libmbus: M-bus Library from Raditex Control (http://www.rscada.se) <span style="float:right;"><a href="https://travis-ci.org/rscada/libmbus" style="border-bottom:none">![Build Status](https://travis-ci.org/rscada/libmbus.svg?branch=master)</a></span>

libmbus is an open source library for the M-bus (Meter-Bus) protocol.

The Meter-Bus is a standard for reading out meter data from electricity meters,
heat meters, gas meters, etc. The M-bus standard deals with both the electrical
signals on the M-Bus, and the protocol and data format used in transmissions on
the M-Bus. The role of libmbus is to decode/encode M-bus data, and to handle
the communication with M-Bus devices.

For more information see http://www.rscada.se/libmbus

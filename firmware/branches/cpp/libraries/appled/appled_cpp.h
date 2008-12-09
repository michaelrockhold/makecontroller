/*********************************************************************************

 Copyright 2006-2008 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/*
	appled.h

*/

#ifndef APPLED_CPP_H
#define APPLED_CPP_H

extern "C" {
  #include "config.h"
}

#include "io_cpp.h"

#ifdef OSC
#include "osc_cpp.h"

class AppLedOSC : public OscHandler
{
public:
  AppLedOSC( ) { }
  int onNewMsg( OscTransport t, OscMessage* msg, int src_addr, int src_port );
  int onQuery( OscTransport t, char* address, int element );
  const char* name( ) { return "appled"; }
  static const char* propertyList[];
};
#endif // OSC

class AppLed
{
public:
  AppLed( int index );
  ~AppLed( ) { }
  bool valid( ) { return leds[_index] != NULL; }
  void setState( bool state );
  bool getState( );
  #ifdef OSC
  static AppLedOSC* oscHandler;
  #endif
  
private:
  int getIo(int index);
  int _index;
  static Io* leds[4]; // only ever want to make 4 of these, to allow for multiple instances using the same Io*
};

#endif // APPLED_CPP_H


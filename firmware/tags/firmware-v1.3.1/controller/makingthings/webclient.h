/*********************************************************************************

 Copyright 2006 MakingThings

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
  WebClient.h
  MakingThings
*/

#ifndef WEBCLIENT_H
#define WEBCLIENT_H

int WebClient_Get( int address, int port, char* path, char* hostname, char* buffer, int buffer_size );
int WebClient_Post( int address, int port, char* path, char* hostname, char* buffer, int buffer_length, int buffer_size );

#endif // WEBCLIENT_H

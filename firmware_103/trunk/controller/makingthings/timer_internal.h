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
	timer_internal.h

  timer Internal headers

  MakingThings
*/

#ifndef TIMER_INTERNAL_H
#define TIMER_INTERNAL_H

#include "types.h"

#define TIMER_COUNT 8
#define TIMER_MARGIN 2

typedef struct TimerEntryS
{
  void (*callback)( int id );
  short id;
  int   time;
  int   timeInitial;
  bool  repeat;
  struct TimerEntryS* next;
} TimerEntry;

struct Timer_
{
  char users;
  short count;

  int jitterTotal;
  int jitterMax;
  int jitterMaxAllDay;

  char running;
  char servicing;
  
  TimerEntry* freeFirst;
  TimerEntry* runningFirst;
  TimerEntry* newFirst;

  TimerEntry entry[ TIMER_COUNT ];
};

#endif

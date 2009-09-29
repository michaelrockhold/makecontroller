/*********************************************************************************

 Copyright 2006-2009 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/


#ifndef PIN_H
#define PIN_H

#define ON  true
#define OFF false

#include "config.h"
#include "types.h"
#include <ch.h>
#include <pal.h>

/**
  Control any of the 35 Input/Output signals on the Make Controller.
  
  Each of the signals on the Make Controller can be used as a digital in/out, and many
  can be used as part of one of the peripherals on the board (serial, usb, etc).  The Io 
  class allows you to easily control these signals.
  
  \section Usage
  The most common way to use the Io system is to control pins as GPIO (general purpose in/out).
  To do this, create a new Io object, specifying that you want it to be a GPIO and whether it should
  be an input or an output.  Check the list of signal names at \ref IoIndices.
  
  \code
  Io mysensor(PIN_PA08, Io::GPIO, INPUT); // a new digital in on PA08, as a GPIO input
  bool isOn = mysensor.value(); // now read the input with the value() method
  \endcode
  
  You can just as easily set it up as a digital out, and turn it on and off:
  \code
  Io myLED(PIN_PA08, Io::GPIO, OUTPUT); // a new digital in on PA08, as a GPIO output
  myLED.on(); // turn it on
  myLED.off(); // turn it off
  // or do it with setValue if you want to keep the state in a variable
  bool state = true;
  myLED.setValue(state); // turn it on
  state = false;
  myLED.setValue(state); // turn it off
  
  // we can even change it back into an input
  myLED.setDirection(INPUT);
  // hmm...the myLED name doesn't make so much sense now...oh well
  \endcode
  
  \section Interrupts
  You can also register to get notified automatically, when the status on an Io
  line changes, rather than constantly reading the value to see if it has changed.  
  This can be much more efficient - see addInterruptHandler() and removeInterruptHandler() 
  for details.
  
  \section settings Additional Settings
  Each Io pin also has some additional settings that can optionally be configured.
  - \b Pullup - each signal has an optional pullup resistor that can be enabled.  To 
  configure this, use the pullup() and setPullup() methods.
  - <b>Glitch Filter</b> - each signal also has a glitch filter that can help reject 
  short signals (most signals less than a master clock cycle, which is roughly 20 ns).
  Check the filter() and setFilter() methods.
  
  \ingroup io
*/

typedef void (*PinInterruptHandler)(void*);

typedef enum PinMode_t {
  INPUT = PAL_MODE_INPUT,
  INPUT_PULLUP = PAL_MODE_INPUT_PULLUP,
  OUTPUT = PAL_MODE_OUTPUT_PUSHPULL,
  OUTPUT_OPENDRAIN = PAL_MODE_OUTPUT_OPENDRAIN,
  PERIPHERAL_A,
  PERIPHERAL_B
} PinMode;

bool pinValue(int pin);
void pinSetValue(int pin, bool value);
void pinOn(int pin);
void pinOff(int pin);
void pinSetMode( int pin, PinMode mode );
#ifndef PIN_NO_ISR
bool pinAddInterruptHandler(int pin, PinInterruptHandler h, void* arg);
void pinRemoveInterruptHandler( int pin );
#endif // PIN_NO_ISR

/**
  \defgroup IoIndices IO Indices
  Indices (0-63) for each of the processor's IO lines.  PA0-PA31 are represented
  by indices 0 - 31, PB0-PB31 by indices 32 - 63.
  \ingroup Io
  @{
*/

#define PIN_PA0  0 /**< Pin 0, Port A */
#define PIN_PA1  1 /**< Pin 1, Port A */
#define PIN_PA2  2 /**< Pin 2, Port A */
#define PIN_PA3  3 /**< Pin 3, Port A */
#define PIN_PA4  4 /**< Pin 4, Port A */
#define PIN_PA5  5 /**< Pin 5, Port A */
#define PIN_PA6  6 /**< Pin 6, Port A */
#define PIN_PA7  7 /**< Pin 7, Port A */
#define PIN_PA8  8 /**< Pin 8, Port A */
#define PIN_PA9  9 /**< Pin 9, Port A */
#define PIN_PA10 10 /**< Pin 10, Port A */
#define PIN_PA11 11 /**< Pin 11, Port A */
#define PIN_PA12 12 /**< Pin 12, Port A */
#define PIN_PA13 13 /**< Pin 113, Port A */
#define PIN_PA14 14 /**< Pin 14, Port A */
#define PIN_PA15 15 /**< Pin 15, Port A */
#define PIN_PA16 16 /**< Pin 16, Port A */
#define PIN_PA17 17 /**< Pin 17, Port A */
#define PIN_PA18 18 /**< Pin 18, Port A */
#define PIN_PA19 19 /**< Pin 19, Port A */
#define PIN_PA20 20 /**< Pin 20, Port A */
#define PIN_PA21 21 /**< Pin 21, Port A */
#define PIN_PA22 22 /**< Pin 22, Port A */
#define PIN_PA23 23 /**< Pin 23, Port A */
#define PIN_PA24 24 /**< Pin 24, Port A */
#define PIN_PA25 25 /**< Pin 25, Port A */
#define PIN_PA26 26 /**< Pin 26, Port A */
#define PIN_PA27 27 /**< Pin 27, Port A */
#define PIN_PA28 28 /**< Pin 28, Port A */
#define PIN_PA29 29 /**< Pin 29, Port A */
#define PIN_PA30 30 /**< Pin 30, Port A */
#define PIN_PA31 31 /**< Pin 31, Port A */

#define PIN_PB00 32 /**< Pin 0, Port B */
#define PIN_PB01 33 /**< Pin 1, Port B */
#define PIN_PB02 34 /**< Pin 2, Port B */
#define PIN_PB03 35 /**< Pin 3, Port B */
#define PIN_PB04 36 /**< Pin 4, Port B */
#define PIN_PB05 37 /**< Pin 5, Port B */
#define PIN_PB06 38 /**< Pin 6, Port B */
#define PIN_PB07 39 /**< Pin 7, Port B */
#define PIN_PB08 40 /**< Pin 8, Port B */
#define PIN_PB09 41 /**< Pin 9, Port B */
#define PIN_PB10 42 /**< Pin 10, Port B */
#define PIN_PB11 43 /**< Pin 11, Port B */
#define PIN_PB12 44 /**< Pin 12, Port B */
#define PIN_PB13 45 /**< Pin 13, Port B */
#define PIN_PB14 46 /**< Pin 14, Port B */
#define PIN_PB15 47 /**< Pin 15, Port B */
#define PIN_PB16 48 /**< Pin 16, Port B */
#define PIN_PB17 49 /**< Pin 17, Port B */
#define PIN_PB18 50 /**< Pin 18, Port B */
#define PIN_PB19 51 /**< Pin 19, Port B */
#define PIN_PB20 52 /**< Pin 20, Port B */
#define PIN_PB21 53 /**< Pin 21, Port B */
#define PIN_PB22 54/**< Pin 22, Port B */
#define PIN_PB23 55 /**< Pin 23, Port B */
#define PIN_PB24 56 /**< Pin 24, Port B */
#define PIN_PB25 57 /**< Pin 25, Port B */
#define PIN_PB26 58 /**< Pin 26, Port B */
#define PIN_PB27 59 /**< Pin 27, Port B */
#define PIN_PB28 60 /**< Pin 28, Port B */
#define PIN_PB29 61 /**< Pin 29, Port B */
#define PIN_PB30 62 /**< Pin 30, Port B */
#define PIN_PB31 63 /**< Pin 31, Port B */

/* @} */

/**
\defgroup IoBits IO Bits
  The values to use to create a mask to pass into any of the \b Bits style functions.  
  Your mask values need to be of type longlong since it needs to represent 64 bits, 
  for the 64 IO lines.

  \b Example

  \code
  longlong mymask = 0;
  mymask |= (PIN_PA00_BIT | PIN_PA03_BIT | PIN_PA11_BIT);
  // now disable the pullups for lines PA00, PA03, and PA11
  PIN_PullupDisableBits( mymask );
  \endcode
\ingroup Io
@{
*/

#define PIN_PA00_BIT 1LL<<0x00 /**< IO 0, Port A */
#define PIN_PA01_BIT 1LL<<0x01 /**< IO 1, Port A */
#define PIN_PA02_BIT 1LL<<0x02 /**< IO 2, Port A */
#define PIN_PA03_BIT 1LL<<0x03 /**< IO 3, Port A */
#define PIN_PA04_BIT 1LL<<0x04 /**< IO 4, Port A */
#define PIN_PA05_BIT 1LL<<0x05 /**< IO 5, Port A */
#define PIN_PA06_BIT 1LL<<0x06 /**< IO 6, Port A */
#define PIN_PA07_BIT 1LL<<0x07 /**< IO 7, Port A */
#define PIN_PA08_BIT 1LL<<0x08 /**< IO 8, Port A */
#define PIN_PA09_BIT 1LL<<0x09 /**< IO 9, Port A */
#define PIN_PA10_BIT 1LL<<0x0A /**< IO 10, Port A */
#define PIN_PA11_BIT 1LL<<0x0B /**< IO 11, Port A */
#define PIN_PA12_BIT 1LL<<0x0C /**< IO 12, Port A */
#define PIN_PA13_BIT 1LL<<0x0D /**< IO 13, Port A */
#define PIN_PA14_BIT 1LL<<0x0E /**< IO 14, Port A */
#define PIN_PA15_BIT 1LL<<0x0F /**< IO 15, Port A */
#define PIN_PA16_BIT 1LL<<0x10 /**< IO 16, Port A */
#define PIN_PA17_BIT 1LL<<0x11 /**< IO 17, Port A */
#define PIN_PA18_BIT 1LL<<0x12 /**< IO 18, Port A */
#define PIN_PA19_BIT 1LL<<0x13 /**< IO 19, Port A */
#define PIN_PA20_BIT 1LL<<0x14 /**< IO 20, Port A */
#define PIN_PA21_BIT 1LL<<0x15 /**< IO 21, Port A */
#define PIN_PA22_BIT 1LL<<0x16 /**< IO 22, Port A */
#define PIN_PA23_BIT 1LL<<0x17 /**< IO 23, Port A */
#define PIN_PA24_BIT 1LL<<0x18 /**< IO 24, Port A */
#define PIN_PA25_BIT 1LL<<0x19 /**< IO 25, Port A */
#define PIN_PA26_BIT 1LL<<0x1A /**< IO 26, Port A */
#define PIN_PA27_BIT 1LL<<0x1B /**< IO 27, Port A */
#define PIN_PA28_BIT 1LL<<0x1C /**< IO 28, Port A */
#define PIN_PA29_BIT 1LL<<0x1D /**< IO 29, Port A */
#define PIN_PA30_BIT 1LL<<0x1E /**< IO 30, Port A */
#define PIN_PA31_BIT 1LL<<0x1F /**< IO 31, Port A */

#define PIN_PB00_BIT 1LL<<0x20 /**< IO 0, Port B */
#define PIN_PB01_BIT 1LL<<0x21 /**< IO 1, Port B */
#define PIN_PB02_BIT 1LL<<0x22 /**< IO 2, Port B */
#define PIN_PB03_BIT 1LL<<0x23 /**< IO 3, Port B */
#define PIN_PB04_BIT 1LL<<0x24 /**< IO 4, Port B */
#define PIN_PB05_BIT 1LL<<0x25 /**< IO 5, Port B */
#define PIN_PB06_BIT 1LL<<0x26 /**< IO 6, Port B */
#define PIN_PB07_BIT 1LL<<0x27 /**< IO 7, Port B */
#define PIN_PB08_BIT 1LL<<0x28 /**< IO 8, Port B */
#define PIN_PB09_BIT 1LL<<0x29 /**< IO 9, Port B */
#define PIN_PB10_BIT 1LL<<0x2A /**< IO 10, Port B */
#define PIN_PB11_BIT 1LL<<0x2B /**< IO 11, Port B */
#define PIN_PB12_BIT 1LL<<0x2C /**< IO 12, Port B */
#define PIN_PB13_BIT 1LL<<0x2D /**< IO 13, Port B */
#define PIN_PB14_BIT 1LL<<0x2E /**< IO 14, Port B */
#define PIN_PB15_BIT 1LL<<0x2F /**< IO 15, Port B */
#define PIN_PB16_BIT 1LL<<0x30 /**< IO 16, Port B */
#define PIN_PB17_BIT 1LL<<0x31 /**< IO 17, Port B */
#define PIN_PB18_BIT 1LL<<0x32 /**< IO 18, Port B */
#define PIN_PB19_BIT 1LL<<0x33 /**< IO 19, Port B */
#define PIN_PB20_BIT 1LL<<0x34 /**< IO 20, Port B */
#define PIN_PB21_BIT 1LL<<0x35 /**< IO 21, Port B */
#define PIN_PB22_BIT 1LL<<0x36 /**< IO 22, Port B */
#define PIN_PB23_BIT 1LL<<0x37 /**< IO 23, Port B */
#define PIN_PB24_BIT 1LL<<0x38 /**< IO 24, Port B */
#define PIN_PB25_BIT 1LL<<0x39 /**< IO 25, Port B */
#define PIN_PB26_BIT 1LL<<0x3A /**< IO 26, Port B */
#define PIN_PB27_BIT 1LL<<0x3B /**< IO 27, Port B */
#define PIN_PB28_BIT 1LL<<0x3C /**< IO 28, Port B */
#define PIN_PB29_BIT 1LL<<0x3D /**< IO 29, Port B */
#define PIN_PB30_BIT 1LL<<0x3E /**< IO 30, Port B */
#define PIN_PB31_BIT 1LL<<0x3F /**< IO 31, Port B */

/** @} */

#endif

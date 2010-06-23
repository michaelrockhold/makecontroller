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

#include "config.h"
#ifdef MAKE_CTRL_USB

#include "usbserial.h"
#include "core.h"
#include "string.h"
#include <usb/device/core/USBDCallbacks.h>

#ifndef USBSER_NO_SLIP
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte
static int  usbserialWriteSlipIfFull(char** bufptr, char* buf, int timeout);
#endif // USBSER_NO_SLIP

static void usbserialOnTx(void *pArg, unsigned char status, unsigned int received, unsigned int remaining);

typedef struct UsbSerial_t {
  Semaphore txSemaphore;
  InputQueue inq;
  uint8_t inbuffer[USBSER_MAX_READ];
  int justWrote;
#ifndef USBSER_NO_SLIP
  char slipOutBuf[USBSER_MAX_WRITE];
#endif
} UsbSerial;

static UsbSerial usbSerial;

/**
  \defgroup usbserial USB Serial
  Virutal serial port USB communication.
  This allows the Make Controller look like a serial port to your desktop, which it can then easily
  open up, read and write to.

  \section Usage
  First call usbserialInit() and then you can read and write to it.

  \code
  usbserialInit();
  usbserialWrite("hello", 5); // write a little something

  char buffer[128];
  int got = usbserialRead(buffer, 128); // and read
  \endcode

  \section Drivers
  On OS X, the system driver is used - no external drivers are needed.
  An entry in \b /dev is created - similar to <b>/dev/cu.usbmodem.xxxx</b>.  It may be opened for reading and
  writing like a regular file using the standard POSIX open(), close(), read(), write() functions.

  On Windows, the first time the device is seen, it needs
  to be pointed to a .INF file containing additional information - the \b make_controller_kit.inf in
  the same directory as this file.  Once Windows sets this up, the device can be opened as a normal
  COM port.  If you've installed mchelper or mcbuilder, this file has already been installed in the
  right spot on your system.
  \ingroup interfacing
  @{
*/

static void onByteRx(void *pArg, char byte)
{
  UNUSED(pArg);
  chIQPutI(&usbSerial.inq, (uint8_t)byte);
}

/*
  Gets called whenever the input queue is being read from.
  If we're empty, go get some more data
*/
static void usbserialInotify(void)
{
  if (chIQIsEmpty(&usbSerial.inq) && usbserialIsActive()  )
    USBD_Read(CDCDSerialDriverDescriptors_DATAOUT, 0, sizeof(usbSerial.inq), 0, 0, onByteRx);
}

/**
  Initialize the USB serial system.
*/
void usbserialInit()
{
  chIQInit(&usbSerial.inq, usbSerial.inbuffer, sizeof(usbSerial.inbuffer), usbserialInotify);
  chSemInit(&usbSerial.txSemaphore, 0);
  USBD_Disconnect();
  CDCDSerialDriver_Initialize();
  USBD_Connect();
}

// called back from the central USB driver.
// reset things that clients might be blocking on.
void USBDCallbacks_Reset()
{
  chIQResetI(&usbSerial.inq);
  chSemResetI(&usbSerial.txSemaphore, 0);
}

/**
  Check if the USB system got set up OK.
  When things are starting up, if you want to wait until the USB is ready, 
  you can use this to check.  
  @return Whether the USB serial system is currently running.
  
  \b Example
  \code
  usbserialInit();
  // wait until usb is active
  while (usbserialIsActive() == NO) {
    sleep(10);
  }
  \endcode
*/
bool usbserialIsActive()
{
  return USBD_GetState() == USBD_STATE_CONFIGURED;
}

/**
  The number of bytes that have already arrived.
  Use this to determine if there's data already waiting to be read.  Note that
  if this is empty, you'll need to fire off usbserialRead() to fetch more data.
  @return The number of bytes waiting to be read.

  \b Example
  \code
  char data[28];
  int avail = usbserialAvailable();
  if (avail > 0) {
    usbserialRead(data, avail, 0); // we can read with no timeout
  }
  \endcode
*/
int usbserialAvailable()
{
  return chQSpace(&usbSerial.inq);
}

/**
  Read data from a USB host.
  This will wait up until the timeout for the specified number of bytes to arrive.
  If you need to get a byte at a time, see usbserialGet().
  @param buffer Where to store the incoming data.
  @param length How many bytes to read.
  @param timeout The number of milliseconds to wait if no data is available.  -1 means wait forever.
  @return The number of bytes successfully read.
  
  \b Example
  \code
  char mydata[128];
  int got = usbserialRead(mydata, 20);
  \endcode
*/
int usbserialRead(char *buffer, int length, int timeout)
{
  // if we're not connected, don't try to read more than has already arrived.
  if (!usbserialIsActive()) {
    if (chIQIsEmpty(&usbSerial.inq))
      return 0;
    if (length > chQSpace(&usbSerial.inq))
      length = chQSpace(&usbSerial.inq);
  }
  return chIQReadTimeout(&usbSerial.inq, (uint8_t*)buffer, length, MS2ST(timeout));
}

/**
  Get a character from the USB port.
  @return The character.
*/
char usbserialGet()
{
  if (!usbserialIsActive() && chIQIsEmpty(&usbSerial.inq))
    return 0;
  // TODO - would prefer to use chIQGetTimeout, but it doesn't trigger inotify currently.
  // should be fixed in chibios shortly
  char ch;
  chIQReadTimeout(&usbSerial.inq, (uint8_t*)&ch, 1, TIME_INFINITE);
  return ch;
}

/**
  Write a character to the USB port.
  @param c The character to write.
  @return 1 on success, -1 on failure.
*/
int usbserialPut(char c)
{
  char ch = c;
  return usbserialWrite(&ch, 1, TIME_INFINITE);
}

/**
  Write data to a USB host.
  @param buffer The data to send.
  @param length How many bytes to send.
  @param timeout How many milliseconds to wait for the data to be written out.
  @return The number of bytes successfully written.
  
  \b Example
  \code
  int written = usbserialWrite("hi hi", 5);
  \endcode
*/
int usbserialWrite(const char *buffer, int length, int timeout)
{
  int rv = -1;
  if (usbserialIsActive()) {
    if (USBD_Write(CDCDSerialDriverDescriptors_DATAIN,
          buffer, length, usbserialOnTx, 0) == USBD_STATUS_SUCCESS ) {
      if (chSemWaitTimeout(&usbSerial.txSemaphore, MS2ST(timeout)) == RDY_OK ) {
        rv = usbSerial.justWrote;
        usbSerial.justWrote = 0;
      }
    }
  }
  return rv;
}

// called back when data is actually transmitted
void usbserialOnTx(void *pArg, unsigned char status, unsigned int received, unsigned int remaining)
{
  UNUSED(pArg);
  if (status == USBD_STATUS_SUCCESS)
    usbSerial.justWrote += received;
  if (remaining == 0)
    chSemSignalI(&usbSerial.txSemaphore);
}

#ifndef USBSER_NO_SLIP
/**
  Read from the USB port using SLIP codes to de-packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from another 
  on an open serial connection.  This is the way OSC messages are sent over USB, for example.  
  
  SLIP uses a simple start/end byte and an escape byte in case your data actually 
  contains the start/end byte.  This function will not return until it has received a complete 
  SLIP encoded message, and will pass back the original message with the SLIP codes removed.

  Check the Wikipedia description of SLIP at http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol
  @param buffer Where to store the incoming data.
  @param length The number of bytes to read.
  @param timeout The number of milliseconds to wait for new data.
  @return The number of characters successfully read.
  @see read() for a similar example
*/
int usbserialReadSlip(char *buffer, int length, int timeout)
{
  UNUSED(timeout);
  int received = 0;

  while (received < length) {
    char c = usbserialGet();
    switch (c) {
      case END:
        if (received) // only return if we actually got anything
          return received;
        else
          break;
      case ESC:
        // get the next byte.  if it's not an ESC_END or ESC_ESC, it's a
        // malformed packet.  http://tools.ietf.org/html/rfc1055 says just
        // drop it in the packet in this case
        c = usbserialGet();
        if (c == ESC_END)
          c = END;
        else if (c == ESC_ESC)
          c = ESC;
        // no break here
      default:
        buffer[received++] = c;
        break;
    }
  }
  return CONTROLLER_ERROR_BAD_FORMAT; // error if we get here
}

/**
  Write to the USB port using SLIP codes to packetize messages.
  SLIP (Serial Line Internet Protocol) is a way to separate one "packet" from 
  another on an open serial connection.  This is the way OSC messages are sent over USB, 
  for example.  SLIP uses a simple start/end byte and an escape byte in case your data
  actually contains the start/end byte.  Pass your normal buffer to this function to
  have the SLIP codes inserted and then write it out over USB.

  Check the <A HREF="http://en.wikipedia.org/wiki/Serial_Line_Internet_Protocol">Wikipedia description</A>
  of SLIP for more info.
  @param buffer The data to write.
  @param length The number of bytes to write.
  @param timeout
  @return The number of characters successfully written.
  @see write() for a similar example.
*/
int usbserialWriteSlip(const char *buffer, int length, int timeout)
{
  char* obp = usbSerial.slipOutBuf;
  int count = 0;
  *obp++ = END; // clear out any line noise
   while (length--) {
     char c = *buffer++;
     switch (c) {
       // if it's the same code as an END character, we send a special 
       //two character code so as not to make the receiver think we sent an END
       case END:
         *obp++ = (char)ESC;
         count += usbserialWriteSlipIfFull(&obp, usbSerial.slipOutBuf, timeout);
         *obp++ = (char)ESC_END;
         count += usbserialWriteSlipIfFull(&obp, usbSerial.slipOutBuf, timeout);
         break;
         // if it's the same code as an ESC character, we send a special 
         //two character code so as not to make the receiver think we sent an ESC
       case ESC:
         *obp++ = (char)ESC;
         count += usbserialWriteSlipIfFull(&obp, usbSerial.slipOutBuf, timeout);
         *obp++ = (char)ESC_ESC;
         count += usbserialWriteSlipIfFull(&obp, usbSerial.slipOutBuf, timeout);
         break;
         //otherwise, just send the character
       default:
         *obp++ = c;
         count += usbserialWriteSlipIfFull(&obp, usbSerial.slipOutBuf, timeout);
     }
   }

   *obp++ = END; // end byte
   count += usbserialWrite(usbSerial.slipOutBuf, (obp - usbSerial.slipOutBuf), timeout);
   return count;
}

/** @}
*/

int usbserialWriteSlipIfFull(char** bufptr, char* buf, int timeout)
{
  int bufSize = *bufptr - buf;
  if (bufSize >= USBSER_MAX_WRITE) {
    *bufptr = buf;
    return usbserialWrite(buf, bufSize, timeout);
  }
  else
    return 0;
}
#endif // USBSER_NO_SLIP
#endif // MAKE_CTRL_USB



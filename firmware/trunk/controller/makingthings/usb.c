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

/** \file usb.c	
	USB Subsystem.
	Functions for controlling the USB port on the Make Controller Board.
*/

#include "config.h"
#ifdef MAKE_CTRL_USB

#include "usb.h"
#include "rtos.h"
#include "stdio.h"
#include "USB-CDC.h"
#include "queue.h"
#include "string.h"

#define MAX_INCOMING_SLIP_PACKET 400

extern xQueueHandle xTxCDC; 
extern xQueueHandle xRxCDC; 

static int Usb_Users;
// set by the running task when it gets started
int Usb_Running;

#define mainUSB_PRIORITY			( 4 )
#define mainUSB_TASK_STACK		( 600 )
#define TICKRATE 1000

void vUSBCDCTask( void *pvParameters );
int TestAndSend(xBULKBUFFER *pq, char newByte);

/** \defgroup Usb
	The Usb subsystem provides access to USB CDC functionality.  On Mac OS X, when this
  subsystem is running and plugged in, a new serial device is created.  The filename 
  that appears is similar to /dev/cu.usbmodem.xxxx.  It may be opened for reading and 
  writing like a regular file.  On Windows, the first time the device is seen, it needs 
  to be pointed to a .INF file containing additional information.  The INF file is in 
  the same directory as this file.  When this is set up, the device can be opened as a 
  COM: port.  See also the MC Helper codebase for code snippets.
  \todo expanded usb support to include emulation of other kinds of USB device - mouse, 
        keyboard, etc.
  \todo rename the INF file to make.inf
* \ingroup Controller
* @{
*/

/**
	Sets whether the Usb subsystem is active.
	@param state An integer specifying the active state of the Usb system - 1 (on) or 0 (off).
	@return Zero on success.
*/
int Usb_SetActive( int state )
{
  if ( state )
  {
    if ( Usb_Users++ == 0 )
    {
      // Create the USB task.
      TaskCreate(  vUSBCDCTask, "USB", mainUSB_TASK_STACK, NULL, mainUSB_PRIORITY );

      while ( !Usb_Running )
        Sleep( 100 );
    }
  }
  else
  {
    if ( Usb_Users )
    {
      if ( --Usb_Users == 0 )
      {
        /* kill the task, someday */
      }
    }
  }
  return CONTROLLER_OK;
}

/**
	Read the active state of the Usb system.
	@return State - 1 (on) or 0 (off).
*/
int Usb_GetActive( )
{
  return Usb_Users > 0;
}


/**	
	Get a character from the USB port.  If the USB subsystem is not already initialized,
  a call to this function will do so.  If this is the case, this first call may take a little
  while (1 second?) to return.  The return value is an integer to accomodate full 8 bit
  communication and still provide a mechanism to return "no character" - the -1.
	@param timeout an integer specifying, in milliseconds, how long to wait for a character before quitting
	@return the latest character from the USB port, or -1 if there was none
*/
int Usb_Read(char *buffer, int length)
{
  static xBULKBUFFER DeQueue;
  static int queueRemaining = 0; // whether or not there's anything left over from last time.
  static unsigned char *queuePtr = DeQueue.Data; // pointer into our block buffer
  char *bufferPtr = buffer; // pointer into the buffer passed in

  while( bufferPtr - buffer < length )
  {
    if( queueRemaining ) // first check if there's anything left from a previous read
    {
      if( queuePtr - DeQueue.Data < DeQueue.Count )
        *bufferPtr++ = *queuePtr++;
      else 
        queueRemaining = 0;
    }
    else // nothing left to read, so grab more from the queue
    {
      if ( xQueueReceive( xRxCDC, &DeQueue, 0 ) == pdTRUE  ) // got a new message from USB waiting on the queue
      {
        queueRemaining = 1;
        queuePtr = DeQueue.Data;
      }
      else // nothing was available so we're done
        break;
    }
  }
  return bufferPtr - buffer;
}

/**	
	Send a character out on the USB port.   If the USB subsystem is not already initialized,
  a call to this function will do so.  If this is the case, this first call may take a little
  while (1 second?) to return.
	@param c the character to send out
*/
int Usb_Write( char* buffer, int length )
{
  if ( !Usb_Users )
    Usb_SetActive( 1 );

  xBULKBUFFER q;
  q.Count = 0;
  int count = length;
  char *bp = buffer;

  while( count-- )
    TestAndSend( &q, *bp++ );

  // if bytes have been loaded into the buffer, but not sent because it's not full yet, send it now
  if( q.Count ) 
  {
    while(xQueueSend( xTxCDC, &q, 0) != pdPASS)
      Sleep( usbSHORTEST_DELAY );
    q.Count = 0;
  }
  
  return CONTROLLER_OK;
}

// the outgoing USB hardware can write a block of 64 (EP_FIFO) bytes.
// test the buffer's size and send it if we're full.
int TestAndSend(xBULKBUFFER *pq, char newByte)
{
  pq->Data[pq->Count++] = newByte;
  
  if(pq->Count == EP_FIFO) 
  { 
    while(xQueueSend( xTxCDC, pq, 0) != pdPASS)
      Sleep( usbSHORTEST_DELAY );
    pq->Count = 0; 
  }

  return pq->Count;
}


// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

int Usb_SlipSend( char* buffer, int length )
{
  char outgoingBuf[ length * 2 ];
  char *obp = outgoingBuf, *bp = buffer;
  int count = length;

  *obp++ = (char)END;

  while( count-- )
  {
    switch(*bp)
    {
      // if it's the same code as an END character, we send a special 
      //two character code so as not to make the receiver think we sent an END
      case END:
        *obp++ = (char) ESC;
        *obp++ = (char) ESC_END;
        break;
        // if it's the same code as an ESC character, we send a special 
        //two character code so as not to make the receiver think we sent an ESC
      case ESC:
        *obp++ = (char) ESC;
        *obp++ = (char) ESC_ESC;
        break;
        //otherwise, just send the character
      default:
        *obp++ = *bp;
    }
    bp++;
  }
  
  *obp++ = END; // tell the receiver that we're done sending the packet
  int sendLength = obp - outgoingBuf;
  Usb_Write( outgoingBuf, sendLength );
  
  return CONTROLLER_OK;
}

int Usb_SlipReceive( char* buffer, int length )
{
  int started = 0, count = 0, i;
  static char parseBuf[MAX_INCOMING_SLIP_PACKET], *pbp = parseBuf;
  static int bufRemaining = 0;
  char* bp = buffer;

  while ( count < length )
  {
    if( !bufRemaining ) // if there's nothing left over from last time
    {
      bufRemaining = Usb_Read( parseBuf, MAX_INCOMING_SLIP_PACKET );
      int test;
      if( bufRemaining > 26 )
        test = 0;
      pbp = parseBuf;
    }

    for( i = 0; i < bufRemaining; i++ )
    {
      switch( *pbp )
      {
        case END:
          if( started && count ) // it was the END byte
          {
            bufRemaining--; // decrement this here since we're returning
            return count; // We're done now if we had received any characters
          }
          else // skipping all starting END bytes
            started = true;
          break;
        case ESC:
          // if it's an ESC character, we just want to skip it and stick the next byte in the packet
          pbp++;  // no break here
        default:
          if( started )
          {
            *bp++ = *pbp;
            count++;
          }
          break;
      }
      pbp++;
      bufRemaining--;
    }
    Sleep(1);
  }
  return CONTROLLER_ERROR_BAD_FORMAT; // should never get here
}

/** @}
*/

#ifdef OSC

#include "osc.h"
#include "string.h"
#include "stdio.h"

// Need a list of property names
// MUST end in zero
static char* UsbOsc_Name = "usb";
static char* UsbOsc_PropertyNames[] = { "active", 0 }; // must have a trailing 0

int UsbOsc_PropertySet( int property, int value );
int UsbOsc_PropertyGet( int property );

// Returns the name of the subsystem
const char* UsbOsc_GetName( )
{
  return UsbOsc_Name;
}

// Now getting a message.  This is actually a part message, with the first
// part (the subsystem) already parsed off.
int UsbOsc_ReceiveMessage( int channel, char* message, int length )
{
  return Osc_IntReceiverHelper( channel, message, length, 
                                UsbOsc_Name,
                                UsbOsc_PropertySet, UsbOsc_PropertyGet, 
                                UsbOsc_PropertyNames );
}

// Set the index Usb, property with the value
int UsbOsc_PropertySet( int property, int value )
{
  switch ( property )
  {
    case 0: 
      Usb_SetActive( value );
      break;      
  }
  return CONTROLLER_OK;
}

// Get the index Usb, property
int UsbOsc_PropertyGet( int property )
{
  int value;
  switch ( property )
  {
    case 0:
      value = Usb_GetActive( );
      break;
  }
  
  return value;
}

#endif  // OSC

#endif // MAKE_CTRL_USB



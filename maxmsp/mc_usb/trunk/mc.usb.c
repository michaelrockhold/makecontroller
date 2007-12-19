/*********************************************************************************

 Copyright 2006-2007 MakingThings

 Licensed under the Apache License, 
 Version 2.0 (the "License"); you may not use this file except in compliance 
 with the License. You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0 
 
 Unless required by applicable law or agreed to in writing, software distributed
 under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied. See the License for
 the specific language governing permissions and limitations under the License.

*********************************************************************************/

/* mc.usb.c
*
* MakingThings 2006.
*/	

#include "ext.h"
#include "ext_common.h"
#include "ext_obex.h"

#include "mc_osc.h"
#include "mcError.h"
#include "usb_serial.h"


#define MAXSIZE 512
#define MAX_READ_LENGTH 16384

// SLIP codes
#define END             0300    // indicates end of packet 
#define ESC             0333    // indicates byte stuffing 
#define ESC_END         0334    // ESC ESC_END means END data byte 
#define ESC_ESC         0335    // ESC ESC_ESC means ESC data byte

// Max object structure def.
typedef struct _mcUsb
{
  t_object mcUsb_ob;
  void* obex;
  t_symbol *symval;
  //Max things
  void* mc_clock;
  long sampleperiod; //the sampleperiod attribute
  void *out0;
  //OSC things
  t_osc_packet* osc_packet;  //the current packet being created
  char* packetP;  //the pointer to the beginning of the packet
  t_osc_message* osc_message; // the message to be sent out into Max
  t_osc* Osc;  //the osc object
  int packetStarted;
  // USB things
  t_usbInterface* mc_usbInt;  // the USB interface
  char* usbReadBufPtr;
  char usbReadBuffer[ MAX_READ_LENGTH ];
	int usbReadBufLength;
} t_mcUsb;

void *mcUsb_class;  // global variable pointing to the class

void *mcUsb_new( t_symbol *s, long ac, t_atom *av );
void mcUsb_free(t_mcUsb *x);
void mcUsb_anything(t_mcUsb *x, t_symbol *s, short ac, t_atom *av);
void mcUsb_assist(t_mcUsb *x, void *b, long m, long a, char *s);
void mcUsb_tick( t_mcUsb *x );
mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length );
void mc_SLIP_receive( t_mcUsb *x );
void mcUsb_sampleperiod( t_mcUsb *x, long i );
void mcUsb_devicepath( t_mcUsb *x );

// define the object so Max knows all about it, and define which messages it will implement and respond to
int main( )
{
  t_class	*c;
	void	*attr;
	long	attrflags = 0;
	
	c = class_new("mcUsb", (method)mcUsb_new, (method)mcUsb_free, (short)sizeof(t_mcUsb), 0L, A_GIMME, 0);
	class_obexoffset_set(c, calcoffset(t_mcUsb, obex));  // register the byte offset of obex with the class
	
	//add some attributes
	attr = attr_offset_new("sampleperiod", gensym( "long" ), attrflags, 
		(method)0L, (method)0L, calcoffset(t_mcUsb, sampleperiod));
	class_addattr(c, attr);

	class_addmethod(c, (method)mcUsb_anything, "anything", A_GIMME, 0);
	class_addmethod(c, (method)mcUsb_assist, "assist", A_CANT, 0);
	class_addmethod(c, (method)mcUsb_sampleperiod, "sampleperiod", A_LONG, 0);
	class_addmethod(c, (method)mcUsb_devicepath, "devicepath", A_GIMME, 0);

	// add methods for dumpout and quickref	
	class_addmethod(c, (method)object_obex_dumpout, "dumpout", A_CANT, 0); 
	class_addmethod(c, (method)object_obex_quickref, "quickref", A_CANT, 0);

	// we want this class to instantiate inside of the Max UI; ergo CLASS_BOX
	class_register(CLASS_BOX, c);
	mcUsb_class = c;
	return 0;
}

// gets called by Max when the object receives a "message" in its left inlet
void mcUsb_anything(t_mcUsb *x, t_symbol *s, short ac, t_atom *av)
{
	if (ac > MAXSIZE)
		ac = MAXSIZE;
	
	if( Osc_create_message( x->Osc, s->s_name, ac, av ) == MC_OK )
		mc_send_packet( x, x->mc_usbInt, x->Osc->outBuffer, (OSC_MAX_MESSAGE - x->Osc->outBufferRemaining) );
}

// set the inlet and outlet assist messages when somebody hovers over them in an unlocked patcher
void mcUsb_assist(t_mcUsb *x, void *b, long msg, long arg, char *s)
{
	if ( msg == ASSIST_INLET )
	{
		switch (arg)
		{
			case 0: sprintf(s,"Outgoing data - OSC messages");
				break;
		}
	}
	else if( msg == ASSIST_OUTLET ) 
	{
		switch (arg)
		{
			case 0: sprintf(s,"Incoming data - OSC messages"); 
				break;
		}
	}
}

// function that gets called back by the clock.
void mcUsb_tick( t_mcUsb *x )
{
	mc_SLIP_receive( x );
	
	if( x->mc_usbInt->deviceOpen )
	  clock_delay( x->mc_clock, 1 ); //set the delay interval for the next tick
	else
	  clock_delay( x->mc_clock, 100 );
}

void mc_SLIP_receive( t_mcUsb *x )
{
  int started = 0, i;
  char *bufferPtr = x->osc_packet->packetBuf, *tempPtr;
	x->osc_packet->length = 0;

  if( !x->mc_usbInt->deviceOpen )
    usb_open( x->mc_usbInt );
  
  if( !x->mc_usbInt->deviceOpen ) // if we're still not open, bail
     return;
	
	while( 1 )
	{
		if( x->usbReadBufLength < 1 ) // if we don't have any unprocessed chars in our buffer, go read some more
		{
			int available = usb_numBytesAvailable( x->mc_usbInt );
			if( available > 0 )
			{
				int justGot = 0;
				if( available > MAX_READ_LENGTH )
					available = MAX_READ_LENGTH;
				justGot = usb_read( x->mc_usbInt, x->usbReadBuffer, available );
    		x->usbReadBufPtr = x->usbReadBuffer;
				x->usbReadBufLength += justGot;
    		if( x->usbReadBufLength < 0 )
				{
					post( "breaking..." );
					break;
				}
			}
		}

		for( i = 0; i < x->usbReadBufLength; i++ )
		{
			switch( (unsigned char)*x->usbReadBufPtr )
			{
				case END:
					if( started && x->osc_packet->length > 0 ) // it was the END byte
					{
						*bufferPtr = '\0';
						Osc_receive_packet( x->out0, x->Osc, x->osc_packet->packetBuf, x->osc_packet->length, x->osc_message );
						x->packetStarted = 0;
						return; // We're done now if we had received any characters
					}
					else // skipping all starting END bytes
					{
						started = true;
						x->osc_packet->length = 0;
					}
					break;					
				case ESC:
					// if it's the same code as an ESC character, we just want to skip it and 
					// stick the next byte in the packet
					x->usbReadBufPtr++;
					x->usbReadBufLength--;
					// no break here, just stick it in the packet		
				default:
					if( started )
					{
						*bufferPtr++ = *x->usbReadBufPtr;
						x->osc_packet->length++;
					}
			}
			x->usbReadBufPtr++;
			x->usbReadBufLength--;
		}
		if( x->usbReadBufLength == 0 ) // if we didn't get anything, sleep...otherwise just rip through again
			break;
		//justGot = 0; // reset our count for the next run through
	}
	return; //IO_ERROR; // should never get here
}


mcError mc_send_packet( t_mcUsb *x, t_usbInterface* u, char* packet, int length )
{
	  /*
	  // see if we can dispense with the bundle business
	  if ( outMessageCount == 1 )
	  {
	    // skip 8 bytes of "#bundle" and 8 bytes of timetag and 4 bytes of size
	    buffer += 20;
	    // shorter too
	    length -= 20;
	  }
	  */
		//packetInterface->sendPacket( buffer, size );

	//int totallength = length + 2;
	char *ptr;
	int size;
	char buf[ OSC_MAX_MESSAGE * 2 ]; // make it twice as long, as worst case scenario is ALL escape characters
	buf[0] = END;  // Flush out any spurious data that may have accumulated
	ptr = buf + 1; 
	size = length;

  while( size-- )
  {
    switch(*packet)
		{
			// if it's the same code as an END character, we send a special 
			//two character code so as not to make the receiver think we sent an END
			case END:
				*ptr++ = ESC;
				*ptr++ = ESC_END;
				break;
				
				// if it's the same code as an ESC character, we send a special 
				//two character code so as not to make the receiver think we sent an ESC
			case ESC:
				*ptr++ = ESC;
				*ptr++ = ESC_ESC;
				break;
				//otherwise, just send the character
			default:
				*ptr++ = *packet;
		}
		packet++;
	}
	
	// tell the receiver that we're done sending the packet
	*ptr++ = END;
	usb_write( u, buf, (ptr - buf) );
	Osc_resetOutBuffer( x->Osc );
	return 0;
}

// set how often the USB port should be read.
void mcUsb_sampleperiod( t_mcUsb *x, long i )
{
	if( i < 1 )
	  i = 1;
	object_attr_setlong( x, gensym("sampleperiod"), i );
}

// print out the device's file path, if it's connected, in response to a "filepath" message
void mcUsb_devicepath( t_mcUsb *x )
{
	if( x->mc_usbInt->deviceOpen )
	  post( "mc.usb is connected to a Make Controller at %s", x->mc_usbInt->deviceLocation );
	else
		post( "mc.usb is not currently connected to a Make Controller Kit." );
}

void mcUsb_free(t_mcUsb *x)
{
  freeobject( (t_object*)x->mc_clock );
  free( (t_osc_packet*)x->osc_packet );
  free( (t_osc*)x->Osc );
  free( (t_osc_message*)x->osc_message );
  usb_close( x->mc_usbInt );
}

// the constructor for the object
void *mcUsb_new( t_symbol *s, long ac, t_atom *av )
{
	t_mcUsb* new_mcUsb;
	//int i;
	cchar* usbConn = NULL;

	// we use object_alloc here, rather than newobject
	if( new_mcUsb = (t_mcUsb*)object_alloc(mcUsb_class) )
	{
		//object_obex_store( new_mcUsb, _sym_dumpout, outlet_new(new_mcUsb, NULL) ); // add a dumpout outlet
		new_mcUsb->out0 = outlet_new(new_mcUsb, 0L);  // add a left outlet
	}
	
	new_mcUsb->mc_clock = clock_new( new_mcUsb, (method)mcUsb_tick );
	new_mcUsb->sampleperiod = 1;  //interval at which to call the clock
	clock_delay( new_mcUsb->mc_clock, new_mcUsb->sampleperiod );  //set the clock running
	
	new_mcUsb->Osc = ( t_osc* )malloc( sizeof( t_osc ) );
	Osc_resetOutBuffer( new_mcUsb->Osc );
	
	new_mcUsb->osc_packet = ( t_osc_packet* )malloc( sizeof( t_osc_packet ));
	new_mcUsb->packetStarted = 0;
	new_mcUsb->packetP = NULL;
	
	new_mcUsb->osc_message = ( t_osc_message* )malloc( sizeof( t_osc_message ) );
	Osc_reset_message( new_mcUsb->osc_message );

	new_mcUsb->usbReadBufPtr = new_mcUsb->usbReadBuffer;
	new_mcUsb->usbReadBufLength = 0;
	
	new_mcUsb->mc_usbInt = usb_init( usbConn, &new_mcUsb->mc_usbInt );
	usb_open( new_mcUsb->mc_usbInt );
	
	return( new_mcUsb );
}


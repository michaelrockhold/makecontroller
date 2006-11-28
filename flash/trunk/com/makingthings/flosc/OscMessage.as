﻿/********************************************************************************* Copyright 2006 MakingThings Licensed under the Apache License,  Version 2.0 (the "License"); you may not use this file except in compliance  with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License. *********************************************************************************//** 	An OscMessage includes two properties, an \b address and a list of \b arguments.		\section Address	The address is of type \b String and can be accessed with the . operator.	<h3>Example</h3>	\code	var oscM = new OscMessage( );	oscM.address = "/appled/0/state";	\endcode		\section Arguments	The list of arguments is simply an \b Array named \b args.  The array can hold 	any kind of data type, but currently only \b Strings and \b Numbers are parsed.  	Access it with the . operator as well.	<h3>Example</h3>	\code	var oscM = new OscMessage( );	oscM.address = "/appled/0/state";	oscM.args[0] = "1";	\endcode		Or, you can intialize both the address and args values at the time you create the instance:	\code	var oscM = new OscMessage( "/appled/0/state", "1" );	\endcode		\section bundles OSC Bundles	You may also want to create a bundle of several messages to reduce the number of packets being sent.	A bundle is simply an \b Array filled with items of type OscMessage. To send them, use the sendBundle()	or sendBundleToAddress() functions.  		You never need to worry about unpacking a bundle, as this is already done for you.  You just	get the unpacked messages.  		<h3>Example</h3>	\code	var msg1 = new OscMessage( "/appled/1/state", "1" );	var msg2 = new OscMessage( "/servo/2/position", "512" );	var oscBundle = new Array( msg1, msg2 );	flosc.sendBundle( oscBundle );	// now both messages are sent in a single packet.	\endcode*/class com.makingthings.flosc.OscMessage{	var address:String;	var args:Array;	/**	Constructor - can be initialized with the address and a single argument.	Of course, you can 	*/	function OscMessage( address:String, arg )	{		if( address == undefined )			this.address = "";		else			this.address = address;					this.args = new Array( );		if( arg == undefined )			return;		else			this.args[0] = arg;	}		/**	Add an argument to an OscMessage.	The given argument is added to the array of arguments	*/	public function addArgument( arg )	{		if( arg != undefined )			this.args.push( arg );	}		/**	Get the number of arguments in an OscMessage.	/return A number specifying how many arguments are included in the OscMessage.	*/	public function numberOfArgs( ):Number	{		return this.args.length;	}		/**	Return a string representing an OscMessage - not implemented.	/return The OscMessage as a string.	This will look something like \verbatim "/address arg1 (arg2)..." \endverbatim	*/	public static function OscMessageToString( oscM:OscMessage ):String	{		return "";	}		/**	Get the address of an OscMessage.	\return The address of an OscMessage as a string.	*/	public function getAddress( ):String	{		return this.address;	}		/**	Set the address of an OscMessage.	\param addr A string representing the OSC address of an OscMessage.	If an address already exists for the given OscMessage, this will replace it.	*/	public function setAddress( addr:String )	{		this.address = addr;	}		/**	Reset and clear out an OscMessage for reuse.	*/	public function clear( )	{		this.address = "";		for( var i = 0; i < this.args.length; i++ )			this.args.pop();	}}
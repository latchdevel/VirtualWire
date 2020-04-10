# Virtual Wire
This is the VirtualWire "pure C" library for Arduino and other boards.
It provides a simple message passing protocol for a range of inexpensive transmitter and receiver modules.

## END OF LIFE NOTICE (2014)
This VirtualWire library has now been superceded by the RadioHead  library http://www.airspayce.com/mikem/arduino/RadioHead
RadioHead and its RH_ASK driver provides all the features supported by VirtualWire, and much more
besides, including Reliable Datagrams, Addressing, Routing and Meshes.
RH_ASK driver message format and software technology is based on VirtualWire library, with which it is compatible.
All the platforms that VirtualWire supported are also supported by RadioHead. 

This library will no longer be maintained or updated, but we will continue to publish
it for the benefit of the the community. Nevertheless we recommend upgrading to RadioHead where
possible.

### Installation
To install, unzip the library into the libraries sub-directory of your Arduino application directory. 
Then launch the Arduino environment; you should see the library in the Sketch->Import Library menu, and example code in
File->Sketchbook->Examples->VirtualWire menu.

To use the VirtualWire library, you must have:
```c
#include <VirtualWire.h>
```
at the top of your sketch.

## VirtualWire API

VirtualWire is a library for Arduino, Maple and others that provides features to send short messages, without addressing, retransmit or acknowledgment, a bit like UDP over wireless, using ASK (amplitude shift keying). 
Supports a number of inexpensive radio transmitters and receivers. All that is required is transmit data, receive data and (for transmitters, optionally) a PTT transmitter enable. 
Can also be used over various analog connections (not just a data radio), such as the audio channel of an A/V sender It is intended to be compatible with the RF Monolithics (www.rfm.com) Virtual Wire protocol, but this has not been tested.

Does not use the Arduino UART. Messages are sent with a training preamble, message length and checksum. Messages are sent with 4-to-6 bit encoding for good DC balance, and a CRC checksum for message integrity.

But why not just use a UART connected directly to the transmitter/receiver? As discussed in the RFM documentation, ASK receivers require a burst of training pulses to synchronize the transmitter and receiver, and also requires good balance between 0s and 1s in the message stream in order to maintain the DC balance of the message. 
UARTs do not provide these. They work a bit with ASK wireless, but not as well as this code.

### Implementation Details
Messages of up 67 bytes can be sent each message is transmitted as:

- 36 bit training preamble consisting of 0-1 bit pairs
- 12 bit start symbol 0xb38
- 1 byte of message length byte count (4 to 30), count includes byte count and FCS bytes
- n message bytes (uincluding 4 bytes of header), maximum n is MAX_MESSAGE_LEN + 4 (64)
- 2 bytes FCS, sent low byte-hi byte

The Arduino UNO clock rate is 16MHz => 62.5ns/cycle. For an RF bit rate of 2000 bps, need 500microsec bit period. 
The ramp requires 8 samples per bit period, so need 62.5microsec per sample => interrupt tick is 62.5microsec.

The maximum packet length consists of (6 + 2 + MAX_MESSAGE_LEN*2) * 6 = 768 bits = 0.384 secs (at 2000 bps). where MAX_MESSAGE_LEN is MAX_PAYLOAD_LEN - 7 (= 60). 
The code consists of an ISR interrupt handler. Most of the work is done in the interrupt handler for both transmit and receive, but some is done from the user level. 
Expensive functions like CRC computations are always done in the user level.

### Supported Hardware
A range of communications hardware is supported. The ones listed below are available in common retail outlets in Australia and other countries for under $10 per unit.  Many other modules may also work with this software. 

Runs on a wide range of Arduino processors using Arduino IDE 1.0 or later.
Runs on ATmega8/168 (Arduino Diecimila, Uno etc), ATmega328 and can run on almost any other AVR8 platform, without relying on the Arduino framework, by properly configuring the library using 'VirtualWire_Config.h' header file for describing the access to IO pins and for setting up the timer. From version 1.22 the library can be compiled by a C compiler (by renaming VirtualWire.cpp into VirtualWire.c) and can be easily integrated with other IDEs like 'Atme Studio' for example (courtesy of Alexandru Mircescu).

Also runs on on Energia with MSP430G2553 / G2452 and Arduino with ATMega328 (courtesy Yannick DEVOS - XV4Y),  but untested by us. It also runs on Teensy 3.0 and 3.1 (courtesy of Paul Stoffregen) using the Arduino IDE 1.0.5 and the Teensyduino addon 1.18. 
Also compiles and runs on ATtiny85 in Arduino environment, courtesy r4z0r7o3. 
Also compiles on maple-ide-v0.0.12, and runs on Maple, flymaple 1.1 etc. 

- Receivers
 - RX-B1 (433.92MHz) (also known as ST-RX04-ASK)
- Transmitters: 
 - TX-C1 (433.92MHz)
- Transceivers
 - DR3100 (433.92MHz)

For testing purposes you can connect 2 VirtualWire instances directly, by connecting pin 12 of one to 11 of the other and vice versa, like this for a duplex connection:
```
Arduino 1       wires          Arduino 2
 D11-----------------------------D12
 D12-----------------------------D11
 GND-----------------------------GND
```

You can also connect 2 VirtualWire instances over a suitable analog transmitter/receiver, such as the audio channel of an A/V transmitter/receiver. 
You may need buffers at each end of the connection to convert the 0-5V digital output to a suitable analog voltage.

**Caution:*** ATTiny85 has only 2 timers, one (timer 0) usually used for millis() and one (timer 1) for PWM analog outputs. 
The VirtualWire library, when built for ATTiny85, takes over timer 0, which prevents use of millis() etc but does permit analog outputs.

## Basic Usage
VirtualWire works somewhat differently than most Arduino libraries. Many individual functions are used, and their names are somewhat different. Fortunately, each one is simple.

### Configuration Functions
`vw_set_tx_pin(transmit_pin)`
Configure the transmit pin. Default is pin 12.

`vw_set_rx_pin(receive_pin)`
Configure the receive pin, Default is pin 11. On Teensy 2.0, pin 11 should not be used because most receiver modules can not work correctly with the orange LED on that pin.

`vw_set_ptt_pin(transmit_en_pin)`
Configure the transmit enable pin, or "push to talk". Default is pin 10.

`vw_set_ptt_inverted(true)`
Configure the "push to talk" polarity.

`vw_setup(2000)`
Begin using all settings and initialize the library. This is similar to the "begin" function of other libraries. All pins should be configured before calling this function.

### Transmission Functions
`vw_send(message, length)`
Transmit a message. "message" is an array of the bytes to send, and "length" is the number of bytes stored in the array. This function returns immediately and the message is sent slowly by an interrupt-based background process.

`vw_tx_active()`
Returns true if the message is being sent, or false if the transmitter is not active. You can use this after sending a message to test when it has finished being transmitted.

`vw_wait_tx()`
Wait for a message to be fully transmitted. Often the simplest approach is to call this after vw_send.

### Reception Functions
`vw_rx_start()`
Activate the receiver process. You must call this function before any reception can occur. An interrupt-based background process is started which monitors the reception of data.

`vw_have_message()`
Returns true if message has been received. This is similar to the "available" function of most other libraries.

`vw_wait_rx()`
Wait for a message to be received. This will only return when a message has been received, otherwise it will wait forever.

`vw_wait_rx_max(timeout_ms)`
Wait for a message, but give up after "timeout_ms". Returns true if a message was received, or false if the timeout period elapsed.

`vw_get_message(buf, &buflen))`
Read the last received message. This should be called only when a message is known to be received with any of the 3 functions above. "buf" is an array where the message is copied. "buflen" should have the array's maximum size upon input, and upon return the number of bytes actually copied is retured. The function itself returns true if the message was verified correct, or false if a message was received but appears to have been corrupted.

`vw_rx_stop()` 
Disable the receiver process.

## Trademarks

VirtualWire is a trademark of AirSpayce Pty Ltd. The VirtualWire mark was first used on April 20 2008 for
international trade, and is used only in relation to data communications hardware and software and related services.
It is not to be confused with any other similar marks covering other goods and services.

## Copyright

This software is Copyright (C) 2008-2014 Mike McCauley (mikem@airspayce.com).
Use is subject to license conditions. The main licensing options available are GPL V2 or Commercial.

### Open Source Licensing GPL V2

This is the appropriate option if you want to share the source code of your
application with everyone you distribute it to, and you also want to give them
the right to share who uses it. If you wish to use this software under Open
Source Licensing, you must contribute all your source code to the open source
community in accordance with the GPL Version 2 when your application is
distributed. See http://www.gnu.org/copyleft/gpl.html

### Commercial Licensing

This is the appropriate option if you are creating proprietary applications
and you are not prepared to distribute and share the source code of your
application. Contact info@airspayce.com for details.
// VirtualWire.h
//
// Virtual Wire implementation for Arduino and other boards
// Author: Mike McCauley (mikem@airspayce.com)
// Copyright (C) 2008 Mike McCauley
// v1.27 (2014)
//
#ifndef VirtualWire_h
#define VirtualWire_h

#include <stdint.h>
#include "VirtualWire_Config.h"

//	Currently supported platforms
#define VW_PLATFORM_ARDUINO 1
#define VW_PLATFORM_MSP430  2
#define VW_PLATFORM_STM32   3
#define VW_PLATFORM_GENERIC_AVR8 4
#define VW_PLATFORM_UNO32   5

//	Select platform automatically, if possible
#ifndef VW_PLATFORM
 #if defined(ARDUINO)
  #define VW_PLATFORM VW_PLATFORM_ARDUINO
 #elif defined(__MSP430G2452__) || defined(__MSP430G2553__)
  #define VW_PLATFORM VW_PLATFORM_MSP430
 #elif defined(MCU_STM32F103RE)
  #define VW_PLATFORM VW_PLATFORM_STM32
 #elif defined MPIDE
  #define VW_PLATFORM VW_PLATFORM_UNO32
 #else
  #error Platform not defined! 	
 #endif
#endif

#if (VW_PLATFORM == VW_PLATFORM_ARDUINO)
 #if (ARDUINO >= 100)
  #include <Arduino.h>
 #else
  #include <wiring.h>
#endif
#elif (VW_PLATFORM == VW_PLATFORM_MSP430)// LaunchPad specific
 #include "legacymsp430.h"
 #include "Energia.h"
#elif (VW_PLATFORM == VW_PLATFORM_STM32) // Maple etc
 #include <stdint.h>
 // Defines which timer to use on Maple
 #define MAPLE_TIMER 1
#elif (VW_PLATFORM == VW_PLATFORM_UNO32)
#elif (VW_PLATFORM != VW_PLATFORM_GENERIC_AVR8) 
	#error Platform unknown!
#endif

// These defs cause trouble on some versions of Arduino
#undef abs
#undef double
#undef round

#ifndef VW_MAX_MESSAGE_LEN 
/// Maximum number of bytes in a message, counting the byte count and FCS
	#define VW_MAX_MESSAGE_LEN 80
#endif //VW_MAX_MESSAGE_LEN 

#if !defined(VW_RX_SAMPLES_PER_BIT)
/// Number of samples per bit
	#define VW_RX_SAMPLES_PER_BIT 8
#endif //VW_RX_SAMPLES_PER_BIT  

/// The maximum payload length
#define VW_MAX_PAYLOAD VW_MAX_MESSAGE_LEN-3

/// The size of the receiver ramp. Ramp wraps modulo this number
#define VW_RX_RAMP_LEN 160

// Ramp adjustment parameters
// Standard is if a transition occurs before VW_RAMP_TRANSITION (80) in the ramp,
// the ramp is retarded by adding VW_RAMP_INC_RETARD (11)
// else by adding VW_RAMP_INC_ADVANCE (29)
// If there is no transition it is adjusted by VW_RAMP_INC (20)
/// Internal ramp adjustment parameter
#define VW_RAMP_INC (VW_RX_RAMP_LEN/VW_RX_SAMPLES_PER_BIT)
/// Internal ramp adjustment parameter
#define VW_RAMP_TRANSITION VW_RX_RAMP_LEN/2
/// Internal ramp adjustment parameter
#define VW_RAMP_ADJUST 9
/// Internal ramp adjustment parameter
#define VW_RAMP_INC_RETARD (VW_RAMP_INC-VW_RAMP_ADJUST)
/// Internal ramp adjustment parameter
#define VW_RAMP_INC_ADVANCE (VW_RAMP_INC+VW_RAMP_ADJUST)

/// Outgoing message bits grouped as 6-bit words
/// 36 alternating 1/0 bits, followed by 12 bits of start symbol
/// Followed immediately by the 4-6 bit encoded byte count, 
/// message buffer and 2 byte FCS
/// Each byte from the byte count on is translated into 2x6-bit words
/// Caution, each symbol is transmitted LSBit first, 
/// but each byte is transmitted high nybble first
#define VW_HEADER_LEN 8

// Cant really do this as a real C++ class, since we need to have 
// an ISR
#ifdef __cplusplus
extern "C"
{
#endif //__cplusplus

#if (VW_PLATFORM != VW_PLATFORM_GENERIC_AVR8 )
    // Set the digital IO pin which will be used to enable the transmitter (press to talk, PTT)'
    /// This pin will only be accessed if
    /// the transmitter is enabled
    /// \param[in] pin The Arduino pin number to enable the transmitter. Defaults to 10.
    extern void vw_set_ptt_pin(uint8_t pin);
    
    /// Set the digital IO pin to be for transmit data. 
    /// This pin will only be accessed if
    /// the transmitter is enabled
    /// \param[in] pin The Arduino pin number for transmitting data. Defaults to 12.
    extern void vw_set_tx_pin(uint8_t pin);

    /// Set the digital IO pin to be for receive data.
    /// This pin will only be accessed if
    /// the receiver is enabled
    /// \param[in] pin The Arduino pin number for receiving data. Defaults to 11.
    extern void vw_set_rx_pin(uint8_t pin);
#endif

    /// By default the RX pin is expected to be low when idle, and to pulse high 
    /// for each data pulse.
    /// This flag forces it to be inverted. This may be necessary if your transport medium
    /// inverts the logic of your signal, such as happens with some types of A/V tramsmitter.
    /// \param[in] inverted True to invert sense of receiver input
    extern void vw_set_rx_inverted(uint8_t inverted);

    /// By default the PTT pin goes high when the transmitter is enabled.
    /// This flag forces it low when the transmitter is enabled.
    /// \param[in] inverted True to invert PTT
    extern void vw_set_ptt_inverted(uint8_t inverted);

    /// Initialise the VirtualWire software, to operate at speed bits per second
    /// Call this one in your setup() after any vw_set_* calls
    /// Must call vw_rx_start() before you will get any messages
    /// \param[in] speed Desired speed in bits per second
    extern void vw_setup(uint16_t speed);

    /// Start the Phase Locked Loop listening to the receiver
    /// Must do this before you can receive any messages
    /// When a message is available (good checksum or not), vw_have_message();
    /// will return true.
    extern void vw_rx_start();

    /// Stop the Phase Locked Loop listening to the receiver
    /// No messages will be received until vw_rx_start() is called again
    /// Saves interrupt processing cycles
    extern void vw_rx_stop();

    /// Returns the state of the
    /// transmitter
    /// \return true if the transmitter is active else false
    extern uint8_t vw_tx_active();

    /// Block until the transmitter is idle
    /// then returns
    extern void vw_wait_tx();

    /// Block until a message is available
    /// then returns
    extern void vw_wait_rx();

    /// Block until a message is available or for a max time
    /// \param[in] milliseconds Maximum time to wait in milliseconds.
    /// \return true if a message is available, false if the wait timed out.
    extern uint8_t vw_wait_rx_max(unsigned long milliseconds);

    /// Send a message with the given length. Returns almost immediately,
    /// and message will be sent at the right timing by interrupts
    /// \param[in] buf Pointer to the data to transmit
    /// \param[in] len Number of octetes to transmit
    /// \return true if the message was accepted for transmission, false if the message is too long (>VW_MAX_MESSAGE_LEN - 3)
    extern uint8_t vw_send(uint8_t* buf, uint8_t len);

    /// Returns true if an unread message is available
    /// \return true if a message is available to read
    extern uint8_t vw_have_message();

    /// If a message is available (good checksum or not), copies
    /// up to *len octets to buf.
    /// \param[in] buf Pointer to location to save the read data (must be at least *len bytes.
    /// \param[in,out] len Available space in buf. Will be set to the actual number of octets read
    /// \return true if there was a message and the checksum was good
    extern uint8_t vw_get_message(uint8_t* buf, uint8_t* len);

    /// Returns the count of good messages received
    /// Caution,: this is an 8 bit count and can easily overflow
    /// \return Count of good messages received
    extern uint8_t vw_get_rx_good();

    /// Returns the count of bad messages received, ie
    /// messages with bogus lengths, indicating corruption
    /// or lost octets.
    /// Caution,: this is an 8 bit count and can easily overflow
    /// \return Count of bad messages received
    extern uint8_t vw_get_rx_bad();

#ifdef __cplusplus
} //	extern "C"
#endif //__cplusplus

#endif /* VirtualWire_h */

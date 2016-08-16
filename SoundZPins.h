#ifndef SoundZPins_h
#define SoundZPins_h

#include <ArduinoPins.h>


//------------------------------------------------------------------------------
// DAC pin definitions

// LDAC may be connected to ground to save a pin
/** Set USE_MCP_DAC_LDAC to 0 if LDAC is grounded. */
#define USE_MCP_DAC_LDAC 1

// Mighty/Sanguino
#if defined(__AVR_ATmega1284P__) || defined(__AVR_ATmega644P__)

//Serial Flush pin definitions

//SPI pin definitions
/** SPI slave select pin.*/
#define SS  SS_PINZ
/** SPI master output, slave input pin. */
#define MOSI MOSI_PINZ
/** SPI master input, slave output pin. */
#define MISO MISO_PINZ
/** SPI serial clock pin. */
#define SCK  SCK_PINZ

// use sanguino pins 0, 1, 2, 3 for DAC

// pin 0 is DAC chip select
/** Data direction register for DAC chip select. */
#define MCP_DAC_CS_DDR  PIN0_DDRREG
/** Port register for DAC chip select. */
#define MCP_DAC_CS_PORT PIN0_PORTREG
/** Port bit number for DAC chip select. */
#define MCP_DAC_CS_BIT  PIN0_BITNUM

/** Data direction register for DAC clock. */
#define MCP_DAC_SCK_DDR  PIN22_DDRREG
/** Port register for DAC clock. */
#define MCP_DAC_SCK_PORT PIN22_PORTREG
/** Port bit number for DAC clock. */
#define MCP_DAC_SCK_BIT  PIN22_BITNUM
// pin 2 is DAC serial data in
/** Data direction register for DAC serial in. */
#define MCP_DAC_SDI_DDR  PIN21_DDRREG
/** Port register for DAC clock. */
#define MCP_DAC_SDI_PORT PIN21_PORTREG
/** Port bit number for DAC clock. */
#define MCP_DAC_SDI_BIT  PIN21_BITNUM


// pin 3 is LDAC if used
#if USE_MCP_DAC_LDAC
/** Data direction register for Latch DAC Input. */
#define MCP_DAC_LDAC_DDR  PIN3_DDRREG
/** Port register for Latch DAC Input. */
#define MCP_DAC_LDAC_PORT PIN3_PORTREG
/** Port bit number for Latch DAC Input. */
#define MCP_DAC_LDAC_BIT  PIN3_BITNUM

#endif // USE_MCP_DAC_LDAC

#else // 168, 328, 1280 Arduinos

// use arduino pins 2, 3, 4, 5 for DAC

// pin 2 is DAC chip select
/** Data direction register for DAC chip select. */
#define MCP_DAC_CS_DDR  PIN2_DDRREG
/** Port register for DAC chip select. */
#define MCP_DAC_CS_PORT PIN2_PORTREG
/** Port bit number for DAC chip select. */
#define MCP_DAC_CS_BIT  PIN2_BITNUM

// pin 3 is DAC serial clock
/** Data direction register for DAC clock. */
#define MCP_DAC_SCK_DDR  PIN9_DDRREG
/** Port register for DAC clock. */
#define MCP_DAC_SCK_PORT PIN9_PORTREG
/** Port bit number for DAC clock. */
#define MCP_DAC_SCK_BIT  PIN9_BITNUM

// pin 4 is DAC serial data in
/** Data direction register for DAC serial in. */
#define MCP_DAC_SDI_DDR  PIN4_DDRREG
/** Port register for DAC clock. */
#define MCP_DAC_SDI_PORT PIN4_PORTREG
/** Port bit number for DAC clock. */
#define MCP_DAC_SDI_BIT  PIN4_BITNUM

// pin 5 is LDAC if used
#if USE_MCP_DAC_LDAC
/** Data direction register for Latch DAC Input. */
#define MCP_DAC_LDAC_DDR  PIN8_DDRREG
/** Port register for Latch DAC Input. */
#define MCP_DAC_LDAC_PORT PIN8_PORTREG
/** Port bit number for Latch DAC Input. */
#define MCP_DAC_LDAC_BIT  PIN8_BITNUM

#endif // USE_MCP_DAC_LDAC

#endif // DAC pin definitions

#endif // SoundZPins_h

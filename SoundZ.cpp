/*
	"SoundZ" is an audio library for playing back uncompressed WAV files from SPI Flash with Arudino.
	It's based on WaveHC library by Adafruit (and William Greiman) and modified by Lensdigital to read files
	from SPI Flash instead of SD card. 
	It requires SerialFlash library.
	(2016) Lensdigital.com
	 ==================================================================================
	 License
	 ==================================================================================
	 This program is free software; you can redistribute it 
	 and/or modify it under the terms of the GNU General    
	 Public License as published by the Free Software       
	 Foundation; either version 3 of the License, or        
	 (at your option) any later version.                    
															
	 This program is distributed in the hope that it will   
	 be useful, but WITHOUT ANY WARRANTY; without even the  
	 implied warranty of MERCHANTABILITY or FITNESS FOR A   
	 PARTICULAR PURPOSE. See the GNU General Public        
	 License for more details.                              
															
	 You should have received a copy of the GNU General    
	 Public License along with this program.
	 If not, see <http:www.gnu.org/licenses/>.
															
	 Licence can be viewed at                               
	 http:www.gnu.org/licenses/gpl-3.0.txt

	 Please maintain this license information along with authorship
	 and copyright notices in any redistribution of this code
	 ==================================================================================
*/
#include <string.h>
#include <avr/interrupt.h>
#include <mcpDac.h>
#include "SoundZ.h"
#include "SoundZUtil.h"

// verify program assumptions
#if PLAYBUFFLEN != 256 && PLAYBUFFLEN != 512
#error PLAYBUFFLEN must be 256 or 512
#endif // PLAYBUFFLEN

WaveHC *playing = 0;

uint8_t buffer1[PLAYBUFFLEN];
uint8_t buffer2[PLAYBUFFLEN];
uint8_t *playend;      // end position for current buffer
uint8_t *playpos;      // position of next sample
uint8_t *sdbuff;       // SerialFlash (SF) fill buffer
uint8_t *sdend;        // end of data in SF buffer

// status of SerialFlash (SF)
#define SD_READY 1     // buffer is ready to be played
#define SD_FILLING 2   // buffer is being filled from DS
#define SD_END_FILE 3  // reached end of file
uint8_t sfstatus = 0;

//=================================
// timer interrupt for DAC
//=================================
ISR(TIMER1_COMPA_vect) {
  if (!playing) return;

  if (playpos >= playend) {
    if (sfstatus == SD_READY) {
    
      // swap double buffers
      playpos = sdbuff;
      playend = sdend;
      sdbuff = sdbuff != buffer1 ? buffer1 : buffer2;
      
      sfstatus = SD_FILLING;
      // interrupt to call SD reader
	    TIMSK1 |= _BV(OCIE1B);
    }
    else if (sfstatus == SD_END_FILE) {
      playing->stop();
	  
      return;
    }
    else {
      // count overrun error if not at end of file
      if (playing->remainingBytesInChunk) {
        playing->errors++;
      }
      return;
    }
  }

  uint8_t dh, dl;
  if (playing->BitsPerSample == 16) {
  
    // 16-bit is signed
    dh = 0X80 ^ playpos[1];
    dl = playpos[0];
    playpos += 2;
  }
  else {
  
    // 8-bit is unsigned
    dh = playpos[0];
    dl = 0;
    playpos++;
  }
  
#if DVOLUME
  uint16_t tmp = (dh << 8) | dl;
  tmp >>= playing->volume;
  dh = tmp >> 8;
  dl = tmp;
#endif //DVOLUME

  // dac chip select low
  mcpDacCsLow();
  
  // send DAC config bits
  mcpDacSdiLow();
  mcpDacSckPulse();  // DAC A
  mcpDacSckPulse();  // unbuffered
  mcpDacSdiHigh();
  mcpDacSckPulse();  // 1X gain
  mcpDacSckPulse();  // no SHDN
  
  // send high 8 bits
  mcpDacSendBit(dh,  7);
  mcpDacSendBit(dh,  6);
  mcpDacSendBit(dh,  5);
  mcpDacSendBit(dh,  4);
  mcpDacSendBit(dh,  3);
  mcpDacSendBit(dh,  2);
  mcpDacSendBit(dh,  1);
  mcpDacSendBit(dh,  0);
  
  // send low 4 bits
  mcpDacSendBit(dl,  7);
  mcpDacSendBit(dl,  6);
  mcpDacSendBit(dl,  5);
  mcpDacSendBit(dl,  4);
  
  // chip select high - done
  mcpDacCsHigh();

}

//=========================================================
// this is the interrupt that fills the playbuffer
//=========================================================

ISR(TIMER1_COMPB_vect) {

  // turn off calling interrupt
  TIMSK1 &= ~_BV(OCIE1B);
  
  if (sfstatus != SD_FILLING) return;
  // enable interrupts while reading the SD
  sei();
  
  int16_t read = playing->readWaveData(sdbuff, PLAYBUFFLEN);
  
  cli();
  if (read > 0) {
    sdend = sdbuff + read;
    sfstatus = SD_READY;
  }
  else {
    sdend = sdbuff;
    sfstatus = SD_END_FILE;
	//putstring_nl(" endOfFile"); // Debug
	
  }
}

//=================================
// create an instance of WaveHC. 
WaveHC::WaveHC(void) {
  fd = 0;
}
//=================================

//uint8_t WaveHC::create(SerialFlash &f) {
uint8_t WaveHC::create(SerialFlashFile &f) {
 fd =0; // Init pointer to file object (MUST!)
 //Serial.print ("Size ");
 // Serial.println (f.size());
  // 18 byte buffer
  // can use this since Arduino and RIFF are Little Endian
  union {
    struct {
      char     id[4];
      uint32_t size;
      char     data[4];
    } riff;  // riff chunk
    struct {
      uint16_t compress;
      uint16_t channels;
      uint32_t sampleRate;
      uint32_t bytesPerSecond;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
      uint16_t extraBytes;
    } fmt; // fmt data
  } buf;

    // must start with WAVE header
  if (f.read(&buf, 12) != 12
      || strncmp(buf.riff.id, "RIFF", 4)
      || strncmp(buf.riff.data, "WAVE", 4)) {
        return false;
  }
  
  // next chunk must be fmt
  if (f.read(&buf, 8) != 8
      || strncmp(buf.riff.id, "fmt ", 4)) {
        return false;
  }
  
  // fmt chunk size must be 16 or 18
  uint16_t size = buf.riff.size;
  if (size == 16 || size == 18) {
    if (f.read(&buf, size) != (int16_t)size) {
      return false;
    }
  }
  else {
    // compressed data - force error
    buf.fmt.compress = 0;
  }
  
  if (buf.fmt.compress != 1 || (size == 18 && buf.fmt.extraBytes != 0)) {
    putstring_nl("Compression not supported");
    return false;
  }
  
  Channels = buf.fmt.channels;
  if (Channels > 2) {
    putstring_nl("Not mono/stereo!");
    return false;
  }
  else if (Channels > 1) {
    putstring_nl(" Warning stereo file!");
  }
  
  BitsPerSample = buf.fmt.bitsPerSample;
  if (BitsPerSample > 16) {
    putstring_nl("More than 16 bits per sample!");
    return false;
  }
  
  dwSamplesPerSec = buf.fmt.sampleRate;
  uint32_t clockRate = dwSamplesPerSec*Channels;
  uint32_t byteRate = clockRate*BitsPerSample/8;

 #if RATE_ERROR_LEVEL > 0
  if (clockRate > MAX_CLOCK_RATE
      || byteRate > MAX_BYTE_RATE) {
    putstring_nl("Sample rate too high!");
    if (RATE_ERROR_LEVEL > 1) {
      return false;
    }
  }
 #endif // RATE_ERROR_LEVEL > 0
 
 fd = &f;
 
  errors = 0;
  isplaying = 0;
  remainingBytesInChunk = 0;
  
  //putstring_nl("File validated");

#if DVOLUME
volume = 0;
#endif //DVOLUME 
  // position to data
  return readWaveData(0, 0) < 0 ? false: true;

}

//============================================================================
// Returns true if the player is paused else false.
// ============================================================================
uint8_t WaveHC::isPaused(void) {
  cli();
  uint8_t rtn = isplaying && !(TIMSK1 & _BV(OCIE1A));
  sei();
  return rtn;
}
//============================================================================
// Pause the player (not working with SerialFlash yet)
//============================================================================
void WaveHC::pause(void) {
  cli();
  TIMSK1 &= ~_BV(OCIE1A); //disable DAC interrupt
  sei();
  //fd->volume()->rawDevice()->readEnd(); // redo any partial read on resume (LD: Need to Fix for SerialFlash)
}
//============================================================================
/**
 * Play a wave file.
 *
 * WaveHC::create() must be called before a file can be played.
 *
 * Check the member variable WaveHC::isplaying to monitor the status
 * of the player.
 */
//============================================================================ 
void WaveHC::play(void) {
  // setup the interrupt as necessary
  // putstring_nl("Starting playback...");
  int16_t read;

  playing = this;

  // fill the play buffer
  read = readWaveData(buffer1, PLAYBUFFLEN);
  if (read <= 0) return;
  playpos = buffer1;
  playend = buffer1 + read;
  
 
  // fill the second buffer
  read = readWaveData(buffer2, PLAYBUFFLEN);
  if (read < 0) return;
  sdbuff = buffer2;
  sdend = sdbuff + read;
  sfstatus = SD_READY;
  
  // its official!
  isplaying = 1;
  
  // Setup mode for DAC ports
  mcpDacInit();
  
  // Set up timer one
  // Normal operation - no pwm not connected to pins
  TCCR1A = 0;
  // no prescaling, CTC mode
  TCCR1B = _BV(WGM12) | _BV(CS10); 
  // Sample rate - play stereo interleaved
  OCR1A =  F_CPU / (dwSamplesPerSec*Channels);
  // SD fill interrupt happens at TCNT1 == 1
  OCR1B = 1;
  // Enable timer interrupt for DAC ISR
  TIMSK1 |= _BV(OCIE1A);
}

//============================================================================
// Resume a paused player. 
//============================================================================
void WaveHC::resume(void) {
  cli();
  // enable DAC interrupt
  if(isplaying) TIMSK1 |= _BV(OCIE1A);
  sei();
}
//============================================================================
/**
 * Reposition a wave file. (Not workign yet with SerialFlush)
 *
 * \param[in] pos seek will attempt to position the file near \a pos.
 * \a pos is the byte number from the beginning of file.
*/
//============================================================================
void WaveHC::seek(uint32_t pos) {
  // make sure buffer fill interrupt doesn't happen
  cli();
  if (fd) {
    pos -= pos % PLAYBUFFLEN;
    if (pos < PLAYBUFFLEN) pos = PLAYBUFFLEN; //don't play metadata
    uint32_t maxPos = fd->position() + remainingBytesInChunk;
    if (maxPos > fd->size()) maxPos = fd->size();
    if (pos > maxPos) pos = maxPos;
    //if (fd->seekSet(pos)) {  // Need to fix for Serial Flash
      // assumes a lot about the wave file
     // remainingBytesInChunk = maxPos - pos;
    //}
  }
  sei();
}

 
//============================================================================
/** Set the player's sample rate.
 *
 * \param[in] samplerate The new sample rate in samples per second.
 * No checks are done on the input parameter.
 */
//============================================================================
void WaveHC::setSampleRate(uint32_t samplerate) {
  if (samplerate < 500) samplerate = 500;
  if (samplerate > 50000) samplerate = 50000;
  // from ladayada's library.
  cli();
  while (TCNT0 != 0);
  
  OCR1A = F_CPU / samplerate;
  sei();
}
//============================================================================
/** Stop the player. */
void WaveHC::stop(void) {
  TIMSK1 &= ~_BV(OCIE1A);   // turn off interrupt
  playing->isplaying = 0;
  playing = 0;
}
//============================================================================
// Read wave data.
//
// Not for use in applications.  Must be public so SF read ISR can access it.
//============================================================================
int16_t WaveHC::readWaveData(uint8_t *buff, uint16_t len) {
	//Serial.println (fd->size()); // Debug
  if (remainingBytesInChunk == 0) {
    struct {
      char     id[4];
      uint32_t size;
    } header;
    while (1) {
      if (fd->read(&header, 8) != 8) return -1;
      if (!strncmp(header.id, "data", 4)) {
        remainingBytesInChunk = header.size;
        break;
      }
	 // if not "data" then skip it! // LD: Doesn't work w/ SerialFlash, but if check is not really needed. Instead just do seek to skip header.
      //if (!fd->seekCur(header.size)) {
       // return -1;
     // }
	  fd->seek(header.size); // Skips header and prevents burst of noise in playback of some files
    }
	
  }

  // make sure buffers are aligned on SD sectors *TODO Convert to SerialFlash
  //uint16_t maxLen = PLAYBUFFLEN - fd->readPosition() % PLAYBUFFLEN;
  //if (len > maxLen) len = maxLen;

  if (len > remainingBytesInChunk) {
    len = remainingBytesInChunk;
  }
  
  int16_t ret = fd->read(buff, len);
  if (ret > 0) remainingBytesInChunk -= ret;
  return ret;
}

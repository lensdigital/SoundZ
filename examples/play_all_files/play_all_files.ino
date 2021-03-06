// ======================================================
// SoundZ example: Plays back all files on Flash IC once.
// by Lensdigital
// ======================================================

#include <SoundZ.h>
#include <SerialFlash.h>
#include <SPI.h>

#define FLASH_CS 20 //Pin connected to SPI Flash chip select

SerialFlashFile f; //Instance for current file
WaveHC wave; 

void setup() {
  Serial.begin (115200);
  if (!SerialFlash.begin(FLASH_CS)) {
    Serial.println (F("Unable to access SPI Flash chip"));
  }
  SerialFlash.opendir();
  while (1) {
    char filename[64];
    uint32_t filesize;

    if (SerialFlash.readdir(filename, sizeof(filename), filesize)) {
      Serial.print("Playing: ");
      Serial.print(filename);
      //spaces(20 - strlen(filename));
      for (int i=0; i < (20 - strlen(filename) ); i++) {Serial.print(" ");}
      Serial.print("  ");
      Serial.print(filesize);
      Serial.print(" bytes");
      Serial.println();
      playcomplete (filename);
    } else {
      break; // no more files
    }
  }
}

void loop() {

}

// =======================================================================================
// ---- Plays a full file from beginning to end with no pause.   ----
// =======================================================================================
// 
void playcomplete(char *name) {
  //Serial.println (F("Start playback"));
  playfile(name);
  while (wave.isplaying) {
  // do nothing while its playing
  }
  // now its done playing
   //Serial.println (F("End playback"));
}
// =======================================================================================
// ---- Plays a file interrupting previous playback. Doesn't stop rest of program   ----
// =======================================================================================
void playfile(char *name) {
  char *tmp; //Used for UPPERCASE conversion
  for (tmp=name;*tmp!='\0';tmp++) *tmp = (char) toupper(*tmp); // Convert text to UPPERCASE
  f=SerialFlash.open (name);
  if (wave.isplaying) {// already playing something, so stop it!
    Serial.print ("Already playing file");
    wave.stop(); // stop it
  }
  if (!f) {
    Serial.print (F("Error opening file ")); Serial.println (name);
    return;
  }
   if (!wave.create(f)) {
    Serial.println (F("Not a valid WAV"));
   }
   wave.play();
}




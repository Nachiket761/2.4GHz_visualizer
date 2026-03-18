//------------------------------------------Visualizer Part-------------------------------------------------//

#include <SPI.h>
#include "RF24.h"
#include <FastLED.h>

// --- LED Matrix Settings ---
#define LED_PIN     6
#define NUM_LEDS    128     // Dual matrices
#define BRIGHTNESS  30      
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

CRGB leds[NUM_LEDS];

// --- Radio Settings ---
RF24 radio(7, 8);
const int num_channels = 126; 

void setup() {
  Serial.begin(115200);
  
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  FastLED.show();

  if (!radio.begin()) {
    Serial.println("ERROR: Radio not responding!");
    while (1); 
  }
  
  radio.setAutoAck(false);
  radio.setPALevel(RF24_PA_MAX); 
  radio.setDataRate(RF24_1MBPS);
  radio.disableCRC();
  
  radio.startListening();
  radio.stopListening();
  Serial.println("Color-Coded Spectrum Analyzer Online.");
}

void loop() {
  // Fade the radar tail
  fadeToBlackBy(leds, NUM_LEDS, 35);

  // Sweep the spectrum
  for (int i = 0; i < num_channels; i++) {
    radio.setChannel(i);
    radio.startListening();
    delayMicroseconds(130); 
    radio.stopListening();
    
    // If we intercept a radio wave...
    if (radio.testCarrier()) {
      
      // --- APPLY THE COLOR MAPPING ---
      
      // 1. Bluetooth Low Energy Beacons (Channels 2, 26, 80)
      if (i == 2 || i == 26 || i == 80) {
        leds[i] = CRGB::White; 
      }
      // 2. Wi-Fi Channel 1 Zone (Blue)
      else if (i >= 1 && i <= 23) {
        leds[i] = CRGB::Blue;
      }
      // 3. Wi-Fi Channel 6 Zone (Green)
      else if (i >= 27 && i <= 48) {
        leds[i] = CRGB::Green;
      }
      // 4. Wi-Fi Channel 11 Zone (Red)
      else if (i >= 51 && i <= 73) {
        leds[i] = CRGB::Red;
      }
      // 5. Unknown Background Noise / Other Protocols
      else {
        // A dim purple for cosmic background radiation
        leds[i] = CHSV(190, 255, 100); 
      }
      
    }
  }

  // Push the colors to the dual-matrix array
  FastLED.show();
}

//--------------------------------------------------------------------------------------------------------------------------------//

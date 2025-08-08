#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Bounce2.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Constant Debounce
const int debounceDelayMs = 50;

// Button pins
const int button1Pin = 18;
const int button2Pin = 19;
const int resetPin   = 23;

// Debouncers
Bounce debouncer1 = Bounce();
Bounce debouncer2 = Bounce();
Bounce resetDebouncer = Bounce();

// Timing variables for each button (absolute timestamps)
unsigned long press1Time = 0;
unsigned long prevPress1Time = 0;
unsigned long fastest1 = ULONG_MAX;

unsigned long press2Time = 0;
unsigned long prevPress2Time = 0;
unsigned long fastest2 = ULONG_MAX;

// Sound
const int speakerPin = 16; // Use any PWM-capable pin
bool revving = false;
unsigned long lastUpdateSound = 0;
int currentFreq = 200;
const int targetFreq = 1000;
const int freqStep = 20;
const int stepDelay = 15; // ms

// Happy sound melody (frequencies in Hz)
const int happyMelody[] = {523, 659, 783, 1047}; // C5, E5, G5, C6
const int happyDurations[] = {150, 150, 150, 300}; // ms per note
const int happyNotesCount = sizeof(happyMelody) / sizeof(happyMelody[0]);

bool playingHappySound = false;
int happyNoteIndex = 0;
unsigned long happyNoteStartTime = 0;



static unsigned long lastUpdateLCD = 0;
bool runningAnimation = true;

void setup() {
  Serial.begin(115200);

  display.begin(0x3C, true);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.display();

  // Setup buttons with pullups
  pinMode(button1Pin, INPUT_PULLUP);
  pinMode(button2Pin, INPUT_PULLUP);
  pinMode(resetPin, INPUT_PULLUP);

  debouncer1.attach(button1Pin);
  debouncer1.interval(debounceDelayMs);

  debouncer2.attach(button2Pin);
  debouncer2.interval(debounceDelayMs);

  resetDebouncer.attach(resetPin);
  resetDebouncer.interval(debounceDelayMs);
  resetTimer();
}

void loop() {
  debouncer1.update();
  debouncer2.update();
  resetDebouncer.update();

  unsigned long now = millis();

  if (debouncer1.fell()) {
    Serial.println("Button 1 pressed");
    prevPress1Time = press1Time;
    press1Time = now;

    if (prevPress1Time != 0) {
      unsigned long diff = press1Time - prevPress1Time;
      Serial.println("diff: " + String(diff) + " (" + String(fastest1) + ")");
      if (diff < fastest1) {
        fastest1 = diff;
        Serial.println("Button 1 new record");
        startHappySound();   // Play happy sound on new best
      }else {
        startRevvingSound();
      }
    }
  }

  if (debouncer2.fell()) {
    Serial.println("Button 2 pressed");
    prevPress2Time = press2Time;
    press2Time = now;

    if (prevPress2Time != 0) {
      unsigned long diff = press2Time - prevPress2Time;
      if (diff < fastest2) {
        fastest2 = diff;
        Serial.println("Button 2 new record");
        startHappySound();   // Play happy sound on new best
      } else {
        startRevvingSound();
      }
    }
  }

  if (resetDebouncer.fell()) {
    resetTimer();
  }

  // Ensure we do not get above 99 seconds
  if (now - press1Time >= 99999) {
    prevPress1Time = press1Time = now;
  }
  if (now - press2Time >= 99999) {
    prevPress2Time = press2Time = now;
  }

  if (now - lastUpdateLCD >= 30) {  // Update display every 30ms (100ms would not be great: wont show 2nd decimal moving)
    if(runningAnimation == false){
      updateDisplay(now);
    }
    lastUpdateLCD = now;
  }

  // Non-blocking sound update
  updateRevvingSound();

  // Non-blocking happy sound
  updateHappySound();
}

String formatSecMillis(unsigned long ms) {
  unsigned int sec = ms / 1000;
  unsigned int centi = (ms % 1000) / 10; // keep only 2 digits

  char buf[16];
  sprintf(buf, "%lu.%02u", sec, centi);
  return String(buf);
}

void updateDisplay(unsigned long now) {
  display.clearDisplay();
  display.setTextSize(1);

  int col1 = 0;
  int col2 = SCREEN_WIDTH / 2;

  // Line 1: Real-time (since last press)
  display.setCursor(col1, 0);
  display.print("RT:");
  display.println(formatSecMillis(now - press1Time));

  display.setCursor(col2, 0);
  display.print("RT:");
  display.println(formatSecMillis(now - press2Time));

  // Line 2: Last press delta
  unsigned long delta1 = (press1Time > prevPress1Time) ? (press1Time - prevPress1Time) : 0;
  unsigned long delta2 = (press2Time > prevPress2Time) ? (press2Time - prevPress2Time) : 0;

  display.setCursor(col1, 20);
  display.print("Lap:");
  display.println(formatSecMillis(delta1));

  display.setCursor(col2, 20);
  display.print("Lap:");
  display.println(formatSecMillis(delta2));

  // Line 3: Fastest press delta
  display.setCursor(col1, 40);
  display.print("Best:");
  display.println(fastest1 == ULONG_MAX ? "0.00" : formatSecMillis(fastest1));

  display.setCursor(col2, 40);
  display.print("Best:");
  display.println(fastest2 == ULONG_MAX ? "0.00" : formatSecMillis(fastest2));

  display.display();
}

void resetTimer() {
  runningAnimation = true;
  showCountdownAnimation(); // <<< Add this line
  unsigned long now = millis();
  press1Time = prevPress1Time = now;
  press2Time = prevPress2Time = now;
  fastest1 = fastest2 = ULONG_MAX;
  runningAnimation = false;
}

void showCountdownAnimation() {
  const char* countdownTexts[] = { "3", "2", "1", "GO" };
  const int numFrames = 4;

  display.clearDisplay();
  display.setTextSize(4); // Very large text
  display.setTextWrap(false);

  for (int i = 0; i < numFrames; i++) {
    display.clearDisplay();

    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(countdownTexts[i], 0, 0, &x1, &y1, &w, &h);

    int16_t x = (SCREEN_WIDTH - w) / 2;
    int16_t y = (SCREEN_HEIGHT - h) / 2;

    display.setCursor(x, y);
    display.print(countdownTexts[i]);
    display.display();
    // Wait 1 second
    tone(speakerPin, 200); // Beep
    delay(300); 
    noTone(speakerPin);
    delay(700); 
    // 300+700 = 1 second
  }
  tone(speakerPin, 50); // Beep
  delay(2000); // Wait 1 second
  noTone(speakerPin);
  display.clearDisplay(); // Clean up after animation
}

void startRevvingSound() {
  if (playingHappySound) {
    return; // Skip since we are already playing the happy sound
  }
  revving = true;
  currentFreq = 200;
  lastUpdateSound = millis();
  tone(speakerPin, currentFreq);
}

void updateRevvingSound() {
  if (revving) {
    unsigned long now = millis();
    if(now - lastUpdateSound >= stepDelay){
      lastUpdateSound = now;
      tone(speakerPin, currentFreq);
      currentFreq += freqStep;
      if (currentFreq > targetFreq) {
        noTone(speakerPin);
        revving = false;
      }
    }
  }
}


void startHappySound() {
  revving = false; // Stop revving to avoid too many noise
  playingHappySound = true;
  happyNoteIndex = 0;
  happyNoteStartTime = millis();
  tone(speakerPin, happyMelody[happyNoteIndex]);
}

void updateHappySound() {
  if (!playingHappySound) return;

  unsigned long now = millis();
  if (now - happyNoteStartTime >= happyDurations[happyNoteIndex]) {
    happyNoteIndex++;
    Serial.print("Happy note index advanced to: ");
    Serial.println(happyNoteIndex);

    if (happyNoteIndex >= happyNotesCount) {
      noTone(speakerPin);
      playingHappySound = false;
      Serial.println("Happy sound ended");
    } else {
      tone(speakerPin, happyMelody[happyNoteIndex]);
      happyNoteStartTime = now;
      Serial.print("Playing happy note freq: ");
      Serial.println(happyMelody[happyNoteIndex]);
    }
  }
}

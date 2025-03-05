/* 
 * Midterm Project: Perception Accuracy Test (PAT)
 * Description: An IoT device that allows its user to gauge their perception and estimation abilities (i.e. eyeballing) via small games/tests
 * Author: Marlon McPherson
 * Date: 4 MAR 2025
 */

#include "Particle.h"
#include "neopixel.h"
#include "Adafruit_SSD1306.h"
#include "Adafruit_GFX.h"
#include "Encoder.h"
#include "Button.h"
#include "hue.h"
#include "wemo.h"
#include "IoTTimer.h"
#include "hsv.h"

SYSTEM_MODE(MANUAL);
// SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

const int PIXELCOUNT = 16;
const int REDLEDPIN = D11;
const int GREENLEDPIN = D12;
const int BLUELEDPIN = D13;
const int ENCBTNPIN = D14;
const int PSHBTNPIN = D19;
const int OLED_RESET=-1;

Adafruit_SSD1306 display(OLED_RESET);
Encoder myEncoder(D8, D9);
Button encButton(ENCBTNPIN);  // encoder button
Button myButton(PSHBTNPIN);   // standalone button
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);

int tableNum;
int gameMode = 0;
int bulbColor;

int selectTable();
void startGame(int tableNum, int gameMode);
void displayInstructions(int gameMode);
float guessHue();


void setup() {
  Serial.begin(4900);
  waitFor(Serial.isConnected, 5000);

  pinMode(REDLEDPIN, OUTPUT);
  pinMode(GREENLEDPIN, OUTPUT);
  pinMode(BLUELEDPIN, OUTPUT);

  WiFi.on();
  WiFi.clearCredentials();
  WiFi.setCredentials("IoTNetwork");
  
  WiFi.connect();
  while(WiFi.connecting()) {
    Serial.printf(".");
  }
  Serial.printf("\n\n");

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display(); // splash screen
  delay(1000);
  display.clearDisplay();
  
  pixel.begin();
  pixel.setBrightness(100);
  
}

void loop() {
  // digitalWrite(REDLEDPIN, LOW);
  tableNum = selectTable(); // working
  startGame(tableNum, gameMode);

  // delay(2140000000);
}

int selectTable() {
  int tableNum = 0;
  while (!encButton.isClicked()) {
    display.clearDisplay();
    // guaranteed value 0-4
    tableNum = abs((myEncoder.read() / 4) % 5);

    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(11,0);
    display.printf("TURN RIGHT KNOB TO\n");
    display.setCursor(12,13);
    display.printf("SELECT YOUR TABLE.\n");
    display.setCursor(36,32);
    display.printf("Table #%i\n", tableNum);
    display.setCursor(11,52);
    display.printf("PUSH KNOB TO BEGIN.\n");
    display.display();
  }
  return tableNum;
}

void startGame(int tableNum, int gameMode) {
  int _tableNum = tableNum;
  int _gameMode = gameMode;
  int _bulbNum = _tableNum + 1;
  
  // Turn off all hue bulbs except active table. 
  // Set active bulb to random hue. 
  bulbColor = random(0, 65353);
  for (int i=1; i<6; i++) {  // hues indexed from 1, yay
    delay(100);
    if (i == _bulbNum) {
      setHue(_bulbNum, true, bulbColor, 255, 255);
    }
    else {
      setHue(i, false);
    }
  }
  
  display.clearDisplay();
  // Display game instructions and begin specified game mode.
  displayInstructions(_gameMode);
  switch (_gameMode) {
    case 0:
      guessHue();
      break;
    default:
      break;
  }

}

void displayInstructions(int gameMode) {
  int _gameMode = gameMode;
  while (!encButton.isClicked()){
    switch (_gameMode) {
      case 0:   // guess hue bulb color
        display.setCursor(14,0);
        display.printf("REPLICATE CURRENT\n");
        display.setCursor(14,10);
        display.printf("HUE BULB COLOR BY\n");
        display.setCursor(9,20);
        display.printf("TURNING RIGHT KNOB.\n");
        display.setCursor(9,40);
        display.printf("PUSH KNOB TO BEGIN.\n");
        break;
      default:
        break;
    }
    display.display();
  }
}

float guessHue() {
  int _pos;
  int _guess;
  float _accuracy;
  // guessing hue via encoder
  while (!encButton.isClicked()) {
    _pos = myEncoder.read();
    _guess = _pos * 50;
    // bound hue values
    if (_guess < 0) {
      myEncoder.write(round(65350 / 50));
    }
    if (_guess > 65353) {
      myEncoder.write(0);
    }
    Serial.printf("guess: %i\n", _guess);
    setHue(tableNum + 1, true, _guess, 255, 255);
    delay(100);
  }
  // compare values and calculate accuracy
  return _accuracy;
}
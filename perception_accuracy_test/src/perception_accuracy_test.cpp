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
#include "Wire.h"

SYSTEM_MODE(MANUAL);
// SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

const int PIXELCOUNT = 16;
const int BLUE_LEDPIN = D11;
const int RED_LEDPIN = D12;
const int GREEN_LEDPIN = D13;
const int ENCBTNPIN = D14;
const int PSHBTNPIN = D19;
const int SERVOPIN = D16;
const int OLED_RESET = -1;
const int HUERANGE = 65350;

Adafruit_SSD1306 display(OLED_RESET);
Encoder myEncoder(D8, D9);
Button encButton(ENCBTNPIN);  // encoder button
Button myButton(PSHBTNPIN);   // standalone button
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);
Servo accuracyGauge;

int tableNum;
int gameMode = 0;
int servoStartPosition = 0;

int selectTable();
void startGame(int tableNum, int gameMode);
void displayInstructions(int gameMode);
void displayAccuracy(float _accuracy);
float guessHue(int bulbColor);


void setup() {
  Serial.begin(4900);
  waitFor(Serial.isConnected, 5000);

  pinMode(BLUE_LEDPIN, OUTPUT);
  pinMode(RED_LEDPIN, OUTPUT);
  pinMode(GREEN_LEDPIN, OUTPUT);

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

  accuracyGauge.attach(SERVOPIN);
  accuracyGauge.write(servoStartPosition);
  
}

void loop() {
  // digitalWrite(BLUE_LEDPIN, LOW);
  tableNum = selectTable(); // working
  startGame(tableNum, gameMode);

  // delay(2140000000);
}

int selectTable() {
  int tableNum = 0;
  digitalWrite(RED_LEDPIN, LOW); 
  digitalWrite(GREEN_LEDPIN, LOW); 
  digitalWrite(BLUE_LEDPIN, HIGH);   
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
  // Set active bulb to random hue. 
  int _bulbColor = random(0, HUERANGE);
  int _bulbNum = _tableNum + 1; // hue bulbs indexed from 1
  float _accuracy;
  
  // Turn off all hue bulbs and wemos except active table. 
  for (int i=1; i<6; i++) {  
    delay(100);
    if (i == _tableNum) {
      wemoWrite(_tableNum, HIGH);
      setHue(i, false);
    }
    else if (i == _bulbNum) {
      wemoWrite(i, LOW);
      setHue(_bulbNum, true, _bulbColor, 255, 255);
    }
    else {
      wemoWrite(i, LOW);
      setHue(i, false);
    }
  }
  
  display.clearDisplay();
  // Display game instructions and begin specified game mode.
  displayInstructions(_gameMode);
  switch (_gameMode) {
    case 0:
      _accuracy = guessHue(_bulbColor);
      delay(100);
      displayAccuracy(_accuracy);
      break;
    default:
      break;
  }

}

void displayInstructions(int gameMode) {
  int _gameMode = gameMode;
  digitalWrite(RED_LEDPIN, HIGH); 
  digitalWrite(GREEN_LEDPIN, LOW); 
  digitalWrite(BLUE_LEDPIN, LOW);   
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
  // pick random hue to start guessing from
  // int currHue = getHue(tableNum + 1);
  int nextHue = random(0, HUERANGE);
 
  setHue(tableNum + 1, true, nextHue, 255, 255);
}

void displayAccuracy(float accuracy) {
  display.clearDisplay();
  while (!encButton.isClicked()) {
    if (accuracy > 0.9) {
      display.setTextSize(2);
      display.setCursor(12,0);
      display.printf("ACCURACY:\n");
      display.setCursor(36,40);
      display.printf("%0.1f%%\n", accuracy);
      display.display();
      accuracyGauge.write(round(accuracy));
    }
  }
}

float guessHue(int bulbColor) {
  int _pos;
  int _guess;
  int _bulbColor = bulbColor; // is this needed?
  int _delta;
  float _accuracy;
  digitalWrite(RED_LEDPIN, LOW); 
  digitalWrite(GREEN_LEDPIN, HIGH); 
  digitalWrite(BLUE_LEDPIN, LOW);   
  // guessing hue via encoder
  while (!encButton.isClicked()) {
    _pos = myEncoder.read();
    _guess = _pos * 50;
    // bound hue values
    if (_guess < 0) {
      myEncoder.write(HUERANGE / 50); 
    }
    if (_guess > HUERANGE) {
      myEncoder.write(0);
    }
    // display guess on OLED
    display.clearDisplay();
    display.setCursor(34,0);
    display.printf("CURRENT HUE:\n");
    display.setCursor(50,30);
    display.printf("%i\n", _guess);
    display.display();

    setHue((tableNum + 1), true, _guess, 255, 255);
    delay(100);
  }
  // compare values and calculate accuracy
  // https://stackoverflow.com/a/36072199
  _delta = (_guess + HUERANGE - _bulbColor) % HUERANGE;
  if (_delta <= HUERANGE / 2) { 
    _accuracy = ((float)HUERANGE - (float)_delta) / (float)HUERANGE;
  }
  else {
    _accuracy = ((float)HUERANGE - ((float)HUERANGE - (float)_delta)) / (float)HUERANGE;
  }
  // convert from 0.5 - 1.0 range to 0.0 - 1.0
  // https://stackoverflow.com/questions/929103/convert-a-number-range-to-another-range-maintaining-ratio/929107#929107
  float _newAccuracy = ((_accuracy - 0.5) / 0.5);
  Serial.printf("delta: %i, accuracy: %0.3f, converted: %0.3f\n", _delta, _accuracy, _newAccuracy);
  return _newAccuracy * 100; // convert to percentage
}
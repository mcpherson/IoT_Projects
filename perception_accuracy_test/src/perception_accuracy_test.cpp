/* 
 * Midterm Project: Perception Accuracy Test
 * Description: An IoT device that allows its user to gauge their perception and estimation abilities (i.e. eyeballing) via small games/tests
 * Author: Marlon McPherson
 * Date: 4 MAR 2025
 */

#include "Particle.h"
#include "Adafruit_SSD1306.h" 
#include "Adafruit_GFX.h"
#include "Adafruit_BME280.h"
#include "Encoder.h"
#include "Button.h"
#include "hue.h"
#include "wemo.h"
#include "neopixel.h"
#include "IoTTimer.h"
#include "hsv.h"
#include "Wire.h"

SYSTEM_MODE(MANUAL);
// SYSTEM_THREAD(ENABLED);
SerialLogHandler logHandler(LOG_LEVEL_INFO);

const int PIXELCOUNT = 8;
const int BLUE_LEDPIN = D11;
const int RED_LEDPIN = D12;
const int GREEN_LEDPIN = D13;
const int BTNPIN = D14;
const int SERVOPIN = D16;
const int OLED_RESET = -1;
const int HUERANGE = 65350;
const byte BMEADDRESS = 0x76;
const byte DEGREESYMBOL = 248;

Adafruit_SSD1306 display(OLED_RESET);
Encoder myEncoder(D8, D9);
Button myButton(BTNPIN);
Adafruit_NeoPixel pixel(PIXELCOUNT, SPI1, WS2812B);
Servo accuracyGauge;
Adafruit_BME280 bme;

int tableNum = 0;
int gameMode = 0;
int servoStartPosition = 0;
byte status;

int selectTable(int tableNum);
int selectGameMode(int gameMode);
void startGame(int tableNum, int gameMode);
void displayInstructions(int gameMode);
void displayAccuracy(float _accuracy);
void lightshow();
float guessHue(int bulbColor);
float guessMidLine();
float guessTemp();

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
  display.clearDisplay();

  status = bme.begin(BMEADDRESS);
  if (status == false) {
    Serial.printf("BME280 at address 0x%02X failed to start", BMEADDRESS); 
  }
  
  pixel.begin();
  pixel.setBrightness(100);

  accuracyGauge.attach(SERVOPIN);
  accuracyGauge.write(servoStartPosition);
  
}

// MAIN LOOP
void loop() {
  accuracyGauge.write(servoStartPosition);
  tableNum = selectTable(tableNum);
  gameMode = selectGameMode(gameMode);
  startGame(tableNum, gameMode);
}

int selectTable(int tableNum) {
  myEncoder.write(tableNum * 4);
  // output values reversed? Input pullup because encoder is wired upside-down?
  digitalWrite(RED_LEDPIN, HIGH); 
  digitalWrite(GREEN_LEDPIN, HIGH); 
  digitalWrite(BLUE_LEDPIN, LOW);   
  while (!myButton.isClicked()) {
    tableNum = abs(((myEncoder.read() / 4) + 5) % 5); 
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(11,0);
    display.printf("TURN RIGHT KNOB TO\n");
    display.setCursor(12,13);
    display.printf("SELECT YOUR TABLE.\n");
    display.setCursor(36,32);
    display.printf("Table #%i\n", tableNum);
    display.setCursor(5,52);
    display.printf("PUSH BUTTON TO BEGIN.\n");
    display.display();
  }
  return tableNum;
}

int selectGameMode(int gameMode) {
  myEncoder.write(gameMode * 4);  
  while (!myButton.isClicked()) {
    gameMode = abs(((myEncoder.read() / 4) + 4) % 4);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(11,0);
    display.printf("TURN RIGHT KNOB TO\n");
    display.setCursor(15,13);
    display.printf("SELECT GAME MODE.\n");
    display.setCursor(5,52);
    display.printf("PUSH BUTTON TO BEGIN.\n");
    switch (gameMode) {
      case 0: 
        display.setCursor(33,32);
        display.printf("Guess Hue\n");
        break;
      case 1: 
        display.setCursor(23,32);
        display.printf("Line Midpoint");
        break;
      case 2:
        display.setCursor(14,32);
        display.printf("Guess Temperature\n");
        break;
      case 3:
        display.setCursor(33,32);
        display.printf("Lightshow\n");
        break;
      default:
        break;
    }
    display.display();
  }
  return gameMode;
}

void startGame(int tableNum, int gameMode) {
  int _tableNum = tableNum;
  int _gameMode = gameMode;
  // Set active bulb to random hue. 
  int _bulbColor = random(0, HUERANGE);
  int _bulbNum = _tableNum + 1; // hue bulbs indexed from 1
  float _accuracy;
  myEncoder.write(0);
  
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
  
  // Display game instructions and begin specified game mode.
  display.clearDisplay();
  displayInstructions(_gameMode);
  switch (_gameMode) {
    case 0: // Guess Hue
      _accuracy = guessHue(_bulbColor);
      delay(100);
      displayAccuracy(_accuracy);
      break;
    case 1: // Guess Line Midpoint
      _accuracy = guessMidLine();
      delay(100);
      displayAccuracy(_accuracy);
      break;
    case 2: // Guess Temperature
      _accuracy = guessTemp();
      delay(100);
      displayAccuracy(_accuracy);
      break;
    case 3: // Lightshow
      lightshow();
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
  while (!myButton.isClicked()){
    switch (_gameMode) {
      case 0:   // guess hue
        display.setCursor(14,0);
        display.printf("REPLICATE CURRENT\n");
        display.setCursor(14,10);
        display.printf("HUE BULB COLOR BY\n");
        display.setCursor(9,20);
        display.printf("TURNING RIGHT KNOB.\n");
        display.setCursor(0,40);
        display.printf("PUSH BUTTON TO BEGIN.\n");
        break;
      case 1:   // line midpoint
        display.setCursor(8,0);
        display.printf("GUESS LINE MIDPOINT\n");
        display.setCursor(14,10);
        display.printf("USING RIGHT KNOB.\n");
        display.setCursor(0,20);
        display.printf("PUSH BUTTON TO BEGIN.\n");
        display.setCursor(6,40);
        display.printf("PUSH AGAIN TO GUESS.\n");
        break;
      case 2:  // guess temp
        display.setCursor(0,0);
        display.printf("GUESS ROOM TEMP (%cF)\n", DEGREESYMBOL);
        display.setCursor(14,10);
        display.printf("USING RIGHT KNOB.\n");
        display.setCursor(0,20);
        display.printf("PUSH BUTTON TO BEGIN.\n");
        display.setCursor(6,40);
        display.printf("PUSH AGAIN TO GUESS.\n");
        break;
      case 3:  // lightshow
        display.setTextSize(2);
        display.setCursor(5,0);
        display.printf("WHOA DUDE!\n");
        display.setTextSize(1);
        display.setCursor(20,40);
        display.printf("PUSH THE BUTTON\n");
        display.setCursor(9,50);
        display.printf("TO END THE MADNESS.\n");
        display.display();
        return; // skip to execution, no need to read first
      default:
        break;
    }
    display.display();
  }
}

void displayAccuracy(float accuracy) {
  display.clearDisplay();
  accuracyGauge.write(round(accuracy));
  display.setTextSize(2);
  display.setCursor(13,0);
  display.printf("ACCURACY:\n");
  display.setCursor(36,26);
  display.printf("%0.1f%%\n", accuracy);
  display.setTextSize(1);
  display.setCursor(14,54);
  display.printf("PUSH BUTTON TO END\n");
  display.display();
  while (!myButton.isClicked()) {
    if (accuracy > 95.0) {
      // rainbow knob
      for (int i=0; i<6; i++) {
        IoTTimer timer;
        timer.startTimer(200);
        while (!timer.isTimerReady()) {
          if (myButton.isClicked()) {
            return;
          }
          switch (i) {
            case 0:
              digitalWrite(RED_LEDPIN, LOW); 
              digitalWrite(GREEN_LEDPIN, HIGH); 
              digitalWrite(BLUE_LEDPIN, HIGH);
              break;
            case 1: 
              digitalWrite(GREEN_LEDPIN, LOW); 
              break;
            case 2:
              digitalWrite(RED_LEDPIN, HIGH);
              break;
            case 3:
              digitalWrite(BLUE_LEDPIN, LOW);
              break;
            case 4:
              digitalWrite(GREEN_LEDPIN, HIGH);
              break;
            case 5:
              digitalWrite(RED_LEDPIN, LOW);
              break;
            default:
              break;
          }
        }
      }
    }
    else {
      digitalWrite(RED_LEDPIN, LOW); 
      digitalWrite(GREEN_LEDPIN, LOW); 
      digitalWrite(BLUE_LEDPIN, HIGH);
    }
  }
}

void lightshow() { // automatic mode
  int startColors[6] {0, 11000, 22000, 33000, 44000, 55000};
  // turn on wemos
  for (int i=0; i<5; i++) {
    wemoWrite(i, HIGH);
    delay(100);
  }
  // lightshow
  while(!myButton.isClicked()) {
    for (int i=0; i<6; i++) {
      IoTTimer timer;
      timer.startTimer(150);
      while (!timer.isTimerReady()) {
        setHue(i+1, true, startColors[i], 255, 255);
        startColors[i] == 65000 ? startColors[i] = 0 : startColors[i] += 1000;
        // rainbow knob
        switch (i) {
          case 0:
            digitalWrite(RED_LEDPIN, LOW); 
            digitalWrite(GREEN_LEDPIN, HIGH); 
            digitalWrite(BLUE_LEDPIN, HIGH);
            break;
          case 1: 
            digitalWrite(GREEN_LEDPIN, LOW); 
            break;
          case 2:
            digitalWrite(RED_LEDPIN, HIGH);
            break;
          case 3:
            digitalWrite(BLUE_LEDPIN, LOW);
            break;
          case 4:
            digitalWrite(GREEN_LEDPIN, HIGH);
            break;
          case 5:
            digitalWrite(RED_LEDPIN, LOW);
            break;
          default:
            break;
        }
      }
    } 
  }
}

float guessHue(int bulbColor) {
  int _pos;
  int _guess;
  int _bulbColor = bulbColor; // is this needed?
  int _delta;
  float _accuracy;

  // pick a different hue to start guessing from
  int nextHue;
  _bulbColor > HUERANGE/2
    ? nextHue = random(_bulbColor - HUERANGE/2, (_bulbColor - HUERANGE/2) + 2000) 
    : nextHue = random((_bulbColor + HUERANGE/2) - 2000, _bulbColor - HUERANGE/2);
  setHue(tableNum + 1, true, nextHue, 255, 255);

  digitalWrite(RED_LEDPIN, HIGH); 
  digitalWrite(GREEN_LEDPIN, LOW); 
  digitalWrite(BLUE_LEDPIN, HIGH);   
  // guessing hue via encoder
  while (!myButton.isClicked()) {
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

float guessMidLine() {
  int endpoints[4] { // (x1, y1) (x2, y2)
    random(1, 15),
    random(1, 63),
    random(113, 127),
    random(1, 63)
  };
  int deltaEndpoints[2] {
    endpoints[2] - endpoints[0],
    endpoints[3] - endpoints[1]
  };
  float mid[2] { 
    deltaEndpoints[0] / 2.0,
    deltaEndpoints[1] / 2.0
  };
  float slope = ((float)deltaEndpoints[1]) / (deltaEndpoints[0]);
  float yInt = endpoints[1] - (slope * endpoints[0]);
  float guess[2] {
    (float)endpoints[0],
    (float)endpoints[1]
  };
  float accuracy;
  float length = hypot(deltaEndpoints[0], deltaEndpoints[1]);

  digitalWrite(RED_LEDPIN, HIGH); 
  digitalWrite(GREEN_LEDPIN, LOW); 
  digitalWrite(BLUE_LEDPIN, HIGH);

  while (!myButton.isClicked()) {
    display.clearDisplay();
    display.drawLine(endpoints[0], endpoints[1], endpoints[2], endpoints[3], WHITE);
    display.display();
    int pos = myEncoder.read() / 4;
    if (pos < 0) {
      myEncoder.write(0);
    }
    if (pos > deltaEndpoints[0]) {
      myEncoder.write(deltaEndpoints[0]);
    }

    guess[0] = (float)endpoints[0] + (float)pos;
    guess[1] = (slope * pos) + yInt;

    display.drawLine(guess[0], guess[1]-5, guess[0], guess[1]+5, WHITE);
    display.display();
  }
  accuracy = (((length / 2.0) - (abs((mid[0] + endpoints[0]) - guess[0]))) / (length / 2.0)) * 100.0;
  return accuracy;
}

float guessTemp() {
  int currTemp = ((9.0/5.0) * bme.readTemperature()) + 32.0;
  int minTemp = 32;
  int maxTemp = 100;
  int tempRange = maxTemp - minTemp;
  int guess;
  while (!myButton.isClicked()) {
    guess = (myEncoder.read() / 4) + minTemp;
    if (guess < minTemp) {
      myEncoder.write(0);
      guess = minTemp;
    }
    if (guess > maxTemp) {
      myEncoder.write((tempRange * 4)); 
      guess = maxTemp;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(32,0);
    display.printf("Feels like");
    display.setTextSize(2);
    display.setCursor(36,26);
    display.printf("%i %cF\n", guess, DEGREESYMBOL);
    display.setTextSize(1);
    display.setCursor(9,54);
    display.printf("PUSH KNOB TO GUESS.\n");
    display.display();
    // calc hue between blue and red
    int tempHue = round((((guess - minTemp) * (65000.0 - 45000.0)) / (maxTemp - minTemp)) + 45000.0);
    setHue(tableNum + 1, true, tempHue, 255, 255);
    delay(100);
  }
  float accuracy = (((float)tempRange - abs(currTemp - guess)) / tempRange) * 100.0;
  return accuracy;
}

/* FIX:
 * timers
 * +- accuracy
 */
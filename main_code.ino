#include <Wire.h> //library that helps I2C communication as Arduino and LCD 
#include <LiquidCrystal_I2C.h> //controls LCD screen
#include <DHT.h> //controls DHT sensor
#include <PulseSensorPlayground.h>// helps detect heartbeat signals from pulse sensor

// ========================================
// DHT11 SETUP
// ========================================

#define DHTPIN   2 //DHT11 data pin is connected to aurdino pin 2
#define DHTTYPE  DHT11//sensor type is DHT11

DHT dht(DHTPIN, DHTTYPE);

// ========================================
// PULSE SENSOR SETUP
// ========================================

#define PULSE_PIN   A1 
#define THRESHOLD   550

PulseSensorPlayground pulseSensor;

// ========================================
// LCD SETUP
// ========================================

LiquidCrystal_I2C lcd(0x27, 16, 2);

// ========================================
// VARIABLES
// ========================================

int myBPM = 0; //stors heartbeat value

float temperature = 0.0; //float so decimal reading is allowed.
float humidity    = 0.0;

bool dhtError   = false; //these are the error flags 
bool pulseError = false; //They only stores true eor false and if it's true it shows error

unsigned long lastDHTRead   = 0; //time variables and stores last time DHT11 was read
unsigned long lastLCDUpdate = 0; //stores the last time LCD was updated
unsigned long lastBeatTime  = 0; //stores last time heartbeat was detected

bool showTempScreen = true; //used for switching the LCD pages, 1st BPM+Temp, BPM+Humidity

// ========================================
// SETUP
// ========================================

void setup() { //runs one time when arduino starts

  Serial.begin(9600); //starts serial montior at the speed 9600, used for debugging

  // LCD
  lcd.init(); //starts start
  lcd.backlight(); //turns on the LCD light

  lcd.setCursor(0, 0); //0 column, 0 row
  lcd.print("Health Monitor"); //display text
  lcd.setCursor(0, 1); // 0 column, 1 row
  lcd.print("Starting...");//displays text
  delay(2000); //wait 2 secconds
  lcd.clear();//clears the LCD

  // DHT11
  dht.begin(); //Start DHT

  // Pulse Sensor
  pulseSensor.analogInput(PULSE_PIN); // connected to A1
  pulseSensor.setThreshold(THRESHOLD); //sets heartbeat detection threshold

  if (pulseSensor.begin()) { //start pulse senosr
    Serial.println("Pulse Sensor Ready!"); //system fully ready
  }

  Serial.println("System Ready!");
}

// ========================================
// LOOP
// ========================================

void loop() { //runs forever

  // ========================================
  // 1. PULSE SENSOR READ + FILTER
  // ========================================

  int pulseValue = analogRead(PULSE_PIN); //reads raw analog value form sensor

  // Detect unplugged sensor
  if (pulseValue < 10 || pulseValue > 1015) { //checks if the sensor is disconnected

    pulseError = true; //if errore= resets bpm to 0
    myBPM = 0;
  }
  else {

    pulseError = false;

    if (pulseSensor.sawStartOfBeat()) {

      int newBPM = pulseSensor.getBeatsPerMinute();

      Serial.print("Detected BPM: ");
      Serial.println(newBPM);

      // Only allow BPM between 50 and 120
      if (newBPM >= 50 && newBPM <= 120) {

        // Smooth BPM reading
        if (myBPM == 0) {
          myBPM = newBPM; //saves BPM data
        } else { //if not first reading
          myBPM = (myBPM + newBPM) / 2; // old bpm, and new bpm reads and results
        }

        lastBeatTime = millis(); //stors the current time of the heartbeat
      }
    }
  }

  // No heartbeat detected for 5 seconds
  if (millis() - lastBeatTime > 5000) {

    pulseError = true;
    myBPM = 0;
  }

  // ========================================
  // 2. DHT11 READ EVERY 2 SECONDS
  // ========================================

  if (millis() - lastDHTRead >= 2000) { //reads every 2 second because dht11 is slow, and reading too slow cause error

    lastDHTRead = millis();

    float newTemp = dht.readTemperature();
    float newHum  = dht.readHumidity();

    if (isnan(newTemp) || isnan(newHum)) { //isnan, is this not a valid number, happens because DHT disconnect and wiring wrong

      dhtError = true;
      Serial.println("ERROR: DHT11 Missing!");
    }
    else {

      dhtError = false;

      temperature = newTemp;
      humidity    = newHum;
    }
  }

  // ========================================
  // 3. LCD UPDATE EVERY 1 SECOND
  // ========================================

  if (millis() - lastLCDUpdate >= 2000) { //update LCD everyseconds

    lastLCDUpdate = millis();

    lcd.clear();

    // ========================================
    // PRIORITY 1: ERRORS
    // ========================================

    if (dhtError) {

      lcd.setCursor(0, 0);
      lcd.print("ERROR:");
      lcd.setCursor(0, 1);
      lcd.print("DHT11 Missing");
      return;
    }

    if (pulseError) {

      lcd.setCursor(0, 0);
      lcd.print("ERROR:");
      lcd.setCursor(0, 1);
      lcd.print("Pulse Missing");
      return;
    }

    // ========================================
    // NORMAL DISPLAY
    // ========================================

    showTempScreen = !showTempScreen; //used to switch the screen

    if (showTempScreen) {

      lcd.setCursor(0, 0);
      lcd.print("BPM: ");
      lcd.print(myBPM);

      lcd.setCursor(0, 1);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print((char)223);
      lcd.print("C");
    }
    else {

      lcd.setCursor(0, 0);
      lcd.print("BPM: ");
      lcd.print(myBPM);

      lcd.setCursor(0, 1);
      lcd.print("Hum: ");
      lcd.print(humidity, 1);
      lcd.print("%");
    }
  }

  delay(20);
}

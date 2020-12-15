#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "DHT.h"
#include <HTTPClient.h>

//used pins
#define heater 23
#define humidifier 15
#define upButton 33
#define downButton 32
#define setButton 25
#define nextButton 26

// DHT Sensor
uint8_t dhtPin = 4;

// Controller mode
int controlMode = 1; //1 = PID, 2 = on-off

// on-off controller settings
float heaterHysteresis = 1;
float humidifierHysteresis = 1;

//PID preset for heater
float KpTemperature = 0.8;
float KiTemperature = 0.001;
float KdTemperature = 0.03;

//PID preset for humidifier
float KpHumidity = 2;
float KiHumidity = 1.5;
float KdHumidity = 1;

//LCD screen preset
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

//WiFi connection
const char* ssid = "WiFiName";  // Enter SSID here
const char* password = "123456789";  //Enter Password here

//server interactions links - change your server address here
const char* serverGetConnfigAddress = "https://esp32-terrarium-control.your.page/getConfig";
const char* serverSendConfigAddress = "https://esp32-terrarium-control.your.page/config?";
const char* serverSendStateAddress = "https://esp32-terrarium-control.your.page/stateChange?";
const char* serverSendReadingsgAddress = "https://esp32-terrarium-control.your.page/reading?";
const char* serverGetControlModeAddress = "https://esp32-terrarium-control.your.page/getMode";
const char* serverSendControlModeAddress = "https://esp32-terrarium-control.your.page/mode?";
const char* heaterLink = "grzalka=";
const char* humidifierLink = "pompka=";
const char* temperatureLink = "temp=";
const char* humidityLink = "wilg=";

// Initialize DHT sensor.
DHT dht(dhtPin, DHTTYPE);

float temperatureReading, sentTemperature, humidityReading, sentHumidity;
volatile double oldSetTemperature, oldSetHumidity, setTemperature, setHumidity;
float pidIntegralTemperature, pidIntegralHumidity, pidDerivativeTemperature, previousTemperature, pidDerivativeHumidity, previousHumidity, pidProportionTemperature, pidProportionHumidity, pidTemperature, pidHumidity;
bool isHeatingCurrentlyOn, wasHeatingOnLastSent, isHumidifierCurrentlyOn, wasHumidifierOnLastSent;
volatile bool isInSeccondScreen = false, isButtonInteractionLocked = false, isInEditMode = false, isEditingTemperature = true, isNewSettingToSend = false;
volatile int isOnScreen = 0;
static int dataScreens = 1, editScreens = 11;

//button logic
void IRAM_ATTR up() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    switch (isOnScreen) {
      case 10:
        if (isEditingTemperature) {
          setTemperature += 0.5;
        }
        else {
        setHumidity += 0.5;
        }
        break;
    }
  }
}
void IRAM_ATTR down() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    switch (isOnScreen) {
      case 10:
        if (isEditingTemperature) {
          setTemperature -= 0.5;
        }
        else {
        setHumidity -= 0.5;
        }
        break;
      }
    }
  }
}
void IRAM_ATTR set() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    isInEditMode = !isInEditMode;
    isNewSettingToSend = true;
  }
}
void IRAM_ATTR next() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    if (isInEditMode) {
      isEditingTemperature = !isEditingTemperature;
    }
    else {
      isInSeccondScreen = !isInSeccondScreen;
    }
  }
}

//time logic
const long minRefreshTime = 5 * 1000, pidRefreshTime = 5 * 1000, serverRefreshTime = 5 * 1000;

unsigned long previousServerTime = 0, previousPIDTemperatureTime = 0, previousPIDHumidityTime = 0, deltaTime;

TaskHandle_t Task1;

//Program for core 0
void codeForTask1( void * parameter )
{
  for (;;) {

    switch (controlMode) {
      case 1:
        unsigned long currentTimeOnCore0 = millis();

        delay (3000);

        //    Temperature and humidity reading
        temperatureReading = getTemperatureFromSensor();
        humidityReading = getHumidityFromSensor();
        Serial.print("Sir! Reading: ");
        Serial.print(temperatureReading);
        Serial.print(" °C, ");
        Serial.print(humidityReading);
        Serial.print(" %. Difference: ");
        Serial.print(error(setTemperature, temperatureReading));
        Serial.print(" °C, ");
        Serial.print(error(setHumidity, humidityReading));
        Serial.print(" %");
        Serial.print("This Task runs on Core: ");
        Serial.println(xPortGetCoreID());

        //    PID for temperature
        currentTimeOnCore0 = millis();
        deltaTime = (currentTimeOnCore0 - previousPIDTemperatureTime) / 1000;
        previousPIDTemperatureTime = currentTimeOnCore0;
        pidProportionTemperature = error(setTemperature, temperatureReading);
        pidIntegralTemperature = (pidProportionTemperature * deltaTime) + pidIntegralTemperature;
        pidDerivativeTemperature = (pidProportionTemperature - previousTemperature) / deltaTime;
        previousTemperature = pidProportionTemperature;
        pidTemperature = (KpTemperature * pidProportionTemperature) + (KiTemperature * pidIntegralTemperature) + (KdTemperature * pidDerivativeTemperature);
        if (pidTemperature > 1) {
          pidTemperature = 1;
          pidIntegralTemperature = pidIntegralTemperature - (pidProportionTemperature * deltaTime);
        }
        else if (pidTemperature < 0) {
          pidTemperature = 0;
          pidIntegralTemperature = pidIntegralTemperature - (pidProportionTemperature * deltaTime);
        }
        if (pidTemperature > 0.2) {
          digitalWrite(heater, HIGH);
          isHeatingCurrentlyOn = true;
        }
        else {
          digitalWrite(heater, LOW);
          isHeatingCurrentlyOn = false;
        }

        //    PID for humidity
        currentTimeOnCore0 = millis();
        deltaTime = (currentTimeOnCore0 - previousPIDHumidityTime) / 1000;
        previousPIDHumidityTime = currentTimeOnCore0;
        pidProportionHumidity = error(setHumidity, humidityReading);
        pidIntegralHumidity = (pidProportionHumidity * deltaTime) + pidIntegralHumidity;
        pidDerivativeHumidity = (pidProportionHumidity - previousHumidity) / deltaTime;
        previousHumidity = pidProportionHumidity;
        pidHumidity = (KpHumidity * pidProportionHumidity) + (KiHumidity * pidIntegralHumidity) + (KdHumidity * pidDerivativeHumidity);
        if (pidHumidity > 1) {
          pidHumidity = 1;
          pidIntegralHumidity = pidIntegralHumidity - (pidProportionHumidity * deltaTime);
        }
        else if (pidHumidity < 0) {
          pidHumidity = 0;
          pidIntegralHumidity = pidIntegralHumidity - (pidProportionHumidity * deltaTime);
        }
        if (pidHumidity > 0.2) {
          digitalWrite(humidifier, HIGH);
          isHumidifierCurrentlyOn = true;
        }
        else {
          digitalWrite(humidifier, LOW);
          isHumidifierCurrentlyOn = false;
        }
        break;
      case 2:
        delay(3000);

        // on - off heater
        if (isHeatingCurrentlyOn && error(setTemperature, temperatureReading) + heaterHysteresis < 0) {
          digitalWrite(heater, LOW);
          isHeatingCurrentlyOn = false;
        }
        else if (!isHeatingCurrentlyOn && error(setTemperature, temperatureReading) - heaterHysteresis > 0)
        {
          digitalWrite(heater, HIGH);
          isHeatingCurrentlyOn = true;
        }

        // on - off humidifier
        if (isHumidifierCurrentlyOn && error(setHumidity, humidityReading) + humidifierHysteresis < 0) {
          digitalWrite(humidifier, LOW);
          isHumidifierCurrentlyOn = false;
        }
        else if (!isHumidifierCurrentlyOn && error(setHumidity, humidityReading) - humidifierHysteresis > 0)
        {
          digitalWrite(humidifier, HIGH);
          isHumidifierCurrentlyOn = true;
        }

        break;
    }
  }
}


void setup() {
  Serial.begin(115200);
  delay(100);

  pinMode(heater, OUTPUT);
  pinMode(humidifier, OUTPUT);
  digitalWrite(heater, LOW);
  digitalWrite(humidifier, LOW);

  pinMode(dhtPin, INPUT);

  dht.begin();

  Serial.println("Connecting with ");
  Serial.println(ssid);

  //initializing connection with
  WiFi.begin(ssid, password);

  //write dots while connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("o Boi I'm connected!");
  Serial.print("and my IP is: ");  Serial.println(WiFi.localIP());

  // initialize LCD
  lcd.init();

  // turn on LCD backlight
  lcd.backlight();
  pinMode(upButton, INPUT_PULLUP);
  pinMode(downButton, INPUT_PULLUP);
  pinMode(setButton, INPUT_PULLUP);
  pinMode(nextButton, INPUT_PULLUP);
  attachInterrupt(upButton, up, FALLING);
  attachInterrupt(downButton, down, FALLING);
  attachInterrupt(setButton, set, FALLING);
  attachInterrupt(nextButton, next, FALLING);

  xTaskCreatePinnedToCore(
    codeForTask1,            /* Task function. */
    "Task_1",                 /* name of task. */
    1000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &Task1,                   /* Task handle to keep track of created task */
    0);                       /* Core */

}

//Program for core 1
void loop() {

  unsigned long currentTimeOnCore1 = millis();

  //  Sending local changes
  if (isNewSettingToSend && !isInEditMode) {
    isNewSettingToSend = false;
    HTTPClient http;
    http.begin(serverSendConfigAddress + temperatureLink + String(setTemperature, 2) + "&" + humidityLink + String(setHumidity, 2));
    Serial.println("Sending to server temperature set to: " + String(setTemperature, 2) + "and humidity set to: " + String(setHumidity, 2));
    http.GET();
    http.end();
  }
  if (currentTimeOnCore1 - previousServerTime >= serverRefreshTime) {
    previousServerTime = currentTimeOnCore1;
    //  Chcecking connection with WiFi
    if (WiFi.status() == WL_CONNECTED && isInEditMode == false) {
      controlMode = httpGETDATA(serverGetControlModeAddress);
      String serverReply = httpGETDATA(serverGetConnfigAddress);
      noInterrupts();
      oldSetTemperature = setTemperature;
      oldSetHumidity = setHumidity;
      setTemperature = getIntX(serverReply, 1);
      setHumidity = getIntX(serverReply, 2);
      interrupts();
      if (oldSetTemperature != setTemperature || oldSetHumidity != setHumidity) {
        simulateLCD();
        Serial.print(setTemperature);
        Serial.print(" °C, ");
        Serial.print(setHumidity);
        Serial.print(" %");
      }
      Serial.print("This Task runs on Core: ");
      Serial.println(xPortGetCoreID());
    }

    //sending heater and humidifier state on change
    bool isStateChanged = wasHeatingOnLastSent != isHeatingCurrentlyOn || wasHumidifierOnLastSent != isHumidifierCurrentlyOn;
    if (isStateChanged) {
      wasHeatingOnLastSent = isHeatingCurrentlyOn;
      wasHumidifierOnLastSent = isHumidifierCurrentlyOn;
      simulateLCD();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("sending heater and humidifier state to server");
        HTTPClient http;
        http.begin(serverSendStateAddress + heaterLink + String(wasHeatingOnLastSent ? 1 : 0, 0) + "&" + humidifierLink + String(wasHumidifierOnLastSent ? 1 : 0, 0));
        http.GET();
        http.end();
      }
    }

    //sending temperature and humidity on change
    bool isReadingChanged = sentTemperature != temperatureReading || sentHumidity != humidityReading;
    if (isReadingChanged) {
      sentTemperature = temperatureReading;
      sentHumidity = humidityReading;
      simulateLCD();
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.println("sending temperature and humidity to server");
        http.begin(serverSendReadingsgAddress + temperatureLink + String(temperatureReading, 2) + "&" + humidityLink + String(humidityReading, 2));
        http.GET();
        http.end();
      }
    }

  }

  if (isButtonInteractionLocked) {
    noInterrupts();
    simulateLCD();
    delay (50);
    isButtonInteractionLocked = false;
    interrupts();
  }
}

String httpGETDATA(const char* serverLink) {
  HTTPClient http;

  //  Vader the time has come
  http.begin(serverLink);

  // Do it,... do it now
  int httpReply = http.GET();

  String package = "--";

  if (httpReply > 0)  {
    Serial.print("Sir! The server says: ");
    package = http.getString();
  }
  else {
    Serial.print("Sir! I don't like it. It's quiet too quiet: ");
    Serial.print(httpReply);
  }
  // Vader release him at once
  http.end();

  return package;
}

/*
  gets the number x int from serverReply
*/
float getIntX(String serverReply, int x) {
  serverReply = serverReply.substring(3, serverReply.length()); //removes the first 3 characters from left
  char message[serverReply.length()]; //converting to char
  serverReply.toCharArray(message, serverReply.length());
  char* wantedVariable = strtok(message, " :\{\"temp,"); //returns the first wanted variable
  if (x == 1) {
    return atof(wantedVariable);
  }
  else {
    wantedVariable = strtok(NULL, " :\{\"temp,wilg");
    return atof(wantedVariable);
  }
}


/*
  gets 18 readings of temperature from sensor, eliminates extreme values and returns the average
*/
float getTemperatureFromSensor() {
  float currentTemperatureReading;

  //  gets the first mesurement
  while (true) {
    currentTemperatureReading = dht.readTemperature();
    if (!isnan(currentTemperatureReading)) { //checks if the first mesurement is correct
      break;
    }
  }
  float temperatureReadingsSum = currentTemperatureReading, maxTemperatureReading = currentTemperatureReading, minTemperatureReading = currentTemperatureReading;

  for (int var = 0; var < 17; var++) {
    currentTemperatureReading = dht.readTemperature();
    if (!isnan(currentTemperatureReading)) {
      temperatureReadingsSum = temperatureReadingsSum + currentTemperatureReading;
      if (currentTemperatureReading > maxTemperatureReading) {
        maxTemperatureReading = currentTemperatureReading;
      }
      if (currentTemperatureReading < minTemperatureReading) {
        minTemperatureReading = currentTemperatureReading;
      }
    }
  }
  return (temperatureReadingsSum - maxTemperatureReading - minTemperatureReading) / 16;
}

/*
  gets 18 readings of humidity from sensor, eliminates extreme values and returns the average
*/
float getHumidityFromSensor() {
  float currentHumidityReading;

  //  gets the first mesurement
  while (true) {
    currentHumidityReading = dht.readHumidity();
    if (!isnan(currentHumidityReading)) { //checks if the first mesurement is correct
      break;
    }
  }

  float humidityReadingSum = currentHumidityReading, maxHumidityReading = currentHumidityReading, minHumidityReading = currentHumidityReading;

  for (int var = 0; var < 17; var++) {
    currentHumidityReading = dht.readHumidity();
    if (!isnan(currentHumidityReading)) {
      humidityReadingSum = humidityReadingSum + currentHumidityReading;
      if (currentHumidityReading > maxHumidityReading) {
        maxHumidityReading = currentHumidityReading;
      }
      if (currentHumidityReading < minHumidityReading) {
        minHumidityReading = currentHumidityReading;
      }
    }
  }
  return (humidityReadingSum - maxHumidityReading - minHumidityReading) / 16;
}

/*
  function responsible for proper message display
*/
void simulateLCD() {
  if (isInEditMode) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (isEditingTemperature) {
      lcd.print("Temp set to " + String(setTemperature, 2));
    }
    else {
      lcd.setCursor(0, 1);
      lcd.print("Wil set to " + String(setHumidity, 2) + "%");
    }
  }
  else {
    if (!isInSeccondScreen) {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temp " + String(temperatureReading, 1) + "->" + String(setTemperature, 1));
      lcd.setCursor(0, 1);
      lcd.print("Wil " + String(humidityReading, 1) + "%->" + String(setHumidity, 1) + "%");
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Grzalka" + String(wasHeatingOnLastSent, 2));
      lcd.setCursor(0, 1);
      lcd.print("Pompka" + String(wasHumidifierOnLastSent, 2));
    }
  }
}

float error(float set, float is) {
  float error = set - is;
  return error;
}

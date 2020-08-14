// TODO notes
// + PID - gotowe
// + Wysyłanie pomiarów i zmian stanów - gotowe
// + Histereza (+/- do włącz wyłącz) - gotowe
// + zbieraj 18 pomiarów usuwaj skrajne max, min i licz średnią - gotowe
// + upiększ kod i zoptymalizuj
// + korzystanie z 2 rdzeni - gotowe
// + obsluga ekranu i przyciskow - gotowe

#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
//#include <WebServer.h>
#include "DHT.h"
#include <HTTPClient.h>

//wykorzystane piny
#define heater 23
#define humidifier 15
#define upButton 33
#define downButton 32
#define setButton 25
#define nextButton 26

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

/*Podaj SSID & haslo*/
//const char* ssid = "DEMV";  // Enter SSID here
//const char* password = "Viki$Madzia2";  //Enter Password here
const char* ssid = "KIAE";  // Enter SSID here
const char* password = "K3N4U5F42X";  //Enter Password here

//linki interakcji z serwerem
const char* serverSettingsAdres = "https://esp32-terrarium-control.now.sh/getConfig";
const char* serverNameHumi = "http://192.168.4.1/humidity";
const char* serverNamePres = "http://192.168.4.1/pressure";

// DHT Sensor
uint8_t dhtPin = 4;

// Initialize DHT sensor.
DHT dht(dhtPin, DHTTYPE);

float temperatureReading, sentTemperature, humidityReading, sentHumidity;
volatile double oldSetTemperature, oldSetHumidity, setTemperature, setHumidity;
float pidIntegralTemperature, pidIntegralHumidity, pidDerivativeTemperature, previousTemperature, pidDerivativeHumidity, previousHumidity, pidProportionTemperature, pidProportionHumidity, pidTemperature, pidHumidity;
bool isHeatingCurrentlyOn, wasHeatingOnLastSent, isHumidifierCurrentlyOn, wasHumidifierOnLastSent;
volatile bool isInSeccondScreen = false, isButtonInteractionLocked = false, isInEditMode = false, isEditingTemperature = true, isNewSettingToSend = false;

//button logic
void IRAM_ATTR up() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    if (isInEditMode) {
      if (isEditingTemperature) {
        setTemperature += 0.5;
      }
      else {
        setHumidity += 0.5;
      }
    }
  }
}
void IRAM_ATTR down() {
  if (!isButtonInteractionLocked) {
    isButtonInteractionLocked = true;
    if (isInEditMode) {
      if (isEditingTemperature) {
        setTemperature -= 0.5;
      }
      else {
        setHumidity -= 0.5;
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
const long minRefreshTime = 5*1000, pidRefreshTime = 5*1000, serverRefreshTime = 5*1000;

unsigned long previousServerTime = 0, previousPIDTemperatureTime = 0, previousPIDHumidityTime = 0, deltaTime;

TaskHandle_t Task1;

//Program for core 0
void codeForTask1( void * parameter )
{
  for (;;) {
    unsigned long currentTimeOnCore0 = millis();

    delay (3000);

    //    Temperature and humidity reading
    temperatureReading = getTemperatureFromSensor();
    humidityReading = getHumidityFromSensor();
    Serial.print("Sir! Czytam: ");
    Serial.print(temperatureReading);
    Serial.print(" °C, ");
    Serial.print(humidityReading);
    Serial.print(" %. Różnica: ");
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
    http.begin("https://esp32-terrarium-control.now.sh/config?temp=" + String(setTemperature, 2) + "&wilg=" + String(setHumidity, 2));
    Serial.println("Sending to server temperature set to: " + String(setTemperature, 2) + "and humidity set to: " + String(setHumidity, 2));
    http.GET();
    http.end();
  }
  if (currentTimeOnCore1 - previousServerTime >= serverRefreshTime) {
    previousServerTime = currentTimeOnCore1;
    //  Chcecking connection with WiFi
    if (WiFi.status() == WL_CONNECTED && isInEditMode == false) {
      //   do dopisania zbierania zmiennych ustawien
      String serverReply = httpGETDATA(serverSettingsAdres); //przerabia info z serwera na string
      noInterrupts();
      oldSetTemperature = setTemperature;
      oldSetHumidity = setHumidity;
      setTemperature = getIntX(serverReply, 1);
      setHumidity = getIntX(serverReply, 2);
      interrupts();
      if (oldSetTemperature != setTemperature || oldSetHumidity != setHumidity) {
        simLCD();
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
      simLCD();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("sending heater and humidifier state to server");
        HTTPClient http;
        http.begin("https://esp32-terrarium-control.now.sh/stateChange?grzalka=" + String(wasHeatingOnLastSent ? 1 : 0, 0) + "&pompka=" + String(wasHumidifierOnLastSent ? 1 : 0, 0));
        http.GET();
        http.end();
      }
    }
    //sending temperature and humidity on change
    bool isReadingChanged = sentTemperature != temperatureReading || sentHumidity != humidityReading;
    if (isReadingChanged) {
      sentTemperature = temperatureReading;
      sentHumidity = humidityReading;
      simLCD();
      if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        Serial.println("sending temperature and humidity to server");
        http.begin("https://esp32-terrarium-control.now.sh/reading?temp=" + String(temperatureReading, 2) + "&wilg=" + String(humidityReading, 2));
        http.GET();
        http.end();
      }
    }

  }

  if (isButtonInteractionLocked) {
    noInterrupts();
    simLCD();
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
    Serial.print("Sir! Serwer mowi: ");
    //    Serial.print(httpReply);
    package = http.getString();
  }
  else {
    Serial.print("Sir! Jest cicho, zbyt cicho: ");
    Serial.print(httpReply);
  }
  // Vader release him at once
  http.end();

  return package;
}

/*
* gets the number x int from serverReply
*/
float getIntX(String serverReply, int x) {
  serverReply = serverReply.substring(3, serverReply.length()); //pbcina pierwsze 3 znaki
  char wiadomosc[serverReply.length()]; //tworzymy + przerabiamy na char
  serverReply.toCharArray(wiadomosc, serverReply.length());
  char* zmienna = strtok(wiadomosc, " :\{\"temp,"); //zwraca pierwsza zmienna
  if (x == 1) {
    //    Serial.print(zmienna);
    //    Serial.println(atof("3.4"));
    //    Serial.println(atof(zmienna));
    return atof(zmienna);
  }
  else {
    //    while (Tem != NULL){
    //    Serial.println(Tem);
    zmienna = strtok(NULL, " :\{\"temp,wilg");
    return atof(zmienna);
  }
}


/*
* gets 18 readings of temperature from sensor, eliminates extreme values and returns the average
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
* gets 18 readings of humidity from sensor, eliminates extreme values and returns the average
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

void simLCD() {
  if (isInEditMode == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if (isEditingTemperature == true) {
      // print message
      lcd.print("Temp set to " + String(setTemperature, 2));
    }
    else {
      lcd.setCursor(0, 1);
      lcd.print("Wil set to " + String(setHumidity, 2) + "%");
    }
  }
  else {
    if (isInSeccondScreen == false) {
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
      lcd.print("Temp " + String(temperatureReading, 1) + "->" + String(setTemperature, 1));
      lcd.setCursor(0, 1);
      lcd.print("Wil " + String(humidityReading, 1) + "%->" + String(setHumidity, 1) + "%");
    }
    else {
      lcd.clear();
      lcd.setCursor(0, 0);
      // print message
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

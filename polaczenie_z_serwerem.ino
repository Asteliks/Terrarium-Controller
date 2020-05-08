//notatka co zrobić
// + PID - gotowe
// + Wysyłanie pomiarów i zmian stanów
// + Histereza (+/- do włącz wyłącz) - gotowe
// + zbieraj 18 pomiarów usuwaj skrajne max, min i licz średnią - gotowe
// + upiększ kod i zoptymalizuj
// + korzystanie z 2 rdzeni - gotowe


#include <WiFi.h>
//#include <WebServer.h>
#include "DHT.h"
#include <HTTPClient.h>

//wykorzystane piny
#define heater 23
#define humidifier 22

//nastaw PID dla grzałki
float KpTemp = 0.8;
float KiTemp = 0.001;
float KdTemp = 0.03;

//nastaw PID dla pompki
float KpHum = 2;
float KiHum = 1.5;
float KdHum = 1;

// Uncomment one of the lines below for whatever DHT sensor type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT21   // DHT 21 (AM2301)
//#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

/*Podaj SSID & haslo*/
const char* ssid = "KIAE";  // Enter SSID here
const char* password = "K3N4U5F42X";  //Enter Password here

//linki interakcji z serwerem
const char* serverUstawienia = "https://esp32-terrarium-control.now.sh/getConfig";
const char* serverNameHumi = "http://192.168.4.1/humidity";
const char* serverNamePres = "http://192.168.4.1/pressure";

// DHT Sensor
uint8_t DHTPin = 4;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

float Temperature;
float TemperatureS;
float Humidity;
float HumidityS;
float setTemperature;
float setHumidity;
float ITemp;
float IHum;
float DTemp;
float Tempp; //temperatura poprzednia
float DHum;
float Hump; //wilgotność poprzednia
float ETemp;
float EHum;
float PIDTemp;
float PIDHum;
bool Temp;
bool TempS; //wysłany stan
bool Hum;
bool HumS;  //wysłany stan

//logika czasowa
const long oczekiwanie = 5000; //5000ms
const long oczekiwaniePID = 5000; //5000ms
const long oczekiwanieWiFi = 5000; //5000ms

unsigned long poprzedniCzasWiFi = 0;
unsigned long poprzedniCzasPIDTemp = 0;
unsigned long poprzedniCzasPIDHum = 0;
unsigned long dt;

TaskHandle_t Task1;

//Program na rdzeń 0
void codeForTask1( void * parameter )
{
  for (;;) {
    unsigned long aktualnyCzasCore0 = millis();

    delay (3000);

    //    Pomiar Temperatury
    Temperature = getTemp();
    Humidity = getHum();
    Serial.print("Sir! Czytam: ");
    Serial.print(Temperature);
    Serial.print(" °C, ");
    Serial.print(Humidity);
    Serial.print(" %. Różnica: ");
    Serial.print(error(setTemperature, Temperature));
    Serial.print(" °C, ");
    Serial.print(error(setHumidity, Humidity));
    Serial.print(" %");
    Serial.print("This Task runs on Core: ");
    Serial.println(xPortGetCoreID());

    //    PID dla temperatury
    aktualnyCzasCore0 = millis();
    dt = (aktualnyCzasCore0 - poprzedniCzasPIDTemp) / 1000;
    poprzedniCzasPIDTemp = aktualnyCzasCore0;
    ETemp = error(setTemperature, Temperature);
    ITemp = (ETemp * dt) + ITemp;
    DTemp = (ETemp - Tempp) / dt;
    Tempp = ETemp;
    PIDTemp = (KpTemp * ETemp) + (KiTemp * ITemp) + (KdTemp * DTemp);
    if (PIDTemp > 1) {
      PIDTemp = 1;
      ITemp = ITemp - (ETemp * dt);
    }
    else if (PIDTemp < 0) {
      PIDTemp = 0;
      ITemp = ITemp - (ETemp * dt);
    }
    if (PIDTemp > 0.2) {
      digitalWrite(heater, HIGH);
      Temp = true;
    }
    else if (PIDTemp <= 0.2) {
      digitalWrite(heater, LOW);
      Temp = false;
    }

    //    PID dla wilgotnosci
    aktualnyCzasCore0 = millis();
    dt = (aktualnyCzasCore0 - poprzedniCzasPIDHum) / 1000;
    poprzedniCzasPIDHum = aktualnyCzasCore0;
    EHum = error(setHumidity, Humidity);
    IHum = (EHum * dt) + IHum;
    DHum = (EHum - Hump) / dt;
    Hump = EHum;
    PIDHum = (KpHum * EHum) + (KiHum * IHum) + (KdHum * DHum);
    if (PIDHum > 1) {
      PIDHum = 1;
      IHum = IHum - (EHum * dt);
    }
    else if (PIDHum < 0) {
      PIDHum = 0;
      IHum = IHum - (EHum * dt);
    }
    if (PIDHum > 0.2) {
      digitalWrite(humidifier, HIGH);
      Hum = true;
    }
    else if (PIDHum <= 0.2) {
      digitalWrite(humidifier, LOW);
      Hum = false;
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

  pinMode(DHTPin, INPUT);

  dht.begin();

  Serial.println("Trwa laczenie z ");
  Serial.println(ssid);

  //inicjalizuj polaczenie
  WiFi.begin(ssid, password);

  //wypisuj kropki podczas laczenia
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("o Boi jestem polaczony!");
  Serial.print("a moje IP: ");  Serial.println(WiFi.localIP());

  xTaskCreatePinnedToCore(
    codeForTask1,            /* Task function. */
    "Task_1",                 /* name of task. */
    1000,                    /* Stack size of task */
    NULL,                     /* parameter of the task */
    1,                        /* priority of the task */
    &Task1,                   /* Task handle to keep track of created task */
    0);                       /* Core */

}

//Program na rdzeń 1
void loop() {

  unsigned long aktualnyCzasCore1 = millis();

  if (aktualnyCzasCore1 - poprzedniCzasWiFi >= oczekiwanieWiFi) {
    //  Sprawdz czy sie polaczyles z WiFi
    if (WiFi.status() == WL_CONNECTED) {
      //   do dopisania zbierania zmiennych ustawien
      String odp = httpGETDATA(serverUstawienia); //przerabia info z serwera na string
      setTemperature = getIntX(odp, 1);
      setHumidity = getIntX(odp, 2);
      Serial.print(setTemperature);
      Serial.print(" °C, ");
      Serial.print(setHumidity);
      Serial.print(" %");
      Serial.print("This Task runs on Core: ");
      Serial.println(xPortGetCoreID());

      //wysyłanie stanu grzałki i pompki jeżeli zmiana stanu
      if (TempS != Temp || HumS != Hum) {
        TempS = Temp;
        HumS = Hum;
        Serial.println("wysyłam zmiane grzalki i pompki");
        if (TempS == true && HumS == true) {
          HTTPClient http; //z jakiegoś powodu jak deklarowałem to wcześniej to program był mniej stabilny
          http.begin("https://esp32-terrarium-control.now.sh/stateChange?grzalka=1&pompka=1");
          http.GET();
          http.end();
        }
        else if (TempS == true && HumS == false) {
          HTTPClient http;
          http.begin("https://esp32-terrarium-control.now.sh/stateChange?grzalka=1&pompka=0");
          http.GET();
          http.end();
        }

        else if (TempS == false && HumS == true) {
          HTTPClient http;
          http.begin("https://esp32-terrarium-control.now.sh/stateChange?grzalka=0&pompka=1");
          http.GET();
          http.end();
        }

        else if (TempS == false && HumS == false) {
          HTTPClient http;
          http.begin("https://esp32-terrarium-control.now.sh/stateChange?grzalka=0&pompka=0");
          http.GET();
          http.end();
        }
      }
      // wysyłanie temperatury i wilgotności jeżeli zmiana stanu
      if (TemperatureS != Temperature || HumidityS != Humidity) {
        TemperatureS = Temperature;
        HumidityS = Humidity;
        HTTPClient http;
        Serial.println("wysyłam zmiane temp i wilg");
        http.begin("https://esp32-terrarium-control.now.sh/reading?temp=" + String(Temperature, 2) + "&wilg=" + String(Humidity, 2));
        http.GET();
        http.end();
      }

      //      digitalWrite(heater, HIGH);
      //      digitalWrite(humidifier, HIGH);

    }
  }
  delay(1000);
}

String httpGETDATA(const char* linkSerwera) {
  HTTPClient http;

  //  Vader the time has come
  http.begin(linkSerwera);

  // Do it,... do it now
  int httpOdpowiedz = http.GET();

  String ladunek = "--";

  if (httpOdpowiedz > 0)  {
    Serial.print("Sir! Serwer mowi: ");
    //    Serial.print(httpOdpowiedz);
    ladunek = http.getString();
  }
  else {
    Serial.print("Sir! Jest cicho, zbyt cicho: ");
    Serial.print(httpOdpowiedz);
  }
  // Vader release him at once
  http.end();

  return ladunek;
}

float getIntX(String odp, int x) {
  //  zdobywa int z odp
  odp = odp.substring(3, odp.length()); //pbcina pierwsze 3 znaki
  char wiadomosc[odp.length()]; //tworzymy + przerabiamy na char
  odp.toCharArray(wiadomosc, odp.length());
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

float getTemp() {
  float tempSum;
  float maxTemp;
  float minTemp;
  float x;

  //  pozyskanie pierwszego pomiaru
  while (true) {
    x = dht.readTemperature();
    if (!isnan(x)) { //sprawdza czy odczyt jest poprawny
      break;
    }
  }
  tempSum = x;
  maxTemp = x;
  minTemp = x;
  int var = 0;
  while (var < 17) {
    x = dht.readTemperature();
    if (!isnan(x)) {
      tempSum = tempSum + x;
      if (x > maxTemp) {
        maxTemp = x;
      }
      if (x < minTemp) {
        minTemp = x;
      }
      var++;
    }
  }
  return (tempSum - maxTemp - minTemp) / 16;
}

float getHum() {
  float humSum;
  float maxHum;
  float minHum;
  float x;

  //  pozyskanie pierwszego pomiaru
  while (true) {
    x = dht.readHumidity();
    if (!isnan(x)) { //sprawdza czy odczyt jest poprawny
      break;
    }
  }
  humSum = x;
  maxHum = x;
  minHum = x;
  int var = 0;
  while (var < 17) {
    x = dht.readHumidity();
    if (!isnan(x)) {
      humSum = humSum + x;
      if (x > maxHum) {
        maxHum = x;
      }
      if (x < minHum) {
        minHum = x;
      }
      var++;
    }
  }
  return (humSum - maxHum - minHum) / 16;
}

float error(float set, float is) {
  float error = set - is;
  return error;
}

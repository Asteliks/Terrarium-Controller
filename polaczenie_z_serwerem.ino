#include <WiFi.h>
//#include <WebServer.h>
#include "DHT.h"
#include <HTTPClient.h>

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
float Humidity;
float setTemperature;
float setHumidity;

unsigned long poprzedniCzas = 0;
const long oczekiwanie = 5000; //5000ms

void setup() {
  Serial.begin(115200);
  delay(100);

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

}
void loop() {

  unsigned long aktualnyCzas = millis();

  if (aktualnyCzas - poprzedniCzas >= oczekiwanie) {
    //  Sprawdz czy sie polaczyles z WiFi
    if (WiFi.status() == WL_CONNECTED) {
      //   do dopisania zbierania zmiennych ustawien
      String odp = httpGETDATA(serverUstawienia); //przerabia info z serwera na string
      setTemperature = getIntX(odp,1);
      setHumidity = getIntX(odp,2);
      Serial.print(setTemperature);
      Serial.print(" *C, ");
      Serial.print(setHumidity);
      Serial.println(" %");
//      odp = odp.substring(3, odp.length()); //pbcina pierwsze 3 znaki
//      char dupajasiu[odp.length()]; //tworzymy + przerabiamy na char
//      odp.toCharArray(dupajasiu, odp.length());
//      char* Tem = strtok(dupajasiu, " :\{\"temp,"); //zwraca pierwsza zmienna
//      while (Tem != NULL) {
//        Serial.println(Tem);
//        Tem = strtok(NULL, " :\{\"temp,wilg");
//      }


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

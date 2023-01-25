#include "DHT.h"
#include <Arduino.h>
#include <FastLED.h>
#include "Freenove_WS2812_Lib_for_ESP32.h"
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"
#include <IRremote.h>
#include <TimeLib.h>


#define NUM_LEDS 8
CRGB leds[NUM_LEDS];

const int trigPin = 18;
const int echoPin = 19;

unsigned long previousMillis = 0;  // variable to store the last time the task was run
unsigned long previousMillisRecieve = 0;  // variable to store the last time the task was run
const long interval = 2000;       // interval at which to run the task (in milliseconds)
const long intervalRecieve = 100;       // interval at which to run the task (in milliseconds)

#define LED 2
#define DHTPIN 4    
#define FANPIN 5
#define DHTTYPE DHT11   
#define PIN_ANALOG_IN 35
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701


#define WIFI_SSID "MikroTik-WV"
// #define WIFI_SSID "Wifi-ValDesRoses"
#define WIFI_PASSWORD "BrugRijk"
// #define WIFI_PASSWORD "2ValDesRoses"
#define On_Board_LED 2
#define API_KEY "AIzaSyBQs225KxT8eXfOCw3RHuW_VoTfYkPtc7Y"
#define DATABASE_URL "https://arduino-b2dd1-default-rtdb.europe-west1.firebasedatabase.app/" 

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
const long sendDataIntervalMillis = 4000; //--> Sends/stores data to firebase database every 10 seconds.
bool signupOK = false;

float store_random_Float_Val;
int store_random_Int_Val;
int store_random_Temp_Val;

bool rgbStrip = false;
bool sensorEnabled = true;
int calcdistance[3] = {0,0,0};
unsigned long lastUpdate = 0;


int RECV_PIN = 21;

IRrecv irrecv(RECV_PIN);

decode_results results;


DHT dht(DHTPIN, DHTTYPE);




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  FastLED.addLeds<WS2812, 23, GRB>(leds, NUM_LEDS); 
  pinMode(LED, OUTPUT);
  pinMode(FANPIN, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  Serial.println(F("DHTxx test!"));
  irrecv.enableIRIn(); // Start the receiver

  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("---------------Connection");
  Serial.print("Connecting to : ");
  Serial.println(WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");

    digitalWrite(On_Board_LED, HIGH);
    delay(250);
    digitalWrite(On_Board_LED, LOW);
    delay(250);
  }
  digitalWrite(On_Board_LED, LOW);
  Serial.println();
  Serial.print("Successfully connected to : ");
  Serial.println(WIFI_SSID);
  Serial.println("---------------");
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  // Sign up.
  Serial.println();
  Serial.println("---------------Sign up");
  Serial.print("Sign up new user... ");
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");
    signupOK = true;
  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  Serial.println("---------------");
  
  // Assign the callback function for the long running token generation task.
  config.token_status_callback = tokenStatusCallback; //--> see addons/TokenHelper.h
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

    for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Black;
         }
   FastLED.show();
   Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 0) ? "ok" : fbdo.errorReason().c_str());

}


void controlLED(int x){
   Serial.println(x + " LED");
          for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Black;
         }
        for (int i = NUM_LEDS - x; i < NUM_LEDS; i++) {
          leds[i] = CRGB::White;
        }
        FastLED.show();
}



void loop() {


  unsigned long currentMillis = millis();
  unsigned long currentMillisRecieve = millis();


  if (currentMillis - previousMillis >= interval) {
    // run the task
 
  // put your main code here, to run repeatedly:

  float h = dht.readHumidity();
  float t = dht.readTemperature();
  // float f = dht.readTemperature(true);
  // float hif = dht.computeHeatIndex(f, h);
  // float hic = dht.computeHeatIndex(t, h, false);

  int adcValue = analogRead(PIN_ANALOG_IN);                       //read ADC pin
  double voltage = (float)adcValue / 4095.0 * 3.3;                // calculate voltage
  double Rt = 10 * voltage / (3.3 - voltage);                     //calculate resistance value of thermistor
  double tempK = 1 / (1 / (273.15 + 25) + log(Rt / 10) / 3950.0); //calculate temperature (Kelvin)
  double tempC = tempK - 273.15;      
  long duration;
  float distanceCm;
  float distanceInch;                            //calculate temperature (Celsius)




  if (isnan(h) || isnan(t)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  if(h > 35){
    digitalWrite(FANPIN, HIGH);
    Serial.println("Fan is on");
  }
  else{
    digitalWrite(FANPIN, LOW);
    Serial.println("Fan is off");
  }

  Serial.printf("Humidity : %.2f%%\tTemperature : %.2fC\n", h, t);

  Serial.printf("ADC value : %d,\tVoltage : %.2fV, \tTemperature : %.2fC\n", adcValue, voltage, tempC);



// Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculate the distance
  distanceCm = duration * SOUND_SPEED/2;
  
  // Convert to inches
  distanceInch = distanceCm * CM_TO_INCH;
  
  // Prints the distance in the Serial Monitor

  if(sensorEnabled){
    if(distanceCm < 240 && distanceCm > 5){
      Serial.print("Distance (cm): ");
      Serial.println(distanceCm);
      Serial.print("Distance (inch): ");
      Serial.println(distanceInch);
  
      if(rgbStrip){
       if(distanceCm <= 30){
        controlLED(1);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 1) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 60){
          controlLED(2);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 2) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 90){
          controlLED(3);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 3) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 120){
          controlLED(4);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 4) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 150){
          controlLED(5);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 5) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 180){
          controlLED(6);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 6) ? "ok" : fbdo.errorReason().c_str());
        }else if(distanceCm <= 210){
          controlLED(7);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 7) ? "ok" : fbdo.errorReason().c_str());
        }else{
          controlLED(8);
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 8) ? "ok" : fbdo.errorReason().c_str());
        }
      lastUpdate = millis();
      } 
    }
  } 
    
  

   
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > sendDataIntervalMillis || sendDataPrevMillis == 0)){
    sendDataPrevMillis = millis();
    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/debugValue"), random(10, 99) + 10.2) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/temp"), t) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/humidity"), h) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/plateTemp"), tempC) ? "ok" : fbdo.errorReason().c_str());

    Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/distanceCm"), distanceCm) ? "ok" : fbdo.errorReason().c_str());

  }

   

     previousMillis = currentMillis;
  }

  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    if(results.value == 0xFFA25D){
      lastUpdate = now();
      Serial.println(lastUpdate);
      if(rgbStrip == false){
        rgbStrip = true;
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/rgbStripStatus"), rgbStrip) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 8) ? "ok" : fbdo.errorReason().c_str());
          Serial.println("RGB Strip is on");
          for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::White;
          }
        FastLED.show();
      }else{
        rgbStrip = false;
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/rgbStripStatus"), rgbStrip) ? "ok" : fbdo.errorReason().c_str());
        Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 0) ? "ok" : fbdo.errorReason().c_str());
        Serial.println("RGB Strip is off");
          for (int i = 0; i < NUM_LEDS; i++) {
          leds[i] = CRGB::Black;
         }
        FastLED.show();
      }
    }
    if(rgbStrip == true){
      if(results.value == 0xFF30CF){
       controlLED(1);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 1) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF18E7){
        controlLED(2);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 2) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF7A85){
        controlLED(3);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 3) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF10EF){
        controlLED(4);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 4) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF38C7){
        controlLED(5);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 5) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF5AA5){
        controlLED(6);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 6) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF42BD){
        controlLED(7);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 7) ? "ok" : fbdo.errorReason().c_str());
      }
      if(results.value == 0xFF4AB5){
        controlLED(8);
       Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 8) ? "ok" : fbdo.errorReason().c_str());
      }
    }
    if(results.value == 0xFFA857){
      sensorEnabled ? sensorEnabled = false : sensorEnabled = true;
      Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/sensorStatus"), sensorEnabled) ? "ok" : fbdo.errorReason().c_str());
    }
    
    irrecv.resume(); // Receive the next valu
  }

  if (currentMillisRecieve - previousMillisRecieve >= intervalRecieve) {
    if (Firebase.RTDB.getInt(&fbdo, "/values/rgbStripStatus")) {

      if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
        int intValue = fbdo.intData();
        if(intValue != rgbStrip){
          if(intValue == 1){
            rgbStrip = true;
            Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 8) ? "ok" : fbdo.errorReason().c_str());
            for (int i = 0; i < NUM_LEDS; i++) {
              leds[i] = CRGB::White;
            }
          }else{
            rgbStrip = false;
            Serial.printf("Set float... %s\n", Firebase.RTDB.setFloat(&fbdo, F("/values/modus"), 0) ? "ok" : fbdo.errorReason().c_str());
            for (int i = 0; i < NUM_LEDS; i++) {
              leds[i] = CRGB::Black;
            }
          }
          FastLED.show();
        }
      }

    } else {
      Serial.println(fbdo.errorReason());
    }
     if (Firebase.RTDB.getInt(&fbdo, "/values/modus")) {

      if (fbdo.dataTypeEnum() == fb_esp_rtdb_data_type_integer) {
        int intValue = fbdo.intData();
        if(rgbStrip){
          controlLED(intValue);
        }
 
      }

    } else {

    }
    previousMillisRecieve = currentMillisRecieve;
  }


  
}


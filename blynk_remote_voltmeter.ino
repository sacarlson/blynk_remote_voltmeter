/*************************************************************
  Download latest Blynk library here:
    https://github.com/blynkkk/blynk-library/releases/latest

  Blynk is a platform with iOS and Android apps to control
  Arduino, Raspberry Pi and the likes over the Internet.
  You can easily build graphic interfaces for all your
  projects by simply dragging and dropping widgets.

    Downloads, docs, tutorials: http://www.blynk.cc
    Sketch generator:           http://examples.blynk.cc
    Blynk community:            http://community.blynk.cc
    Follow us:                  http://www.fb.com/blynkapp
                                http://twitter.com/blynk_app

  Blynk library is licensed under MIT license
  This example code is in public domain.

 *************************************************************
  This example runs directly on NodeMCU.

  Note: This requires ESP8266 support package:
    https://github.com/esp8266/Arduino

  Please be sure to select the right NodeMCU module
  in the Tools -> Board menu!

  For advanced settings please follow ESP examples :
   - ESP8266_Standalone_Manual_IP.ino
   - ESP8266_Standalone_SmartConfig.ino
   - ESP8266_Standalone_SSL.ino

  Change WiFi ssid, pass, and Blynk auth token to run :)
  Feel free to apply it to any other example. It's simple!
 *************************************************************/

/* Comment this out to disable prints and save space */

// This project is a simple remote voltage monitor to allow monitoring of remote battery installations or other.
// it uses blynk (legacy) as the user interface to view output from an android or apple device
// it uses old code from another older project of hydroponics that parts still remain but not used any more
// I may later add to this code to setup voltage trigers to turn off and on other devices using bridges
// to allow turning off water pumps or other high current devices when voltage get's too low
// and later turn them back on if voltage gets above another triger on level but that is future thoughts
// just a nodempu esp8266 is used in this project.  later I might port this to a esp32 to allow more than one input voltage to be monitored.

// also we selected the ESP8266 NodeMCU 1.0 (ESP-12E Module) for this project
// it is also setup to allow OTA Over The Air updates
// you need to setup a blynk server like I did for this project as blynk no longer supports legacy version of Blynk.

#define BLYNK_PRINT Serial


#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <BlynkSimpleEsp8266.h>

#define VoltSensorPin A0

// for voltage accuracy at low end less than 1.7v set to 3.15 
#define VREF 3.15
// for accuracy on high over 2.0V end set vref to 3.295
//#define VREF 3.295
// adc in this esp8266 looks to be not perfectly linear  

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).
char auth[] = "........5AJ";

char* ssid[] = {"wifinet1","wifinet2","wifinet3"}; //list of wifi networks
char* pass[] = {"password_net1","password_net2","password_net3"}; //list of passwords

BlynkTimer timer;
WidgetLED led1(V10);  // might later use to indicate voltage triger low state active
WidgetLED led2(V11);  // might later use to indicate voltage trigger high state active


int ledstate = 1;
// default calfactor value
float calfactor = 21.2;

void myTimerEvent(){
  float volt = 0;
  float calib_volt = 0;
  ledstate = !ledstate;
  digitalWrite(LED_BUILTIN, ledstate);
  volt = get_voltage(VoltSensorPin);
  Blynk.virtualWrite(V0, volt);       
  calib_volt = volt_to_calib_volt(volt);
  Blynk.virtualWrite(V1, calib_volt); 
  Serial.println("timer event.. ");
  if (ledstate == 0){
    led1.off();
  }else{
    led1.on();
  }
}


//  used  to get voltage calibration factor entry from app
BLYNK_WRITE(V3){
  // v3 returns calfactor entry to apply if default is not good enough
  calfactor = param.asFloat();
}



void led_set(int led, int onoff){
  //Serial.println("onoff");
  //Serial.println(onoff);
  //Serial.println("led");
  //Serial.println(led);
  // led on active 0
  onoff = !onoff;
  switch( led){
    case 1:
      if (onoff == 0){
        led1.on();
      }else{
        led1.off();
      }
      break;
    case 2:
      if (onoff == 0){
        led2.on();
      }else{
        led2.off();
      }
      break;
   } 
}


float get_average_adc(int pin){
   int measurings=0;
   int samples = 30;
    for (int i = 0; i < samples; i++)
    {
        measurings += analogRead(pin);
        delay(10);
    }
    float av =  measurings/samples;  
    return av;   
}

float adc_to_volt( float av){
   float voltage = VREF / 1024.0 * av;  
   return voltage; 
}

float get_voltage(int pin){
  float av = get_average_adc(pin);
  return adc_to_volt(av);
}

float volt_to_calib_volt(float volt){
  float calib_v = volt * calfactor;
  return calib_v;
}


void setup()
{
  // Debug console
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // pin2 also = D4 aka LED_BUILTIN  that is built in led on esp8266
  pinMode(A0, INPUT);

  MultyWiFiBlynkBegin(); //instead Blynk.begin(auth, ssid, pass);  

  Blynk.virtualWrite(V3,calfactor);
 
  ArduinoOTA.setHostname("blynk_voltmeter");
  ArduinoOTA.setPassword("password_OTA");
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  timer.setInterval(1000L, myTimerEvent);
}

void MultyWiFiBlynkBegin() {
  int ssid_count=0;
  int ssid_mas_size = sizeof(ssid) / sizeof(ssid[0]);
  do {
    Serial.println("Trying to connect to wi-fi " + String(ssid[ssid_count]));
    WiFi.begin(ssid[ssid_count], pass[ssid_count]);    
    int WiFi_timeout_count=0;
    while (WiFi.status() != WL_CONNECTED && WiFi_timeout_count<50) { //waiting 10 sec
      delay(200);
      Serial.print(".");
      ++WiFi_timeout_count;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to WiFi! Now I will check the connection to the Blynk server");
      Blynk.config(auth,"www.funtracker.site", 8080);
      //Blynk.begin(auth, ssid, pass, "www.funtracker.site", 8080);
      Blynk.connect(10000); //waiting 10 sec
    }
    ++ssid_count; 
  }
  while (!Blynk.connected() && ssid_count<ssid_mas_size);
    if (!Blynk.connected() && ssid_count==ssid_mas_size) {
    Serial.println("I could not connect to blynk =( Ignore and move on. but still I will try to connect to wi-fi " + String(ssid[ssid_count-1]));
  }
}



void reset_wifi_notconnect(){
  if (WiFi.status() != WL_CONNECTED){
    Serial.println("detected wifi desconnect will reset esp");
    ESP.restart();
  }
}

void loop()
{
  Blynk.run();
  timer.run(); // Initiates BlynkTimer
  reset_wifi_notconnect();
  ArduinoOTA.handle();
}

// Include the ESP8266 WiFi library. (Works a lot like the
// Arduino WiFi library.)
#include <ESP8266WiFi.h>
// Include the SparkFun Phant library.
#include <Phant.h>
#include <Wire.h>
#include <SPI.h>
#include <SFE_MicroOLED.h>
#include "SFE_ISL29125.h"

//////////////////////////
// MicroOLED Defintions //
//////////////////////////
#define PIN_RESET 4
#define DC_JUMPER 1
int postBoxX = 0;
int waitX = 0;

MicroOLED oled(PIN_RESET, DC_JUMPER);

//////////////////////
// WiFi Definitions //
//////////////////////
const char WiFiSSID[] = "RestDevices";
const char WiFiPSK[] = "dunkindecaf";

/////////////////////
// Pin Definitions //
/////////////////////
const int LED_PIN = 5; // Thing's onboard, green LED
const int ANALOG_PIN = A0; // The only analog pin on the Thing
const int DIGITAL_PIN = 12; // Digital pin to be read
SFE_ISL29125 RGB_sensor;

////////////////
// Phant Keys //
////////////////
const char PhantHost[] = "data.sparkfun.com";
const char PublicKey[] = "6Jqg46d7lRtajXoObJLy";
const char PrivateKey[] = "Ww5oZXVp1nsBGAD18xw9";

/////////////////
// Post Timing //
/////////////////
const unsigned long postRate = 30000;
unsigned long lastPost = 0;


void setup() 
{
  initOLED();
  initHardware();
  connectWiFi();
  digitalWrite(LED_PIN, HIGH);
}

void loop() 
{
  if (lastPost + postRate <= millis())
  {
    if (postToPhant()) {
      lastPost = millis();
      drawPosted();
    }
    else {
      delay(100);
    }    
  }
  else {
    drawWaiting();
    delay(500);
  }
}

void initOLED()
{
  oled.begin();
  oled.clear(PAGE);
  oled.display();
}

void drawPosted()
{
  // Draw a rectangle at x to show a successful post
  oled.clear(PAGE);
  oled.rect(postBoxX, 4, 4, 4);
  oled.display();
  postBoxX = postBoxX + 4;
  if (postBoxX > 60) {postBoxX = 0;}
  waitX = 0;
}

void drawWaiting()
{
  // Draw a dot to indicate waiting
  oled.pixel(waitX, 10);
  oled.display();
  waitX = waitX + 1;
  if (waitX > 64) {waitX = 0;}
}


void initHardware()
{
  pinMode(DIGITAL_PIN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  // Don't need to set ANALOG_PIN as input, 
  // that's all it can be.
  RGB_sensor.init();
}

void connectWiFi()
{
  byte ledStatus = LOW;

  // Set WiFi mode to station (as opposed to AP or AP_STA)
  WiFi.mode(WIFI_STA);

  // WiFI.begin([ssid], [passkey]) initiates a WiFI connection
  // to the stated [ssid], using the [passkey] as a WPA, WPA2,
  // or WEP passphrase.
  WiFi.begin(WiFiSSID, WiFiPSK);

  // Use the WiFi.status() function to check if the ESP8266
  // is connected to a WiFi network.
  while (WiFi.status() != WL_CONNECTED)
  {
    // Blink the LED
    digitalWrite(LED_PIN, ledStatus); // Write LED high/low
    ledStatus = (ledStatus == HIGH) ? LOW : HIGH;

    // Delays allow the ESP8266 to perform critical tasks
    // defined outside of the sketch. These tasks include
    // setting up, and maintaining, a WiFi connection.
    delay(100);
    // Potentially infinite loops are generally dangerous.
    // Add delays -- allowing the processor to perform other
    // tasks -- wherever possible.
  }
}

int postToPhant()
{

  // Declare an object from the Phant library - phant
  Phant phant(PhantHost, PublicKey, PrivateKey);

  phant.add("r", RGB_sensor.readRed());
  phant.add("g", RGB_sensor.readGreen());
  phant.add("b", RGB_sensor.readBlue());
  phant.add("extra", analogRead(ANALOG_PIN));
  phant.add("random", digitalRead(DIGITAL_PIN));
  phant.add("location", "My Desk");

  // Now connect to data.sparkfun.com, and post our data:
  WiFiClient client;
  const int httpPort = 80;
  if (!client.connect(PhantHost, httpPort)) 
  {
    // If we fail to connect, return 0.
    digitalWrite(LED_PIN, LOW);
    return 0;
  }
  // If we successfully connected, print our Phant post:
  client.print(phant.post());

  // Read all the lines of the reply from server and print them to Serial
  while(client.available()){
    String line = client.readStringUntil('\r');
    //Serial.print(line); // Trying to avoid using serial
  }

  // If successful, turn the LED on.
  digitalWrite(LED_PIN, HIGH);

  return 1; // Return success
}

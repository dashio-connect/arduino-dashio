  // Arduino with Adafruit Bluefruit LE SPI Friend and DHT Temperature and Humidity Sensor

#include <Adafruit_BLE.h>
#include <Adafruit_BluefruitLE_SPI.h>
//#include <Adafruit_BluefruitLE_UART.h>
#include <SPI.h>
#include <DHT.h>
#include <timer.h>
#include "bluefruitConfig.h"
#include "DashIO.h"

// Temperature and humidity sensor
#define DHTPIN 2 // pin that temp and humid sensor is connected to
// Uncomment whatever type you're using!
//#define DHTTYPE DHT11 // DHT 11 
#define DHTTYPE DHT22 // DHT 22  (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

#define DEVICE_TYPE "TempHumidBLEType"
#define DEVICE_NAME "TempHumidBLE"

// Create device
const DashDevice myDevice;

// Create Connection
const DashConnection myBLEconnection(BLE_CONN);

// Create controls. This simple example only requires the controlID for each control
const String TEMPERATURE_CONTROL_ID = "TEMP01";
const String HUMIDITY_CONTROL_ID = "HUMID01";
const String BUTTON_CONTROL_ID = "FBUT01";
bool fahrenheit = off; // Used for the button control

// Global variables used throughout
auto timer = timer_create_default();
DHT dht(DHTPIN, DHTTYPE);

/*=========================================================================
    APPLICATION SETTINGS

    FACTORYRESET_ENABLE       Perform a factory reset when running this sketch
   
                              Enabling this will put your Bluefruit LE module
                              in a 'known good' state and clear any config
                              data set in previous sketches or projects, so
                              running this at least once is a good idea.
   
                              When deploying your project, however, you will
                              want to disable factory reset by setting this
                              value to 0.  If you are making changes to your
                              Bluefruit LE device via AT commands, and those
                              changes aren't persisting across resets, this
                              is the reason why.  Factory reset will erase
                              the non-volatile memory where config data is
                              stored, setting it back to factory default
                              values.
       
    MINIMUM_FIRMWARE_VERSION  Minimum firmware version to have some new features
    
    MODE_LED_BEHAVIOUR        LED activity, valid options are
                              "DISABLE" or "MODE" or "BLEUART" or
                              "HWUART"  or "SPI"  or "MANUAL"
    -----------------------------------------------------------------------*/
    #define FACTORYRESET_ENABLE         0
    #define MINIMUM_FIRMWARE_VERSION    "0.6.6"
    #define MODE_LED_BEHAVIOUR          "MODE"
/*=========================================================================*/

// Create the bluefruit object, either software serial...uncomment these lines
// SoftwareSerial bluefruitSS = SoftwareSerial(BLUEFRUIT_SWUART_TXD_PIN, BLUEFRUIT_SWUART_RXD_PIN);

// Adafruit_BluefruitLE_UART ble(bluefruitSS, BLUEFRUIT_UART_MODE_PIN, BLUEFRUIT_UART_CTS_PIN, BLUEFRUIT_UART_RTS_PIN);

// ...or hardware serial, which does not need the RTS/CTS pins. Uncomment this line
// Adafruit_BluefruitLE_UART ble(BLUEFRUIT_HWSERIAL_NAME, BLUEFRUIT_UART_MODE_PIN);

// ...hardware SPI, using SCK/MOSI/MISO hardware SPI pins and then user selected CS/IRQ/RST
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_CS, BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);

// ...software SPI, using SCK/MOSI/MISO user-defined SPI pins and then user selected CS/IRQ/RST
/*
Adafruit_BluefruitLE_SPI ble(BLUEFRUIT_SPI_SCK, BLUEFRUIT_SPI_MISO,
                             BLUEFRUIT_SPI_MOSI, BLUEFRUIT_SPI_CS,
                             BLUEFRUIT_SPI_IRQ, BLUEFRUIT_SPI_RST);
*/
  

static void bleWriteStr(String message) {
  if (ble.isConnected()) {
    ble.print(message);
    ble.flush();

    Serial.println(F("**** Send ****"));
    Serial.println(message);
  }
}

bool onTimerCallback() {
// onTimerCallback is not an interrupt, so it is safe to read data and send messages from within.
// Reading temperature or humidity takes about 250 milliseconds!
// Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
// check if returns are valid, if they are NaN (not a number) then something went wrong!

  float t = dht.readTemperature(fahrenheit);
  if (isnan(t)) {
    Serial.println(F("DHT read fail"));
  } else {
    bleWriteStr(myDevice.getTextBoxMessage(TEMPERATURE_CONTROL_ID, String(t)));
  }

  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println(F("DHT read fail"));
  } else {
    bleWriteStr(myDevice.getTextBoxMessage(HUMIDITY_CONTROL_ID, String(h)));
  }

  return true; // to repeat the timer action - false to stop
}

String getTemperatureSymbol() {
  String symbol;
  if (fahrenheit) {
    symbol = "F";
  } else {
    symbol = "°C";
  }
  return symbol;
}
  
// Process incoming control mesages
void processStatus() {
  // Control initial condition messages (recommended)
  bleWriteStr(myDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
}

void processConfig() {
    // Not enough code space for full config, so just do a basic config
    String configData = myDevice.getBasicConfigData(textBox, TEMPERATURE_CONTROL_ID, "Temperature");
    configData += myDevice.getBasicConfigData(textBox, HUMIDITY_CONTROL_ID, "Humidity");
    configData += myDevice.getBasicConfigData(button, BUTTON_CONTROL_ID, "°C/F");
    bleWriteStr(myDevice.getBasicConfigMessage(configData));
}

void processIncomingMessage() {
  switch (myBLEconnection.control) {
    case who:
      bleWriteStr(myDevice.getWhoMessage(DEVICE_NAME, DEVICE_TYPE));
      break;
    case connect:
      bleWriteStr(myDevice.getConnectMessage());
      break;
    case status:
      processStatus();
      break;
    case config:
      processConfig();
      break;
    case button:
      if (myBLEconnection.idStr == BUTTON_CONTROL_ID) {
        fahrenheit = !fahrenheit;
        bleWriteStr(myDevice.getButtonMessage(BUTTON_CONTROL_ID, fahrenheit, "", getTemperatureSymbol()));
      }
      break;
  }
}

void checkIncomingMessages() {
  while(ble.available()) {
    char data;
    data = (char)ble.read(); // Read individual characters.

    if (myBLEconnection.processChar(data)) {
      Serial.println(F("**** Received ****"));
      Serial.print(myDevice.getControlTypeStr(myBLEconnection.control) + " " + myBLEconnection.idStr);
      Serial.println(" " + myBLEconnection.payloadStr);
      processIncomingMessage();
    }
  }
}
  
void setupBLE() {
  // Initialise the module
  Serial.print(F("Initialising Bluefruit LE module: "));
  if (!ble.begin(VERBOSE_MODE)) {
    Serial.print(F("Couldn't find Bluefruit, make sure it's in CoMmanD mode & check wiring"));
    while(1);
  } else {
    Serial.println(F("OK!"));
  }

  // Perform a factory reset to make sure everything is in a known state
  if (FACTORYRESET_ENABLE) {
    Serial.println(F("Performing a factory reset: "));
    if (!ble.factoryReset()) {
      Serial.print(F("Couldn't factory reset"));
      while(1);
    }
  }
  
  ble.echo(false); // Disable command echo from Bluefruit
  Serial.println("Requesting Bluefruit info:");
  ble.info(); // Print Bluefruit information
  ble.verbose(false);  // debug info is a little annoying after this point!

  // LED Activity command is only supported from 0.6.6
  if ( ble.isVersionAtLeast(MINIMUM_FIRMWARE_VERSION) ) {
    // Change Mode LED Activity
    Serial.println(F("Change LED activity to " MODE_LED_BEHAVIOUR));
    ble.sendCommandCheckOK("AT+HWModeLED=" MODE_LED_BEHAVIOUR);
  }

  // Get deviceID from mac address
  while(ble.available() > 0) {ble.read();} // But first, clear the serial.
  ble.println(F("AT+BLEGETADDR"));
  delay(1000); // Wait a while for reply
  myDevice.deviceID = "";
  while(ble.available() > 0) {
    char c = ble.read();
    if ((c == '\n') || (c == '\r')) { // Before the OK
      break;
    }
    myDevice.deviceID += c;
  }
  Serial.print(F("DeviceID: "));
  Serial.println(myDevice.deviceID);

  // Set peripheral name
  Serial.println(F("Set peripheral name to " DEVICE_TYPE));
  ble.sendCommandCheckOK(F("AT+GAPDEVNAME=" DEVICE_TYPE));

  // Set Bluefruit to DATA mode
  Serial.println(F("Switching to DATA mode!"));
  ble.setMode(BLUEFRUIT_MODE_DATA);
  
  Serial.println();
}

void setup(void) {
  Serial.begin(115200);

  setupBLE();
  timer.every(1000, onTimerCallback); // 1000ms
  dht.begin();
}

void loop(void)
{
  timer.tick();
  checkIncomingMessages();
}

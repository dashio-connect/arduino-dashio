/*
 * Simple dash demonstration project for a SparkFun ESP32 Thing Plus DMX to LED Shield with 
 * an RGB LED light. Communicates via TCP connection only. A BLE connection could be added 
 * for provisioning but shoulw use a switch to change to TCP only once provisioning is done.
 * This is because BLE and DMX together use most pof the ESP capability. However, this may 
 * change in the future when faster ESP32s become available.
 * Demonstrates the use of the CololPicker control.
 * Uses the serial monitor to show what is going on (115200 baud).
 */

#include "DashioESP.h"
#include <SparkFunDMX.h>

#define DEVICE_TYPE "ESP32_DMX"
#define DEVICE_NAME "Basic DMX"

// WiFi
#define WIFI_SSID      "yourWiFiSSID"
#define WIFI_PASSWORD  "yourWiFiPassword"

const PROGMEM char *configStr = 
"xVVdc6IwFP0rTl6X6RbtuF3fBCzSIihQ3NmPB5SoWTHphGBlO/73vYHQ9WunfdqFEU/uvbnJPfcEXpBruKj37YeGHjzfqFHoWkGN"
"osGXqEaWo0yj/liBgfdYI9e3a9CPrSaDGal405XgBZVjlhNBGA0SeKLetYaeSSpWaqhrSBCR4SYK9ZDnewOkoaeEYyocyIysWAdD"
"UZAUBrf+aviwzD9H3z/6LL3Xo6/lfTecQMCcUcFZVk0x5YwqMwyi6h9Skvka81CUlXU6HAxcsOaYpj7NSp8GOMNJDr5FkuVYQ7vz"
"va8wWa5EM77qXsOt7yVRcTytCs5VeqMfOqbcleDZKNndweZC8gs8ehvyLDlJTZYVG5qjXhtYyFdQb21BPcELWJ4Wm9cQ/bi8mhGZ"
"2mA8xRziGJeLZsl83XjWS5o2jumKCKwcjc0im5bNk/IwPuIJzSvq52VV8FwF2wmh+YxxdoFXObvCBts1yUcJZ9DOY+dZdklDwJ6h"
"wE7nIPSArK6GCFTuJRu5oscoRhXdTl/p1xwGSquRHYyHNTSiyKu6oUQDIu8XvPg5YRvf8gpjFJsTvpTNV90aOvbQhV8ENrZYNFXY"
"HGN6Ii2jDYZZIQSjA5rMMpw2DWP0tA9nIj7RT/tG3ueM/uVMnB0mJUCYgHdCLmL46PiAXV91quuCml89BwSbnOU52muvzN1vb21/"
"Qv3JZ3ITFx8eZyE2rXcw58o6W+G6bBlZgU8p1N+m0AetLHErAN+/5/H9JF6Q5x/yxjcP+I46TrBrT8ei67Sn2Esmb5N3UXadtzkL"
"SbbF/D/w1c+ylu+9n7Wrrrw+HZEXwdu5OttGZKtvjXkHX5gXlOItmeMQi+JJxpljVL0crcocE/ysXpCLZYC3APf73w==";

DashioDevice dashDevice(DEVICE_TYPE, configStr, 1);

// DashIO comms connections
DashWiFi wifi;
DashTCP  tcp_con(&dashDevice, true);

// Create Control IDs
const char *COLOR_PICKER = "C1";
const char *AUTO_BUTTON = "B1";
const char *DBO_BUTTON = "B2";
const char *ALL_ON_BUTTON = "B3";

HardwareSerial dmxSerial(2);
uint8_t enPin = 21; // Enable pin for DMX shield (Free pin on Thing Plus or Feather pinout)
uint16_t numChannels = 7; // Number of DMX channels, can be up tp 512

SparkFunDMX dmx;
bool tick = false;
int angle = 0;
bool autoButton = true;
int count = 0;

void processStatus(ConnectionType connectionType) {
    String message((char *)0);
    message.reserve(1024);

    message = dashDevice.getButtonMessage(AUTO_BUTTON, autoButton, "pause", "Manual");
    message += dashDevice.getButtonMessage(DBO_BUTTON, true);
    message += dashDevice.getButtonMessage(ALL_ON_BUTTON, true);
    tcp_con.sendMessage(message);
}

void sendAutoButton() {
    String text;
    String icon;
    if (autoButton) {
        text = "Manual";
        icon = "pause";
    } else {
        text = "Auto";
        icon = "play";
    }
    tcp_con.sendMessage(dashDevice.getButtonMessage(AUTO_BUTTON, autoButton, icon, text));
}

void processButton(MessageData *messageData) {
    if (messageData->idStr == AUTO_BUTTON) {
        autoButton = !autoButton;
        sendAutoButton();
    } else if (messageData->idStr == DBO_BUTTON) {
        autoButton = false;
        writeDMXcolor(0, 0, 0); // Blackout
        sendColorHex(0, 0, 0);
        sendAutoButton();
    } else if (messageData->idStr == ALL_ON_BUTTON) {
        autoButton = false;
        writeDMXcolor(255, 255, 255); // All On
        sendColorHex(255, 255, 255);
        sendAutoButton();
    }
}

void processColorPicker(MessageData *messageData) {
    autoButton = false;
    sendAutoButton();

    char charbuf[8];
    messageData->payloadStr.toCharArray(charbuf,8);
    long int rgb=strtol(charbuf + 1, 0, 16);
    byte r=(byte)(rgb>>16);
    byte g=(byte)(rgb>>8);
    byte b=(byte)(rgb);
    writeDMXcolor(r, g, b);
}


void processIncomingMessage(MessageData *messageData) {
    switch (messageData->control) {
    case status:
        processStatus(messageData->connectionType);
        break;
    case colorPicker:
        processColorPicker(messageData);
        break;
    case button:
        processButton(messageData);
        break;
    default:
        break;
    }
}

void sendColorHex(int red, int green, int blue) {
    // Convert RGB to hex value to send to Dash app.

    long rgb = ((long)red << 16) | ((long)green << 8 ) | (long)blue;
    String colorHex = String(rgb + 16777216, HEX); // Adding 16777216 puts a 1 at the start of the string and correct padding of zeros for red
    colorHex.setCharAt(0, '#'); // Replace the 1 at the start with the # char
    
    tcp_con.sendMessage(dashDevice.getColorMessage(COLOR_PICKER, colorHex));
}

void writeDMXcolor(int red, int green, int blue) {   
Serial.println("DMX: " + String(red));
    // Update DMX channels to new brightness
    
    // This is just an example of a DMX device that uses 7 channels. We are just using R, G, & B in this example.
    dmx.writeByte(255, 1);   // Master dimmer
    dmx.writeByte(red, 2);   // red
    dmx.writeByte(green, 3); // green
    dmx.writeByte(blue, 4);  // blue
    dmx.writeByte(0, 5);     // White
    dmx.writeByte(0, 6);     // Strobe
    dmx.writeByte(0, 7);     // Change colous

    dmx.update();
}

void updateAutoValues() {
    // Calculate new RGB values for smooth color change, based on the "angle" variable
    
    int red, green, blue;
    float rads;

    // calc RGB levels from angle
    rads = float(angle) * 3.1415926 / 180;

    red = int((sin(rads) + 1) * 255) / 2;
    green = int((sin(rads - (120*PI/180)) + 1) * 255) / 2;
    blue = int((sin(rads - (240*PI/180)) + 1) * 255) / 2;
    writeDMXcolor(red, green, blue);

    count++;
    if (count >= 10) {
        count = 0;
        sendColorHex(red, green, blue);
    }

    // Update angle for next time
    angle++;
    if (angle >= 360) {
        angle = 0;
    }
}

void timerTask(void *parameters) {
    // 100ms timer for DMX
    
    for(;;) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        tick = true;
    }
}

void setup() {
    Serial.begin(115200);

    dmxSerial.begin(DMX_BAUD, DMX_FORMAT); // Begin DMX serial port
    dmx.begin(dmxSerial, enPin, numChannels);
    dmx.setComDir(DMX_WRITE_DIR); // Set communicaiton direction

    dashDevice.setup(wifi.macAddress(), DEVICE_NAME); // Get unique deviceID
    tcp_con.setCallback(&processIncomingMessage);
    wifi.attachConnection(&tcp_con);
    wifi.begin(WIFI_SSID, WIFI_PASSWORD);

    // Setup 100 ms timer task for updating the DMX
    xTaskCreate(
        timerTask,
        "100ms",
        1024, // stack size
        NULL,
        1, // priority
        NULL // task handle
    );
}

void loop() {
    wifi.run();

    if (tick) {
        tick = false;

        if (autoButton == true) {
            updateAutoValues();
        } else {
            dmx.update(); // Send DMX values every 100ms
        }
    }
}

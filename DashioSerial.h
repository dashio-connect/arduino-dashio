/*
 MIT License

 Copyright (c) 2023 Craig Tuffnell, DashIO Connect Limited

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
*/

#ifndef DashioSerial_h
#define DashioSerial_h

#include "Arduino.h"
#include "Dashio.h"

const char CTRL[] = "CTRL";
const char MODE[] = "MODE";
const char NORMAL[] = "NML";
const char HALT[] = "HLT";
const char CGF[] = "CFG";
const char BLE[] = "BLE";
const char TCP[] = "TCP";
const char MQTT[] = "MQTT";
const char REBOOT[] = "REBOOT";
const char SLEEP[] = "SLEEP";
const char RAM[] = "RAM";

class DashSerial {
private:
    bool printMessages;
    DashDevice *dashDevice;
    MessageData data;
    void (*processSerialmessageCallback)(MessageData *MessageData);

public:
    DashSerial(DashDevice *_dashDevice, bool _printMessages = false);
    void setCallback(void(*processIncomingMessage)(MessageData *messageData));
    void (*transmitMessage)(const String& outgoingMessage);
    void setTransmit(void(*sendMessage)(const String& outgoingMessage));
    
    void processChar(char chr);
    void actOnMessage();

    void sendControlMessage(const String& controlID = "", const String& payload = "");
    void sendStoreConfig(const String &cfgStr, int cfgRev);

    String responseMessage;
};

#endif


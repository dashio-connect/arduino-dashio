/*
 DashJSON.cpp - Library for simle JSON encoding for the DashIO comms protocol.
 Created by C. Tuffnell
 November 17, 2020
 
 MIT License

 Copyright (c) 2020 Craig Tuffnell, DashIO Connect Limited

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

#include "DashJSON.h"

void DashJSON::start() {
    jsonStr = "{";
}

void DashJSON::addKeyString(const String& key, const String& text, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":\"";
    jsonStr += text;
    jsonStr += "\"";
    nextChar(last);
}

void DashJSON::addKeyStringAsNumber(const String& key, const String& text, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":";
    jsonStr += text;
    nextChar(last);
}

void DashJSON::addKeyStringArray(const String& key, String items[], int numItems, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":[";
    for (int i = 0; i < numItems; i++) {
        jsonStr += "\"";
        jsonStr += items[i];
        jsonStr += "\"";
        if (i < numItems - 1) {
            jsonStr += ",";
        }
    }
    jsonStr += "]";
    nextChar(last);
}

void DashJSON::addKeyFloat(const String& key, float number, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":";
    char numberBuffer[9];
#ifdef ARDUINO_ARCH_AVR
    dtostrf(number, 5, 3, numberBuffer);
#else
    sprintf(numberBuffer, "%5.3f", number);
#endif
    jsonStr += numberBuffer;
    nextChar(last);
}

void DashJSON::addKeyInt(const String& key, int number, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":";
    jsonStr += String(number);
    nextChar(last);
}

void DashJSON::addKeyBool(const String& key, bool boolean, bool last) {
    jsonStr += "\"";
    jsonStr += key;
    jsonStr += "\":";
    if (boolean) {
        jsonStr += "true";
    } else {
        jsonStr += "false";
    }
    nextChar(last);
}

void DashJSON::nextChar(bool last) {
    if (last) {
        jsonStr += "}";
    } else {
        jsonStr += ",";
    }
}

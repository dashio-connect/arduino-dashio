/*
 DashJSON.h - Library for simle JSON encoding for the DashIO comms protocol.
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

#ifndef DashJSON_h
#define DashJSON_h

#include "Arduino.h"

class DashJSON {
public:
    String jsonStr = "";

    void start();
    void addKeyString(const String& key, const String& text, bool last = false);
    void addKeyFloat(const String& key, float number, bool last = false);
    void addKeyInt(const String& key, int number, bool last = false);
    void addKeyBool(const String& key, bool boolean, bool last = false);
    void addKeyStringAsNumber(const String& key, const String& text, bool last = false);
    void addKeyStringArray(const String& key, String items[], int numItems, bool last = false);

private:
    void nextChar(bool last);
};

#endif

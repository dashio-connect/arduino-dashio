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

#ifndef DashSerial_h
#define DashSerial_h

#include "Arduino.h"
#include "DashIO.h"

class DashSerial {
  private:
    bool printMessages;
    DashioDevice *dashioDevice;
    MessageData data;
    void (*processSerialmessageCallback)(MessageData *MessageData);

    

  public:
    DashSerial(DashioDevice *_dashioDevice, bool _printMessages = false);
    void setCallback(void(*processIncomingMessage)(MessageData *messageData));
    void (*transmitMessage)(const String& outgoingMessage);
    void setTransmit(void(*sendMessage)(const String& outgoingMessage));
    
    void processChar(char chr);
    void actOnMessage();

    String responseMessage;
};

#endif


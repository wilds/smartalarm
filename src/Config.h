
/**The MIT License (MIT)

Copyright (c) 2022 by Wilds

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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <LittleFS.h>
#include <ArduinoJson.h>

class Config
{
 public:

    Config(String file) : _file(file) {
        if (LittleFS.begin())
        {
            Serial.println(F("LittleFS begin done."));
            read();
        }
        else
        {
            Serial.println(F("LittleFS begin fail."));
        }
    }

    ~Config() {
        LittleFS.end();
    }

    String _file;

    String mqttBrokerURL = "";   // IP or Address (DO NOT include http://)
    int mqttBrokerPort = 80;     // the port you are running (usually 80);
    String mqttUser = "";
    String mqttPassword = "";
    String apiKey = "";

    String deviceName = String(ESP.getChipId(), HEX);

    //String themeColor = "light-blue"; // this can be changed later in the web interface.


    void write() {
        // Save decoded message to LittleFS file for playback on power up.
        File f = LittleFS.open(_file, "w");
        if (!f) {
            Serial.println(F("File open failed!"));
        } else {
            Serial.println(F("Saving settings now..."));
            //StaticJsonDocument<200> doc;
            DynamicJsonDocument doc(2048);
            doc["mqttBrokerURL"] = this->mqttBrokerURL;
            doc["mqttBrokerPort"] = this->mqttBrokerPort;
            doc["mqttUser"] = this->mqttUser;
            doc["mqttPassword"] = this->mqttPassword;
            doc["apiKey"] = this->apiKey;
            doc["deviceName"] = this->deviceName;
            serializeJson(doc, f);
        }
        f.flush();
        f.close();
    }

    void read() {
        if (LittleFS.exists(_file) == false) {
            Serial.println(F("Settings File does not yet exists."));
            return;
        }
        File fr = LittleFS.open(_file, "r");

        //StaticJsonDocument<200> doc;
        DynamicJsonDocument doc(2048);
        DeserializationError error = deserializeJson(doc, fr);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
            return;
        }

        this->mqttBrokerURL = doc["mqttBrokerURL"].as<String>();
        this->mqttBrokerPort = doc["mqttBrokerPort"].as<int>();
        this->mqttUser = doc["mqttUser"].as<String>();
        this->mqttPassword = doc["mqttPassword"].as<String>();
        this->apiKey = doc["apiKey"].as<String>();
        this->deviceName = doc["deviceName"].as<String>();

        fr.close();
    }

    bool remove() {
        return LittleFS.remove(_file);
    }
};

#endif
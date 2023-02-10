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

#include <Arduino.h>
#include <ESP8266WiFi.h>

#include <ESPAsyncWebServer.h>
//#include <ESPAsyncWiFiManager.h>
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager

#include <WebSerial.h>

#include <Ticker.h>
#include <ArduinoOTA.h>
#include <time.h>

#include "settings.h"

//MQTT
#include <PubSubClient.h>

#include "Config.h"
#include "strings.h"

//include l74hc4051 library
#include <l74hc4051.h>

#include <ESP8266SSDP.h>

//#define TEST

enum State
{
  unknown,
  disarmed,
  armed_home,
  armed_away,
  armed_night,
  armed_vacation,
  armed_custom_bypass,
  pending,
  triggered,
  arming,
  disarming
};

String state[] = {"unknown", "disarmed", "armed_home", "armed_away", "armed_night", "armed_vacation", "armed_custom_bypass", "pending", "triggered", "arming", "disarming"};

const char keys[4][4] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'H'},
  {'7','8','9', 'C'},
  {'*','0','#', 'S'}
};
typedef struct KeypadButton {
  KeypadButton(byte _r, byte _c) : r(_r), c(_c) {}
  byte r;
  byte c;
  bool isValid() {
    return r >= 0 && c >= 0;
  }
} KeypadButton;

//L74HC4051 library
L74HC4051 rows;
L74HC4051 columns;
const int ROWS_PIN_A = D1;
const int ROWS_PIN_B = D2;
const int ROWS_PIN_C = -1;
const int COLS_PIN_A = D3;
const int COLS_PIN_B = D4;
const int COLS_PIN_C = -1;
const int PRESS_PIN = D0; // connected to enable

const int CONFIG_MODE_PIN = D8; //D5
const int TRIGGERED_PIN = D7;   //D6
const int ARMED_HOME_PIN = D5;  //D7
const int ARMED_AWAY_PIN = D6;  //D8

State currentState = State::unknown;

WiFiClient client;

WiFiClient mqttClient;
PubSubClient mqtt(mqttClient);
Ticker mqttReconnectTimer;

void initMqtt();
void updateMqtt();
void callback(char* topic, byte* payload, unsigned int length);
void connectToMqtt();
void sendMQTTDeviceDiscoveryMsg();
void pressKey(const char key);
void writeCommand(const char *command);
void writeCommand(String command);
KeypadButton getKey(const char key);

void handleGetState(AsyncWebServerRequest *request);
void handleCommand(AsyncWebServerRequest *request);
void handleControlRoute(AsyncWebServerRequest *request);
void saveParamCallback();
void bindServerCallback();

void updateCurrentState();
void sendStateUpdate();


/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len);

//declaring prototypes
//void configModeCallback(WiFiManager *myWiFiManager);


Config config(String(CONFIG));

String discoveryTopic = "homeassistant/alarm_control_panel/" + config.deviceName + "/config";
//String rootTopic = "home/alarm/" + config.deviceName;
String stateTopic = "home/alarm/" + config.deviceName + "/state";
String commandTopic = "home/alarm/" + config.deviceName + "/set";
String availabilityTopic = "home/alarm/" + config.deviceName + "/lwt";


//WiFiManager
WiFiManager wifiManager;
WiFiManagerParameter mqttBrokerURL("mqttBrokerURL", "Broker URL", config.mqttBrokerURL.c_str(), 255);
WiFiManagerParameter mqttBrokerPort("mqttBrokerPort", "Broker Port", String(config.mqttBrokerPort).c_str(), 5, "input='number'");
WiFiManagerParameter mqttUser("mqttUser", "User", config.mqttUser.c_str(), 255);
WiFiManagerParameter mqttPassword("mqttPassword", "Password", config.mqttPassword.c_str(), 255, "input='password'");
WiFiManagerParameter mqttApiKey("mqttApiKey", "Token", config.apiKey.c_str(), 255);
WiFiManagerParameter deviceName("deviceName", "Device Name", config.deviceName.c_str(), 255);

String hostname = HOSTNAME + String(ESP.getChipId(), HEX);

void setup() {

  Serial.begin(115200);

  //Init config
  //config.read();
  //mqttBrokerURL.setValue();

  //discoveryTopic = "homeassistant/alarm_control_panel/" + config.deviceName + "/config";
  //stateTopic = "home/alarm/" + config.deviceName + "/state";
  //commandTopic = "home/alarm/" + config.deviceName + "/set";

  WiFi.mode(WIFI_STA); // explicitly set mode, esp defaults to STA+AP

  //wifiManager.resetSettings();  // Uncomment for testing wifi manager

  // callbacks
  //wifiManager.setAPCallback(configModeCallback);
 	//wifiManager.setAPCallback([&](WiFiManager* wifiManager) {
	//});
  wifiManager.setWebServerCallback(bindServerCallback);
  //wifiManager.setSaveConfigCallback(saveWifiCallback);
  wifiManager.setSaveParamsCallback(saveParamCallback);

  // parameters
  wifiManager.addParameter(&mqttBrokerURL);
  wifiManager.addParameter(&mqttBrokerPort);
  wifiManager.addParameter(&mqttUser);
  wifiManager.addParameter(&mqttPassword);
  wifiManager.addParameter(&mqttApiKey);
  wifiManager.addParameter(&deviceName);

  const char* menuhtml = "<form action='/control' method='get'><button>Alarm Control</button></form><br/>\n";
  wifiManager.setCustomMenuHTML(menuhtml);

  std::vector<const char *> menu = {"wifi", "info", "param", "custom", "sep", "restart"};
  wifiManager.setMenu(menu);

  wifiManager.setClass("invert");

  wifiManager.setConfigPortalBlocking(true);
  //wifiManager.setConfigPortalTimeout(120);

  if(wifiManager.autoConnect()) {
    Serial.println("connected...yeey :)");
    wifiManager.startWebPortal();
  } else {
    Serial.println("non blocking config portal running");
    //wifiManager.startConfigPortal();
  }


  WebSerial.begin(wifiManager.server.get(), "/serial");
  WebSerial.msgCallback(recvMsg);

  // Setup OTA
  Serial.println("Hostname: " + hostname);
  ArduinoOTA.setHostname((const char *)hostname.c_str());
  //ArduinoOTA.onProgress(drawOtaProgress);
  ArduinoOTA.begin();

  // Initiazile MQTT
  initMqtt();
  //Connect to MQTT
  if (config.mqttBrokerURL && !config.mqttBrokerURL.isEmpty()) {
    connectToMqtt();
  }

  // init keypad
  rows.init(ROWS_PIN_A, ROWS_PIN_B, ROWS_PIN_C);
  columns.init(COLS_PIN_A, COLS_PIN_B, COLS_PIN_C);
  pinMode(PRESS_PIN, OUTPUT);
  digitalWrite(PRESS_PIN, HIGH);


  pinMode(TRIGGERED_PIN, INPUT);
  pinMode(ARMED_HOME_PIN, INPUT);
  pinMode(ARMED_AWAY_PIN, INPUT);
  pinMode(CONFIG_MODE_PIN, INPUT);
}


int checkBlinking = LOW;
long lastBlink = 0;

void updateCurrentState() {
  #ifndef TEST
  int armHome = digitalRead(ARMED_HOME_PIN);
  int armAway = digitalRead(ARMED_AWAY_PIN);
  int triggered = digitalRead(TRIGGERED_PIN);
  int configMode = digitalRead(CONFIG_MODE_PIN);  //TODO

  State newState = State::disarmed;

  //check blinking led -> arming
  if (checkBlinking != armAway) {
    checkBlinking = armAway;
    lastBlink = millis();
  }

  if (configMode == HIGH) {
    newState = State::pending;
  } else if (triggered == HIGH) {
    newState = State::triggered;
  } else if ((millis() - lastBlink) <= 2000) {  //is blinking
    newState = State::arming;
  } else if (armHome == HIGH) {
    newState = State::armed_home;
  } else if (armAway == HIGH && (millis() - lastBlink) > 2000) {
    newState = State::armed_away;
  }

  if (newState != currentState) {
    currentState = newState;

     Serial.printf("armHome: %i\n", armHome);
     Serial.printf("armAway: %i\n", armAway);
     Serial.printf("triggered: %i\n", triggered);
     Serial.printf("configMode: %i\n", configMode);

    sendStateUpdate();
  }
  #endif
}

void sendStateUpdate() {

  Serial.println(F("State update: ") + state[currentState]);

  DynamicJsonDocument doc(1024);
  doc["state"] = state[currentState];

  char buffer[512];
  /*size_t n =*/ serializeJson(doc, buffer);
  if (!mqtt.publish(stateTopic.c_str(), buffer, true)) {
    Serial.println(F("publish failed"));
  }
  yield();
}

void loop() {

  if (config.mqttBrokerURL && !config.mqttBrokerURL.isEmpty()) {
    updateMqtt();
  }

  wifiManager.process();

  ArduinoOTA.handle();

  updateCurrentState();

}


void saveParamCallback(){
  Serial.println("[CALLBACK] saveParamCallback fired");

  config.mqttBrokerURL = mqttBrokerURL.getValue();
  config.mqttBrokerPort = atoi(mqttBrokerPort.getValue());
  config.mqttUser = mqttUser.getValue();
  config.mqttPassword = mqttPassword.getValue();
  config.apiKey = mqttApiKey.getValue();
  config.deviceName = deviceName.getValue();

  config.write();
  //ESP.restart();

}


void handleControlRoute(AsyncWebServerRequest *request){
  Serial.println("request " + request->url());
  String page = FPSTR(HTTP_HEAD_START);
  page.replace("{v}", "Control");
  page += FPSTR(HTTP_SCRIPT);
  page += FPSTR(HTTP_STYLE);
  page += FPSTR(ALARM_PAGE_STYLE);
  page += FPSTR(ALARM_PAGE_SCRIPT);
  page += FPSTR(HTTP_HEAD_END);


  //pitem = FPSTR(HTTP_FORM_START);
  page += FPSTR(ALARM_PAGE_BODY);
  page.replace("{c}", state[currentState]);
  //page += FPSTR(HTTP_FORM_END);
  page += FPSTR(HTTP_BACKBTN);
  ///reportStatus(page);
  page += FPSTR(HTTP_END);

  request->send(200, FPSTR(HTTP_HEAD_CT), page);

}

void handleCommand(AsyncWebServerRequest *request)
{
  Serial.println("request " + request->url());
  AsyncWebServerResponse *response;
  if (request->hasArg(F("plain")) && !request->arg(F("plain")).isEmpty())
  {
    String command = request->arg(F("plain"));
    writeCommand((command + '\0'));
    response = request->beginResponse(200, FPSTR(HTTP_HEAD_CT2), F("OK"));
  } else {
    response = request->beginResponse(400, FPSTR(HTTP_HEAD_CT2), F("KO"));
  }
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL)); // @HTTPHEAD send cors
  request->send(response);
}

void handleGetState(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(200, FPSTR(HTTP_HEAD_CT2), state[currentState]);
  response->addHeader(FPSTR(HTTP_HEAD_CORS), FPSTR(HTTP_HEAD_CORS_ALLOW_ALL)); // @HTTPHEAD send cors
  request->send(response);
}

void bindServerCallback(){
  wifiManager.server->on("/control", HTTP_GET, handleControlRoute);
  wifiManager.server->on("/command", HTTP_POST, handleCommand);
  //server.on("/serial", HTTP_GET, handleSerialRoute);
  // wm.server->on("/info", handleRoute); // you can override wm!

   wifiManager.server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello World");
  });
}


void initMqtt() {
  if (config.mqttBrokerURL && !config.mqttBrokerURL.isEmpty()) {
    mqtt.setBufferSize(1024);
    mqtt
      .setKeepAlive(10)
      .setServer(config.mqttBrokerURL.c_str(), config.mqttBrokerPort)
      .setCallback(callback);
  }
}


void connectToMqtt() {
  // Loop until we're reconnected
  if (!mqtt.connected()) {
    // Create client ID
    //String clientId = "esp-" + String(ESP.getChipId(), HEX);
    //Serial.println("clientId: " + clientId);

    Serial.print(("Attempting MQTT connection to " + config.mqttBrokerURL + ":" + config.mqttBrokerPort + "... ").c_str());

    // Attempt to connect

    bool connected = config.mqttUser.isEmpty() ? mqtt.connect(hostname.c_str()) : mqtt.connect(hostname.c_str(), config.mqttUser.c_str(), config.mqttPassword.c_str(), availabilityTopic.c_str(), 1, true, "offline");
    if (connected) {
      Serial.println("connected");
      // Once connected, publish an announcement...

      mqtt.subscribe(commandTopic.c_str());
      mqtt.publish(availabilityTopic.c_str(), "online", true);

      sendMQTTDeviceDiscoveryMsg();

      mqttReconnectTimer.detach();
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 30 seconds");
      // Wait 5 seconds before retrying
      mqttReconnectTimer.once(30, connectToMqtt);
    }
  }
}

unsigned long lastClientLoop = millis();

void updateMqtt() {

  //if (!mqtt.connected()) {
  //  previusConnected = false;
  //  if (!mqttReconnectTimer.active())
  //    connectToMqtt();
  //} else {
    if (millis() - lastClientLoop >= 500) {
      // mqtt.loop()
      if (!mqtt.loop()) {
        if (!mqttReconnectTimer.active())
          connectToMqtt();
      }
      lastClientLoop = millis();
    }
  //}
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.printf("Message arrived [%s]\n", topic);
  //Serial.printf("%s\n", payload);

  if (strcmp(topic, commandTopic.c_str()) == 0) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String action = doc["action"].as<String>();
    String code = doc["code"].as<String>();
    Serial.println("action: " + action);
    Serial.println("code: " + code);

    if (action.equals("ARM_AWAY")) {
      pressKey('A');
      #ifdef TEST
        currentState = State::armed_away;
        sendStateUpdate();
      #endif
    } else if (action.equals("ARM_HOME")) {
      pressKey('H');
      #ifdef TEST
        currentState = State::armed_home;
        sendStateUpdate();
      #endif
    } else if (action.equals("ARM_NIGHT")) {

    } else if (action.equals("DISARM")) {
      writeCommand(String(code + "#").c_str());
      #ifdef TEST
        currentState = State::disarmed;
        sendStateUpdate();
      #endif
    } else if (action.equals("TRIGGER")) {
      pressKey('S');
      #ifdef TEST
        currentState = State::triggered;
        sendStateUpdate();
      #endif
    }
  }

}

void sendMQTTDeviceDiscoveryMsg() {

  DynamicJsonDocument doc(1024);

  doc[F("name")] = config.deviceName;
  doc[F("uniq_id")]   = String(ESP.getChipId(), HEX);
  doc[F("ic")] = F("mdi:shield-home");  //icon
  doc[F("stat_t")]   = stateTopic; //state_topic
  doc[F("val_tpl")] = F("{{ value_json.state }}"); //value_template
  doc[F("cmd_t")]   = commandTopic; //command_topic
  doc[F("cmd_tpl")] = F("{ action: '{{ action }}', code: '{{ code }}'}"); //command_template
  doc[F("code")] = F("REMOTE_CODE");
  //doc["ret"] = true;  //retain

  doc[F("cod_arm_req")] = false;  //code_arm_required
  doc[F("cod_dis_req")] = true;  //code_disarm_required
  doc[F("cod_trig_req")] = false;  //code_trigger_required

  /*
  pl_arm_away payload_arm_away  ARM_AWAY
  pl_arm_home payload_arm_home  ARM_HOME
  //pl_arm_away payload_arm_night   ARM_NIGHT
  pl_disarm payload_disarm  DISARM
  pl_trig payload_trigger TRIGGER
  */

  //avty_mode          availability_mode
  doc[F("avty_t")] = availabilityTopic;  //availability_topic
  //avty_tpl availability_template
  //pl_avail payload_available  online
  //pl_not_avail payload_not_available offline

  doc[F("qos")] = 1;

  JsonObject device  = doc.createNestedObject("device");
  //device["cu"] = WiFi.localIP().toString(); //configuration_url
  //device["cns"] = ""; //connections: [["imei", "___"]]
  device[F("ids")] = String(ESP.getChipId(), HEX); //identifiers: serial number, imei
  device[F("mf")] = F("Wolf-guard"); //manufacturer  //HOMSECUR
  device[F("mdl")] = F("M2BX-1"); //model    //YA09
  device[F("name")] = F("Alarm"); //name
  device[F("sw")] = F("1.0"); //sw_version

  char buffer[1024];
  /*size_t n =*/ serializeJson(doc, buffer);
  //Serial.println(discoveryTopic);
  //Serial.println(buffer);

  boolean r = mqtt.publish(discoveryTopic.c_str(), buffer, true);
  if (r) {
    Serial.println(F("Discovery message sent successfully"));
  } else {
    Serial.println(F("ERROR sending discovery message"));
  }
  yield();
}

KeypadButton getKey(const char key) {
  //TODO refactoring
  int c = -1;
  int r = -1;
  for (c = 0; c < 4; ++c) {
    for (r = 0; r < 4; ++r) {
      if (keys[r][c] == key) {
        return KeypadButton(r, c);
      }
    }
  }
  return KeypadButton(r, c);
}

void pressKey(const char key) {
  Serial.printf("Looking for key %c (%i)\n", key, key);
  KeypadButton button = getKey(key);
  if (button.isValid()) {
    Serial.printf("Send key %c (%i) (%d, %d)\n", key, key, button.r, button.c);
    rows.setChannel(button.r);
    columns.setChannel(button.c);
    digitalWrite(PRESS_PIN, LOW);
    delay(200);
    digitalWrite(PRESS_PIN, HIGH);
    delay(100);
    //rows.setChannel(0);
    //columns.setChannel(0);
  } else {
    Serial.println(F("Invalid char"));
  }
}

void writeCommand(const char *command) {
  for (size_t i = 0; i < strlen(command); ++i)
  {
    const char c = (const char) command[i];
    pressKey(c);
  }
}

void writeCommand(String command) {
  writeCommand(command.c_str());
}

/* Message callback of WebSerial */
void recvMsg(uint8_t *data, size_t len) {
  WebSerial.println("Received Data...");
  String d = "";
  for (size_t i = 0; i < len; ++i)
  {
    d += char(data[i]);
  }
  WebSerial.println(d);
}

#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266FtpServer.h>
#include <ArduinoJson.h>
#define ONE_WIRE_BUS 0
char defaultSettings[] = "{\"pumpTime\":5,\"timeoutTime\":15,\"temperature\":50,\"turnOffPump\":true,\"diffTemp\":20}";
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const char* ssid = "Evseenko";
const char* password = "787898852456";
ESP8266WebServer server(80);
FtpServer ftpSrv;
#include "GyverTimer.h"
GTimer PumpTimer(MS);
GTimer TimeOutTimer(MS);
GTimer SecondTimer(MS, 1000);
#define DS18B20_PIN 0
uint8_t t;
DynamicJsonDocument settings(200);

void setup() {
  pinMode(2, OUTPUT);
  Serial.begin(115200);
  delay(10);
  digitalWrite(2, HIGH);
  sensors.begin();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  SPIFFS.begin();
  //SPIFFS.format();
  loadSettings();
  server.begin();
  ftpSrv.begin("Pump Switch", "787898");
  Serial.println(WiFi.localIP());
  server.on("/reset_pump", [] () {
    server.send(200, "text/plain", reset_pump());
  });
  server.on("/reset_timeout", [] () {
    server.send(200, "text/plain", reset_timeout());
  });
  server.on("/get_state", [] () {
    server.send(200, "text/plain", get_state());
  });
  server.on("/get_settings", [] () {
    server.send(200, "text/plain", get_settings());
  });
  server.on("/put_settings", put_settings);
  server.onNotFound([] () {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "Not found");
  });
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(500);
      Serial.print(".");
    }
    server.begin();
  }
  ftpSrv.handleFTP();
  server.handleClient();
  if (!TimeOutTimer.isEnabled() || TimeOutTimer.isReady()) {
    if (PumpTimer.isReady()) {
      off_pump();
    } else if (!PumpTimer.isEnabled()) {
      if (t >= settings["temperature"].as<int>()) {
        on_pump();
      }
    }
  }
  if (SecondTimer.isReady()) {
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);
    if (settings["turnOffPump"].as<bool>()) {
      if (PumpTimer.isEnabled() && t <= settings["temperature"].as<int>() - settings["diffTemp"].as<int>()) {
        PumpTimer.stop();
        off_pump();
      }
    }
  }
}

void off_pump() {
  TimeOutTimer.setTimeout(settings["timeoutTime"].as<int>() * 60 * 1000);
  digitalWrite(2, HIGH);
}

void on_pump() {
  PumpTimer.setTimeout(settings["pumpTime"].as<int>() * 60 * 1000);
  digitalWrite(2, LOW);
}

void put_settings() {
  if (server.arg("pumpTime")) settings["pumpTime"] = server.arg("pumpTime").toInt();
  if (server.arg("timeoutTime")) settings["timeoutTime"] = server.arg("timeoutTime").toInt();
  if (server.arg("temperature")) settings["temperature"] = server.arg("temperature").toInt();
  if (server.arg("turnOffPump")) settings["turnOffPump"] = (server.arg("turnOffPump") == "false") ? false : true;
  if (server.arg("diffTemp")) settings["diffTemp"] = server.arg("diffTemp").toInt();
  File file = SPIFFS.open("/settings.json", "w");
  if (!file) {
    server.send(500, "text/plain", "fail"); return;
  }
  serializeJson(settings, file);
  file.close();
  server.send(200, "text/plain", "ok");
}

String get_settings() {
  return settings.as<String>();
}

String get_state() {
  DynamicJsonDocument response(100);
  response["pump_state"] = PumpTimer.isEnabled();
  response["timeout_state"] = TimeOutTimer.isEnabled();
  response["temperature"] = t;
  response["pumpTime"] = settings["pumpTime"];
  response["timeoutTime"] = settings["timeoutTime"];
  return response.as<String>();
}

String reset_pump() {
  DynamicJsonDocument response(20);
  PumpTimer.stop();
  response["response"] = true;
  return response.as<String>();
}

String reset_timeout() {
  DynamicJsonDocument response(20);
  TimeOutTimer.stop();
  response["response"] = true;
  return response.as<String>();
}

bool handleFileRead(String path) {
  Serial.println(path.endsWith("/"));
  if (path.endsWith("/")) path += "index.html";
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, "text/html");
    file.close();
    return true;
  }
  return false;
}

void loadSettings() {
  File file = SPIFFS.open("/settings.json", "r");
  if (file) {
    String in = file.readString();
    deserializeJson(settings, in);
  } else {
    deserializeJson(settings, defaultSettings);
  }
  file.close();
}

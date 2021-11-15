#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FS.h>
#include <ESP8266FtpServer.h>
#include <ArduinoJson.h>

#define RSSI_MAX -65
#define RSSI_MIN -100

#define ONE_WIRE_BUS 0
char defaultSettings[] = "{\"pumpTime\":5,\"timeoutTime\":15,\"temperature\":50,\"turnOffPump\":true,\"diffTemp\":20}";
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
const char* ssid = "SSID";
const char* password = "PASSWORD";
ESP8266WebServer server(80);
IPAddress ip(192, 168, 1, 10);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
FtpServer ftpSrv;
#include <TimerMs.h>
TimerMs PumpTimer;
TimerMs TimeOutTimer;
TimerMs SecondTimer(1000, 1, 0);
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
  WiFi.config(ip, gateway, subnet);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  SPIFFS.begin();
  //SPIFFS.format();
  loadSettings();
  WiFi.setAutoReconnect(true);
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
    server.send(200, "application/json", get_state());
  });
  server.on("/get_settings", [] () {
    server.send(200, "application/json", get_settings());
  });
  server.on("/put_settings", put_settings);
  server.onNotFound([] () {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "Not found");
  });
  PumpTimer.setTimerMode();
  TimeOutTimer.setTimerMode();
  SecondTimer.setPeriodMode();
}

void loop() {
  PumpTimer.tick();
  TimeOutTimer.tick();
  ftpSrv.handleFTP();
  server.handleClient();
  if (!TimeOutTimer.active() || TimeOutTimer.ready()) {
    if (PumpTimer.ready()) {
      off_pump();
    } else if (!PumpTimer.active()) {
      if (t >= settings["temperature"].as<int>()) {
        on_pump();
      }
    }
  }
  if (SecondTimer.tick()) {
    sensors.requestTemperatures();
    t = sensors.getTempCByIndex(0);
    if (settings["turnOffPump"].as<bool>()) {
      if (PumpTimer.active() && t <= settings["temperature"].as<int>() - settings["diffTemp"].as<int>()) {
        PumpTimer.force();
        off_pump();
      }
    }
  }
}

void off_pump() {
  TimeOutTimer.setTime(settings["timeoutTime"].as<int>() * 60 * 1000);
  TimeOutTimer.start();
  digitalWrite(2, HIGH);
}

void on_pump() {
  PumpTimer.setTime(settings["pumpTime"].as<int>() * 60 * 1000);
  PumpTimer.start();
  digitalWrite(2, LOW);
}

void put_settings() {
  if (server.arg("pumpTime")) {
    if (server.arg("pumpTime").toInt() != settings["pumpTime"].as<int>()) {
      PumpTimer.stop();
      settings["pumpTime"] = server.arg("pumpTime").toInt();
    }
  }
  if (server.arg("timeoutTime")) {
    if (server.arg("timeoutTime").toInt() != settings["timeoutTime"].as<int>()) {
      TimeOutTimer.stop();
      settings["timeoutTime"] = server.arg("timeoutTime").toInt();
    }
  }
  if (server.arg("temperature")) {
    if (server.arg("temperature").toInt() != settings["temperature"].as<int>()) {
      PumpTimer.stop();
      digitalWrite(2, HIGH);
      settings["temperature"] = server.arg("temperature").toInt();
    }
  }
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
  DynamicJsonDocument response(150);
  response["pump_state"] = PumpTimer.active();
  response["timeout_state"] = TimeOutTimer.active();
  response["temperature"] = t;
  response["rssi"] = dBmtoPercentage(WiFi.RSSI());
  response["pumpLeft"] = PumpTimer.timeLeft();
  response["timeoutLeft"] = TimeOutTimer.timeLeft();
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
  if (path.endsWith("/")) path += "index.html";
  if (SPIFFS.exists(path)) {
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, getContentType(path));
    file.close();
    return true;
  }
  return false;
}

String getContentType(String ext) {
  if (ext.endsWith(".html")) return "text/html";
  else if (ext.endsWith(".js")) return "application/javascript";
  else if (ext.endsWith(".png")) return "image/png";
  return "text/plain";
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

int dBmtoPercentage(int dBm)
{
  int quality;
  if (dBm <= RSSI_MIN) {
    quality = 0;
  }
  else if (dBm >= RSSI_MAX) {
    quality = 100;
  } else {
    quality = 2 * (dBm + 100);
  }
  return quality;
}

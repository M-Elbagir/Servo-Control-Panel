#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ESP32Servo.h>

const char* WIFI_SSID = "********";
const char* WIFI_PASSWORD = "********";
const char* SUPABASE_URL = "https://***************.supabase.co";
const char* SUPABASE_ANON_KEY = "************************REPLACE_WITH_ANON_KEY************************";
const char* TABLE_NAME = "servo_state";
const char* ROW_ID = "servo";
const int SERVO_PIN = 18;

WiFiClientSecure secureClient;
Servo servo;
int lastAngle = -1;
unsigned long lastFetchMillis = 0;
unsigned long fetchIntervalMs = 500;
unsigned long lastWiFiRetryMillis = 0;
uint8_t consecutiveFailures = 0;

const unsigned long BASE_FETCH_INTERVAL_MS = 500;
const unsigned long MAX_FETCH_INTERVAL_MS = 3000;
const unsigned long WIFI_RETRY_INTERVAL_MS = 2000;

int extractAngleFromPayload(const String& payload) {
  int angleKey = payload.indexOf("\"angle\"");
  if (angleKey < 0) {
    return -1;
  }

  int colon = payload.indexOf(':', angleKey);
  if (colon < 0) {
    return -1;
  }

  int comma = payload.indexOf(',', colon);
  int brace = payload.indexOf('}', colon);
  int endPos = comma;
  if (endPos < 0 || (brace >= 0 && brace < endPos)) {
    endPos = brace;
  }

  if (endPos < 0) {
    endPos = payload.length();
  }

  String angleText = payload.substring(colon + 1, endPos);
  angleText.trim();
  return angleText.toInt();
}

bool fetchAngleFromSupabase(int& angle) {
  HTTPClient http;
  String requestUrl = String(SUPABASE_URL) + "/rest/v1/" + TABLE_NAME +
                      "?select=angle&id=eq." + ROW_ID +
                      "&order=updated_at.desc&limit=1";

  if (!http.begin(secureClient, requestUrl)) {
    Serial.println("[HTTP] Unable to connect");
    return false;
  }

  http.setConnectTimeout(3000);
  http.setTimeout(4000);
  http.setReuse(true);

  http.addHeader("apikey", SUPABASE_ANON_KEY);
  http.addHeader("Authorization", String("Bearer ") + SUPABASE_ANON_KEY);
  http.addHeader("Accept", "application/json");

  int httpCode = http.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.printf("[HTTP] GET failed, code: %d, error: %s\n", httpCode, http.errorToString(httpCode).c_str());
    http.end();
    return false;
  }

  String payload = http.getString();
  http.end();

  angle = extractAngleFromPayload(payload);
  if (angle < 0) {
    Serial.println("[HTTP] No usable angle found in Supabase payload");
    Serial.println(payload);
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  servo.setPeriodHertz(50);
  servo.attach(SERVO_PIN, 500, 2400);
  servo.write(90);

  secureClient.setInsecure();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print('.');
  }

  Serial.println();
  Serial.println("WiFi connected");
  Serial.println(WiFi.localIP());
}

void loop() {
  unsigned long now = millis();
  if (WiFi.status() != WL_CONNECTED) {
    if (now - lastWiFiRetryMillis >= WIFI_RETRY_INTERVAL_MS) {
      lastWiFiRetryMillis = now;
      Serial.println("WiFi disconnected, retrying...");
      WiFi.reconnect();
    }
    delay(50);
    return;
  }

  if (now - lastFetchMillis >= fetchIntervalMs) {
    lastFetchMillis = now;

    int angle = -1;
    if (fetchAngleFromSupabase(angle)) {
      consecutiveFailures = 0;
      fetchIntervalMs = BASE_FETCH_INTERVAL_MS;

      angle = constrain(angle, 0, 180);
      if (angle != lastAngle) {
        lastAngle = angle;
        servo.write(angle);
        Serial.printf("Servo moved to %d degrees\n", angle);
      }
    } else {
      consecutiveFailures++;
      fetchIntervalMs = min(MAX_FETCH_INTERVAL_MS, BASE_FETCH_INTERVAL_MS + (unsigned long)consecutiveFailures * 250);
    }
  }

  delay(10);
}

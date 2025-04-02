#include <Arduino.h>
#include "secrets.h"
#include <ESP8266WiFi.h>
#include <ESP_Mail_Client.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "DateTime.h"
#include <time.h>
#include <espnow.h>
#include <message.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// During Daylight Saving Time (EDT), it becomes UTC-4 hours
#define EST_OFFSET_SECONDS (-5 * 3600)
#define EDT_OFFSET_SECONDS (-4 * 3600)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
bool receivedMessage = false;
void initializeTimeZone() {
  // Explicitly set EST timezone
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
}

struct_message esp_message;
bool connectToWifi();
void sendEmail(); 
void smtpCallback(SMTP_Status status);
void setTime();
void disableESPNow();
void enableESPNow();
void updateMedicationStatus();
String getCurrentDateTimeISO8601();

std::unique_ptr<WiFiClientSecure> client(new WiFiClientSecure());
String deviceId;

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&esp_message, incomingData, sizeof(esp_message));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message: ");
  Serial.println(esp_message.message);
  Serial.print("IsOpen: ");
  Serial.println(esp_message.isOpen);
  Serial.println();

  receivedMessage = true;
}
SMTPSession smtp;
volatile bool criticalTasksComplete = false;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");


void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  enableESPNow();
}

void loop() {
  if (receivedMessage) {
    receivedMessage = false;
    // Disable ESP-NOW before WiFi connection
    disableESPNow();
    if (connectToWifi()) {
      setTime();
      sendEmail();
      updateMedicationStatus();
      WiFi.disconnect(true);
    }
    enableESPNow();
  }
  delay(1000);
}

void updateMedicationStatus() {
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Connected to Wi-Fi");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
} else {
    Serial.println("Failed to connect to Wi-Fi");
}
  HTTPClient http;

  // Create JSON payload
  DynamicJsonDocument doc(256);
  doc["deviceId"] = "ESP8266_Device_1";
  doc["timestamp"] = getCurrentDateTimeISO8601();
  
  String jsonPayload;
  serializeJson(doc, jsonPayload);
  
  Serial.println("Preparing to update medication status...");
  Serial.println("JSON Payload:");
  Serial.println(jsonPayload);
  
  // Set the trust anchors for secure connection
  Serial.println("Setting trust anchors for secure connection...");
  client->setTrustAnchors(new X509List(api_cert));
  // Begin HTTP connection to API Gateway
  Serial.print("Connecting to API Gateway: ");
  Serial.println(AWS_API_ENDPOINT);
  if (!http.begin(*client, AWS_API_ENDPOINT)) {
    Serial.println("Failed to begin HTTP connection");
    return;
  }

  Serial.println("Successfully began HTTP connection");

  // Adding headers to the request
  http.addHeader("Content-Type", "application/json");
  http.addHeader("x-api-key", api_key);

  Serial.println("Sending HTTP POST request...");
  
  // Send the request
  int httpCode = http.POST(jsonPayload);
  
  if (httpCode > 0) {
    String response = http.getString();
    Serial.printf("HTTP response code: %d\n", httpCode);
    Serial.println(response);
  } else {
      Serial.printf("HTTP request failed, error: %s\n", http.errorToString(httpCode).c_str());
      String response = http.getString();
      if (response.length() > 0) {
          Serial.println("Response body:");
          Serial.println(response);  // Print the error message from the server
      } else {
          Serial.println("No response received from the server.");
      }
  }
  
  Serial.println("Ending HTTP session...");
  http.end();
}


void disableESPNow() {
  // Unregister the receive callback
  esp_now_unregister_recv_cb();
  
  // Deinit ESP-NOW
  esp_now_deinit();
  
  Serial.println("ESP-NOW disabled");
}

void enableESPNow() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  esp_now_add_peer(clientAddress, ESP_NOW_ROLE_COMBO, 1, NULL, 0);

  Serial.println("ESP-NOW enabled");
}

bool connectToWifi() {
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wifi");
  
  unsigned long startAttemptTime = millis();
  const unsigned long WIFI_TIMEOUT_MS = 30000;  // 30 second timeout
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    
    if (millis() - startAttemptTime > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi connection failed!");
      return false;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected with IP: ");
    Serial.println(WiFi.localIP());
    
    return true;
  }
  return false;
}

void sendEmail() {
  // 1 for basic debugging, 0 for none
  smtp.debug(1);

  smtp.callback(smtpCallback);

  Session_Config config;

    /* Set the session config */
    config.server.host_name = SMTP_HOST;
    config.server.port = SMTP_PORT;
    config.login.email = AUTHOR_EMAIL;
    config.login.password = AUTHOR_PASSWORD;
    config.login.user_domain = "gmail.com";
    config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
    config.time.gmt_offset = 3;
    config.time.day_light_offset = 0;
  
    SMTP_Message message;
    DateTime currentTime = getCurrentDateTime();
    
    message.sender.name = F("Smart Medication");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = "Medication Notification : " + currentTime.date;
    message.addRecipient(F("Masa"), RECIPIENT_EMAIL);  
     
    String textMsg = "You took your medication at " + currentTime.time + " on " + currentTime.date;
    message.text.content = textMsg.c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    
    message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
    message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;
  

    /* Connect to the server */
    if (!smtp.connect(&config)){
      ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
      return;
    }
  
    if (!smtp.isLoggedIn()){
      Serial.println("\nNot yet logged in.");
    }
    else{
      if (smtp.isAuthenticated())
        Serial.println("\nSuccessfully logged in.");
      else
        Serial.println("\nConnected with no Auth.");
    }
  
    /* Start sending Email and close the session */
    if (!MailClient.sendMail(&smtp, &message))
      ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  }

  
  
  DateTime getCurrentDateTime() {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);
    
    DateTime dt;
    
    // Date extraction
    char dateBuff[11];
    strftime(dateBuff, sizeof(dateBuff), "%Y-%m-%d", &timeinfo);
    dt.date = String(dateBuff);
    
    // 12-hour time extraction with AM/PM, without seconds
    char timeBuff[20];
    strftime(timeBuff, sizeof(timeBuff), "%I:%M %p", &timeinfo);
    dt.time = String(timeBuff);
    
    // Day of week
    char dayBuff[10];
    strftime(dayBuff, sizeof(dayBuff), "%A", &timeinfo);
    dt.dayOfWeek = String(dayBuff);
    
    // Timezone determination
    dt.isDST = timeinfo.tm_isdst > 0;
    dt.timeZone = dt.isDST ? "EDT" : "EST";
    
    return dt;
  }
  
  String getCurrentDateTimeISO8601() {
    time_t now;
    struct tm timeinfo;
    
    time(&now);
    localtime_r(&now, &timeinfo);

    char iso8601Buff[20]; // Buffer for ISO format
    strftime(iso8601Buff, sizeof(iso8601Buff), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    
    return String(iso8601Buff);
}


  void setTime() {
      // Explicitly configure and sync time
    configTime(EST_OFFSET_SECONDS, 0, "pool.ntp.org", "time.google.com", "time.nist.gov");
    
    // Wait for time to be set
    time_t now = time(nullptr);
    int attempts = 0;
    while (now < 1600000000 && attempts < 10) {  // Check if time is reasonable
      delay(500);
      Serial.print(".");
      now = time(nullptr);
      attempts++;
    }

    if (now < 1600000000) {
      Serial.println("Failed to set time!");
    } else {
      Serial.println("Time synchronized successfully!");
    }

    initializeTimeZone();

  }
  /* Callback function to get the Email sending status */
  void smtpCallback(SMTP_Status status){
    Serial.println(status.info());
  
    if (status.success()){
  
      Serial.println("----------------");
      ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
      ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
      Serial.println("----------------\n");
  
      for (size_t i = 0; i < smtp.sendingResult.size(); i++)
      {
        /* Get the result item */
        SMTP_Result result = smtp.sendingResult.getItem(i);
          
        ESP_MAIL_PRINTF("Message No: %d\n", i + 1);
        ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
        ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
        ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
        ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
      }
      Serial.println("----------------\n");
  
      smtp.sendingResult.clear();
    }
}
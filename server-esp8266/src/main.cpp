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

// During Daylight Saving Time (EDT), it becomes UTC-4 hours
#define EST_OFFSET_SECONDS (-5 * 3600)
#define EDT_OFFSET_SECONDS (-4 * 3600)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Broadcast address
bool receivedMessage = false;
void initializeTimeZone() {
  // Explicitly set EST timezone
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1);
  tzset();
}

struct_message esp_message;
void connectToWifi();
void sendEmail(); 
void smtpCallback(SMTP_Status status);
void setTime();

void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  memcpy(&esp_message, incomingData, sizeof(esp_message));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Message: ");
  Serial.println(esp_message.message);
  Serial.print("IsOpen: ");
  Serial.println(esp_message.isOpen);
  Serial.println();

  // connectToWifi();
  receivedMessage = true;
}
SMTPSession smtp;
volatile bool criticalTasksComplete = false;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
  if (receivedMessage) {
    receivedMessage = false;
    connectToWifi();
  }
  delay(1000);
}

// void connectToWifi() {
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   Serial.println("Connecting to Wifi");
//   while(WiFi.status() != WL_CONNECTED) {
//     Serial.print(".");
//     delay(300);
//   }

//   Serial.println();
//   Serial.print("Connected with IP: ");
//   Serial.println(WiFi.localIP());
//   Serial.println();
// }


void disableESPNow() {
  // Unregister the receive callback
  esp_now_unregister_recv_cb();
  
  // Deinit ESP-NOW
  esp_now_deinit();
  
  Serial.println("ESP-NOW disabled");
}

void reEnableESPNow() {
  // Reinitialize ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error reinitializing ESP-NOW");
    return;
  }
  
  // Re-register receive callback
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("ESP-NOW re-enabled");
}

void connectToWifi() {
  // Disable ESP-NOW before WiFi connection
  disableESPNow();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wifi");
  
  unsigned long startAttemptTime = millis();
  const unsigned long WIFI_TIMEOUT_MS = 30000;  // 10 second timeout
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    
    if (millis() - startAttemptTime > WIFI_TIMEOUT_MS) {
      Serial.println("\nWiFi connection failed!");
      break;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected with IP: ");
    Serial.println(WiFi.localIP());
    
    // Send email or perform WiFi-dependent tasks here
    setTime();
    sendEmail();
  }
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

  // void OnDataRecv(uint8_t * mac, uint8_t *incomingData, uint8_t len) {
  //   memcpy(&myData, incomingData, sizeof(myData));
  //   Serial.print("Bytes received: ");
  //   Serial.println(len);
  //   Serial.print("Message: ");
  //   Serial.println(myData.message);
  //   Serial.print("IsOpen: ");
  //   Serial.println(myData.isOpen);
  //   Serial.println();
  // }
  
  
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

    // Rest of your setup continues...
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
#include <Arduino.h>
#include "secrets.h"
#include<ESP8266WiFi.h>
#include<ESP_Mail_Client.h>

#define REED_PIN 4  // GPIO4 (D2)

SMTPSession smtp;


void connectToWifi();
void sendEmail(); 
void smtpCallback(SMTP_Status status);

void setup() {
  Serial.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);
  
  // Blink LED 3 times to indicate setup is running
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_BUILTIN, LOW);  // Turn ON (inverted logic)
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH); // Turn OFF
    delay(500);
  }
  Serial.println("Reed switch is CLOSED (magnet detected) - Waking up!");

  pinMode(REED_PIN, INPUT_PULLUP);  // Enable internal pull-up resistor

  connectToWifi();
  sendEmail();

  Serial.println("Going to sleep...");
  delay(5000);
  
  ESP.deepSleep(0);  // Sleep until reset
}

void loop() {
  // this shouldn't run since the board should sleep right after setup every time
  Serial.println("Not sleeping...");
  delay(1000);
}

void connectToWifi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Connecting to Wifi");
  while(WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }

  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
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
  
    message.sender.name = F("Smart Medication");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Bottle Opened!");
    message.addRecipient(F("Masa"), RECIPIENT_EMAIL);  
     
    String textMsg = "You took your medication!";
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
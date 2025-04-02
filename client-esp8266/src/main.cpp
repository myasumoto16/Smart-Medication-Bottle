#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "secrets.h"
#include <message.h>
#include <espnow.h>

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus);
void printMacAddress(uint8_t* macaddr);

struct_message myData;

void setup() {
  Serial.begin(115200);
  
  Serial.println("\n\nESP-NOW Sender");
  
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != 0) {
      Serial.println("Error initializing ESP-NOW");
      return;
    }
    Serial.println("Initialized ESP-NOW");

    // Once ESPNow is successfully Init, we will register for Send CB to
    // get the status of Trasnmitted packet
    esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);
    esp_now_register_send_cb(OnDataSent);
    
    // Register peer
    esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);
    strcpy(myData.message, "The bottle was OPENED!");
    myData.isOpen = true;

    // Send message via ESP-NOW
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    ESP.deepSleep(0);

}

void loop() {
  Serial.println("Not Sleeping");
  delay(1000);
}

void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("Transmission to MAC: ");
  printMacAddress(mac_addr);
  
  Serial.print("Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }

  // Additional diagnostic info
  Serial.print("Detailed Send Status: ");
  Serial.println(sendStatus);
}

void printMacAddress(uint8_t* macaddr) {
  for (int i = 0; i < 6; i++) {
    Serial.print(macaddr[i], HEX);
    if (i < 5) Serial.print(":");
  }
  Serial.println();
}
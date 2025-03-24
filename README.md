# Smart Medication Bottle

## Overview
An IoT-based medication tracking device that automatically sends text notifications when medication is taken, using a NodeMCU D1 Mini ESP8266 and reed switch.

## Why did I make this?
As someone who struggles with medication management, I created this device to solve a real challenge in my daily life. Taking my medication everyday has always been a struggle because of: 
- Forgetting to take my medications
- Not being sure about whether I've already taken my dose for the day (this was the bigger problem). 

This leads to risk of accidentally skipping doses or double-dosing

None of the available systems didn't work for me so I decided to make my own. 
- Provide a reliable method to track medication intake
- Create an automated reminder system
- Reduce the mental burden of medication management
- No more double dosing! 

## Features
- Detects medication bottle pickup using a reed switch
- Sends automated email notifications
- Low-power design with deep sleep mode
- Real-time timestamp tracking

## Hardware Components
- NodeMCU D1 Mini
- Reed Switch
- Magnet
- Breadboard
- Jumper Wires

## Circuit Diagram

## Breadboard Prototype

## Notification Example

## Circuit Connection
- Reed Switch connected to GPIO4 (D2)
- D0 pin connected to RST for deep sleep wake
- Powered by USB or battery

## Setup Instructions
1. Clone the repository
2. Install required libraries:
   - ESP8266WiFi
   - ESP_Mail_Client
   - NTPClient
3. Configure `secrets.h` with:
   - WiFi credentials
   - SMTP email settings

## Code Structure
- `DateTime.h`: Time extraction utilities
- `secrets.h`: Sensitive configuration
- Main sketch handles:
  - WiFi connection
  - Email sending
  - Deep sleep management

## Power Consumption
- Deep sleep mode enables long battery life
- Minimal power draw when idle

## Troubleshooting
- Ensure stable WiFi connection
- Check email server settings
- Verify reed switch placement

## Future Improvements
- Battery power optimization
- Multiple medication tracking
- Advanced logging

## License
[Specify your license]

## Authors
Masakazu Yasumoto

## Acknowledgments
- [Random Nerd Tutorial](https://randomnerdtutorials.com/)
- ESP8266 Community
- Open-source libraries used
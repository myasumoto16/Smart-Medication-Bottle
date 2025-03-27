#ifndef DATE_TIME_H
#define DATE_TIME_H

#include <Arduino.h>

struct DateTime {
  String date;     // e.g., "2024-03-24"
  String time;     // e.g., "14:30:45"
  String dayOfWeek; // e.g., "Sunday"
  String timeZone;  // e.g., "EST" or "EDT"
  bool isDST;       // Daylight Saving Time flag
};

DateTime getCurrentDateTime();
void initializeTimeZone();

#endif
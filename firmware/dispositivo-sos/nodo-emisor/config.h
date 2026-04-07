#pragma once

// =========================
// CONFIGURACIÓN GENERAL
// =========================
const long LORA_FREQUENCY = 868E6;
const long LORA_BANDWIDTH = 125E3;
const int LORA_SPREADING_FACTOR = 7;

// Intervalo de lectura de sensores
const unsigned long SENSOR_INTERVAL_MS = 60000;

// WiFi AP
const char WIFI_SSID[] = "ARGUSLINK_SOS";
const char WIFI_PASS[] = "12345678";

// Batería
const float BATTERY_DIVIDER_FACTOR = 2.0;
const float ADC_REF_VOLTAGE = 5.0;
const int ADC_MAX_VALUE = 1023;
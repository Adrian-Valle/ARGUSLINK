# 🛰️ Proyecto ArgusLink
> Sistema de monitoreo ambiental remoto basado en tecnología **LoRa** y **Arduino**.

Este repositorio contiene la implementación de un ecosistema de sensores para la recolección de datos climáticos (Temperatura, Humedad, Presión) y Calidad de Aire, transmitidos de forma inalámbrica mediante la librería `RadioLib`.

---

# 🛠️ Esquema de Hardware (Pinout)

A continuación se detalla la configuración de pines para ambos nodos del sistema. Es fundamental respetar los niveles de voltaje para evitar daños en los módulos de radio.

## 📡 1. Nodo Cliente (Emisor)
Basado en **Arduino Uno/Nano**. Este nodo recolecta los datos y los envía al nodo central.

### Módulo LoRa (SX1276 / RFM95)
*Comunicación mediante bus **SPI**.*

| Pin Módulo LoRa | Pin Arduino (Uno/Nano) | Función |
|:---:|:---:|:---|
| **VCC** | 3.3V | ⚠️ Alimentación (Crítico: 3.3V) |
| **GND** | GND | Tierra |
| **SCK** | D13 | SPI Clock |
| **MISO** | D12 | SPI MISO |
| **MOSI** | D11 | SPI MOSI |
| **NSS (CS)** | **D10** | Chip Select |
| **DIO0** | **D4** | Interrupción principal |
| **RST** | **D9** | Reset del módulo |
| **DIO1** | **D3** | Interrupción secundaria |

### Sensores Ambientales
| Sensor | Pin Sensor | Pin Arduino | Protocolo |
|:---:|:---:|:---:|:---:|
| **BME280** | VCC/GND | 3.3V / GND | - |
| **BME280** | SCL | **A5** | I2C (Clock) |
| **BME280** | SDA | **A4** | I2C (Data) |
| **MQ135** | VCC/GND | 5V / GND | - |
| **MQ135** | AO | **A0** | Analógico |

---

## 🏗️ 2. Nodo Productor (Gateway/Receptor)
Basado en **Arduino UNO R4 WiFi**. Este nodo recibe los datos y gestiona la subida a Firebase.

### Módulo LoRa (SX1276 / RFM95)
| Pin Módulo LoRa | Pin Arduino R4 WiFi | Función |
|:---:|:---:|:---|
| **VCC** | 3.3V | ⚠️ Alimentación (¡No usar 5V!) |
| **GND** | GND | Tierra |
| **SCK** | D13 | SPI Clock |
| **MISO** | D12 | SPI MISO |
| **MOSI** | D11 | SPI MOSI |
| **NSS / CS** | **D10** | Chip Select |
| **DIO0** | **D4** | Interrupción 0 |
| **RST** | **D9** | Reset |
| **DIO1** | **D3** | Interrupción 1 |

---

# ⚠️ Notas de Instalación

> [!IMPORTANT]
> **Protección del Radio:** Nunca enciendas los Arduinos sin tener conectada la **antena** en los módulos LoRa. Transmitir sin antena puede quemar el chip permanentemente.

> [!WARNING]
> **Voltajes:** El sensor MQ135 requiere 5V para que su filamento interno caliente correctamente, pero el módulo LoRa **SIEMPRE** debe alimentarse a 3.3V.

---

# 🚀 Tecnologías Utilizadas
* **Lenguaje:** C++ (Arduino IDE)
* **Librería de Radio:** `RadioLib`
* **Sensores:** `Adafruit BME280`, `MQ135 Library`
* **Conectividad:** LoRa (868/915 MHz) y WiFi (HTTP/SSL)
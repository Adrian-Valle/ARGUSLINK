# 🛠️ Lista de Materiales y Hardware

Este documento detalla los componentes físicos utilizados para el desarrollo del prototipo **ARGUSLINK**. El hardware ha sido seleccionado para cumplir con los requisitos de monitorización ambiental y comunicación inalámbrica de largo alcance (LoRa).

---

## 💻 Control y Procesamiento

### 🔹 3x Arduino UNO R4 WiFi
El cerebro del proyecto. Se utilizan tres unidades para distribuir las tareas entre nodos sensores y un nodo receptor/gateway.
* **Procesador:** Renesas RA4M1 (32-bit ARM Cortex-M4).
* **Conectividad:** Wi-Fi y Bluetooth integrados (ESP32-S3).
* **Extras:** Matriz de LED 12x8 para visualización rápida de estados.

---

## 📡 Comunicación de Largo Alcance (LoRa)

| Componente | Cantidad | Función |
| :--- | :---: | :--- |
| **Módulos LoRa RFM95W** | 2 | Transmisión de datos por radiofrecuencia a larga distancia con bajo consumo. |
| **Antenas para LoRa** | 2 | Sintonizadas para maximizar la ganancia y estabilidad de la señal inalámbrica. |

---

## 🌡️ Sensores de Monitorización

### 🟢 Sensor BME280
Sensor de precisión "3 en 1" para variables atmosféricas.
* **Mediciones:** Temperatura, Humedad y Presión barométrica.
* **Interfaz:** I2C / SPI.

### 🟠 Sensor MQ135
Sensor de calidad del aire para la detección de gases nocivos.
* **Detección:** Amoníaco (NH3), óxidos de nitrógeno (NOx), alcohol, sulfuro, benceno y humo.
* **Uso:** Ideal para sistemas de alerta de seguridad ambiental.

---

## 🔌 Prototipado y Estructura

* **2x Protoboards (830 puntos):** Tableros de pruebas para el conexionado rápido y seguro de los componentes sin necesidad de soldadura.
* **Jumpers / Cableado:** Cables de interconexión (macho-macho y macho-hembra) para unir los sensores a las placas Arduino.

---

## 📊 Resumen de Inventario

| Icono | Material | Cantidad |
| :---: | :--- | :---: |
| 🧩 | Arduino UNO R4 WiFi | 3 |
| 📟 | Módulo LoRa RFM95W | 2 |
| 📶 | Antenas LoRa | 2 |
| 🌡️ | Sensor BME280 | 1 |
| 💨 | Sensor MQ135 | 1 |
| 🧰 | Protoboards | 2 |

---
> [!TIP]
> **Nota de montaje:** Asegúrese de conectar las antenas a los módulos LoRa antes de alimentar el sistema para evitar daños en los transceptores RFM95W.
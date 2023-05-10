# Enclosure3D
Control de ambiente para un gabinete de impresión 3D

(Foto componentes)

 ### Componentes
 * NodeMCU 8266 V2 o V3
 * Display 0.91 oled
 * Sensor de humedad y temperatura DHT11 o DHT22
 * Rotary encoder

## Parámetros técnicos
* Dimensiones:  
* Material: PLA
* Alimentación: 12v CC
* Consumo: 

## Puertos
El dispositivo presenta 3 salidas para el control de ambiente:
* Iluminación (LUZ) 12v @ 1A.
* Ventilación (VT1) 12v @ 1A.
* Ventilación (VT2) 12v @ 2A.

### Diagrama de conexionado

```mermaid
graph LR;
id0("Vcc 12v")-->id1[Enclosure3D];
id1[Enclosure3D]--pwm-->id2["Iluminación(LUZ)"];
id1[Enclosure3D]--pwm-->id3["Ventilación 1 (VT1)"];
id1[Enclosure3D]--pwm-->id4["Ventilación 2 (VT2)"];

id1[Enclosure3D]--i2c-->id5["Display 128x32"];
id6["Encoder"]-->id1[Enclosure3D];
id7["DHT"]-->id1[Enclosure3D];

```

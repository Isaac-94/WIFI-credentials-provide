| Chip soportados| ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C6 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | -------- |

# Carga de credenciales WIFI mediante portal cautivo

En este proyeccto se resuelve el problema de cargar las credenciales de la red wifi a la cual se desea que el dispositivo se conecte, mediante la carga de las credenciales en un portal cautivo. 

## Como funciona
 El dispositivo inicia y se configura en modo AP, habilita una red WIFI llamada QMAX_CONFIG_WIFI a la cual se debe conectar usando un dispositivo movil, tablet o pc, ingresando la contrasñea 12345678. 
 Una vez conectado a la red WIFI, se debe abrir el navegador y buscar la ip 192.168.4.1, que mostrará una web con dos campos, correspondientes al SSID y contraseña de la red WIFI a la que desea conectarse. Una vez cargadas las credenciales, presionar el botón "submit" y si las credenciales son correctas el dispositivo se conectará a la red WIFI.

## Configuración
Antes de compilar el proyecto, seleccione el chip con el que trabajará ingresando el siguiente comando en el powershell de ESP-IDF `idf.py set-target <chip_name>`.
Una vez clonado el proyecto, se debe compilar ingresando el siguiente comando en el powershell de ESP-IDF `idf.py build`.
Una vez creado compilado el proyecto, se debe grabar en el chip y monitorear en la terminal, ingresando el siguiente comando en el powershell de ESP-IDF `idf.py flash monitor`.


### Hardware Requerido

* Placa con chip ESP32/ESP32-S2/ESP32-C3 SoC (ej., ESP32-DevKitC, ESP-WROVER-KIT, etc.)
* Cable USB 
* Red WiFi 2.4GHz



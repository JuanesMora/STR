# Sistema Automatizado de Control Ambiental - ESP32C6 / IoT

Sistema embebido desarrollado sobre **ESP32C6** para automatizar y supervisar variables de una habitación inteligente. El proyecto integra control de actuadores, lectura de sensores, automatización horaria, conectividad Wi-Fi, actualización inalámbrica de firmware y una interfaz web embebida.

## Objetivo

Diseñar una arquitectura modular capaz de ejecutar tareas de control ambiental en tiempo real y ofrecer supervisión remota desde una interfaz web.

El proyecto combina conceptos de:

- Sistemas embebidos.
- Sistemas en tiempo real.
- Control mediante PWM.
- Adquisición de señales analógicas.
- Automatización horaria.
- Comunicación Wi-Fi.
- Servidor HTTP embebido.
- Actualización OTA.
- Internet de las Cosas (IoT).

## Arquitectura general

```text
                  +----------------------+
                  |        ESP32C6         |
                  |      ESP-IDF / C     |
                  +----------+-----------+
                             |
          +------------------+------------------+
          |                  |                  |
          v                  v                  v
      Sensores           Control PWM       Servidor HTTP
          |                  |                  |
          v                  v                  v
     Estado del          Ventilación /      Interfaz web
      sistema             actuadores            |
          |                  |                  |
          +------------------+------------------+
                             |
                             v
                    Automatización y lógica
                             |
                             v
                         OTA / Wi-Fi
```

## Funcionalidades principales

- Control proporcional de actuadores mediante **PWM**.
- Lectura de variables analógicas asociadas al ambiente.
- Control de iluminación RGB.
- Manejo de servomotores y actuadores.
- Alarmas mediante LED.
- Gestión de estado global de la aplicación.
- Persistencia de configuración.
- Tareas modulares para ejecución concurrente.
- Conectividad Wi-Fi.
- Servidor HTTP para supervisión y control.
- Actualizaciones inalámbricas **OTA**.

## Estructura del código

El proyecto utiliza una organización modular. Algunos de los componentes principales dentro de `main/` son:

```text
main/
├── main.c
├── app_state.c / app_state.h
├── app_storage.c / app_storage.h
├── app_tasks.c / app_tasks.h
├── fan_pwm.c / fan_pwm.h
├── ntc_adc.c / ntc_adc.h
├── rgb_ctrl.c / rgb_ctrl.h
├── rgb_led.c / rgb_led.h
├── servo_ctrl.c / servo_ctrl.h
├── alarm_led.c / alarm_led.h
├── http_server.c / http_server.h
├── wifi_app.c / wifi_app.h
└── webpage/
```

Esta separación permite desacoplar la lógica de cada periférico y facilita el mantenimiento, la prueba y la evolución del firmware.

## Tecnologías

| Tecnología | Aplicación |
|---|---|
| ESP32 | Plataforma embebida principal |
| C | Desarrollo del firmware |
| ESP-IDF | Framework de desarrollo |
| PWM | Control proporcional de actuadores |
| ADC | Lectura de sensores analógicos |
| Wi-Fi | Conectividad de red |
| HTTP | Interfaz de supervisión |
| OTA | Actualización remota del firmware |
| CMake | Construcción del proyecto |

## Compilación y ejecución

Se requiere tener instalado y configurado **ESP-IDF**.

```bash
# Entrar a la carpeta del proyecto
cd Proyecto_STR_2026_v1

# Configurar el proyecto si es necesario
idf.py menuconfig

# Compilar
idf.py build

# Flashear el ESP32
idf.py -p PORT flash

# Abrir el monitor serial
idf.py -p PORT monitor
```

También puede utilizarse:

```bash
idf.py -p PORT flash monitor
```

Reemplazar `PORT` por el puerto correspondiente al dispositivo.

## Configuración OTA

El proyecto incluye un esquema de particiones para actualización OTA mediante el archivo:

```text
partitions_two_ota.csv
```

Antes de realizar actualizaciones remotas se recomienda verificar la configuración de particiones, la red Wi-Fi y el mecanismo de recuperación ante fallos.

## Interfaz web

La carpeta `main/webpage/` contiene los recursos asociados a la interfaz web embebida. Esta interfaz permite visualizar el estado del sistema y controlar funciones disponibles desde un navegador conectado a la red correspondiente.

## Buenas prácticas de seguridad

- No almacenar contraseñas Wi-Fi ni credenciales sensibles directamente en un repositorio público.
- Utilizar archivos de configuración excluidos mediante `.gitignore` cuando sea necesario.
- Evitar publicar claves, tokens o endpoints privados.

## Contexto académico y autoría

Proyecto desarrollado de forma colaborativa durante la formación en Ingeniería Electrónica de la **Universidad Nacional de Colombia - Sede Manizales**.

Repositorio personal de **Juan Esteban Mora Díaz**, quien participó en el desarrollo y documentación del sistema.

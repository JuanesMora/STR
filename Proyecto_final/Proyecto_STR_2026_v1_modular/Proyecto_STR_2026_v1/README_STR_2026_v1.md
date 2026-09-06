# Proyecto STR 2026 v1 - Sistema Automatizado de Control Ambiental

Primera versión modular para ESP32-C6 con ESP-IDF, basada en el proyecto `http_sever_and_flash_program` del profesor.

## Qué incluye

- WiFi AP + STA reutilizado del proyecto base.
- Servidor web embebido con `index.html`, `app.css` y `app.js`.
- OTA funcional conservando `/OTAupdate` y `/OTAstatus`.
- NTC 14 kOhm con Beta 3950.
- Ventilador PWM automático proporcional o manual.
- RGB ambiental para LED de cátodo común.
- Servo SG90 continuo 360 grados por tiempo de giro.
- LED rojo de alarma con parpadeo a 1 Hz cuando la temperatura supera la máxima.
- 8 registros de programación de cortina con persistencia en NVS.

## Archivo clave para cambiar pines y calibraciones

Edita este archivo:

```txt
main/app_config.h
```

Allí puedes cambiar:

- `APP_NTC_R0_OHM` si el NTC no es de 14 kOhm.
- `APP_NTC_BETA` si el Beta no es 3950.
- `APP_NTC_FIXED_RESISTOR_OHM` si la resistencia fija del divisor no es de 10 kOhm.
- `APP_NTC_TO_GND` si tu divisor está conectado al revés.
- Pines GPIO de RGB, ventilador, servo y alarma.
- Tiempos del servo continuo: `APP_SERVO_FULL_TRAVEL_MS`, `APP_SERVO_OPEN_US`, `APP_SERVO_CLOSE_US` y `APP_SERVO_STOP_US`.

## Comandos recomendados

```bash
idf.py set-target esp32c6
idf.py fullclean
idf.py build
idf.py -p COM3 flash monitor
```

Cambia `COM3` por el puerto real de tu placa.

## Nota sobre el servo SG90 continuo 360

Este servo no conoce posiciones absolutas. Por eso 0%, 50% y 100% se estiman por tiempo de giro. Si no queda preciso, ajusta:

```c
#define APP_SERVO_FULL_TRAVEL_MS 2500
#define APP_SERVO_STOP_US        1500
#define APP_SERVO_OPEN_US        1700
#define APP_SERVO_CLOSE_US       1300
```

## Nota sobre RGB

El proyecto está configurado para LED RGB de cátodo común:

```c
#define APP_RGB_COMMON_CATHODE 1
```

Si algún día usas ánodo común, cambia ese valor a `0`.

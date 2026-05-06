# -*- coding: utf-8 -*-
"""
Created on Fri May  1 16:07:44 2026

@author: miria
"""
import requests
import time
from datetime import datetime

# ==============================
# CONFIGURACIÓN THINGSPEAK
# ==============================

CHANNEL_ID = "3365553"

# Si tu canal es público, puedes dejarlo vacío:
READ_API_KEY = "DQBLZJD5NW32QWW7"

# Si tu canal es privado, pon aquí tu Read API Key:
# READ_API_KEY = "TU_READ_API_KEY"

INTERVALO_LECTURA = 20  # segundos

# Umbral de luz para detectar posible apertura
UMBRAL_LUZ_APERTURA = 50  # lux, puedes ajustarlo según tus pruebas
aaa=5;
bbb=6;
ccc=7;


def leer_datos_thingspeak():
    """
    Lee el último dato enviado al canal de ThingSpeak.
    """

    url = f"https://api.thingspeak.com/channels/{CHANNEL_ID}/feeds/last.json"

    params = {}

    if READ_API_KEY:
        params["api_key"] = READ_API_KEY

    try:
        respuesta = requests.get(url, params=params, timeout=10)

        if respuesta.status_code != 200:
            print(f"Error al leer ThingSpeak. Código HTTP: {respuesta.status_code}")
            return None

        datos = respuesta.json()
        return datos

    except requests.exceptions.RequestException as e:
        print("Error de conexión:", e)
        return None


def convertir_float(valor, defecto=0.0):
    try:
        return float(valor)
    except:
        return defecto


def convertir_int(valor, defecto=0):
    try:
        return int(float(valor))
    except:
        return defecto


def interpretar_datos(datos):
    """
    Interpreta los datos recibidos desde ThingSpeak.
    """

    temperatura = convertir_float(datos.get("field1"))
    luz = convertir_float(datos.get("field2"))
    tarjeta_detectada = convertir_int(datos.get("field3"))
    acceso = convertir_int(datos.get("field4"), -1)
    estado_temp = convertir_int(datos.get("field5"))
    caja_estado = convertir_int(datos.get("field6"))

    fecha = datos.get("created_at", "Sin fecha")

    print("\n==============================")
    print("LECTURA DE THINGSPEAK")
    print("==============================")
    print(f"Fecha ThingSpeak: {fecha}")
    print(f"Temperatura: {temperatura:.2f} °C")
    print(f"Luz: {luz:.2f} lux")

    if tarjeta_detectada == 1:
        print("Tarjeta: detectada")
    else:
        print("Tarjeta: no detectada")

    if acceso == 1:
        print("Acceso RFID: autorizado")
    elif acceso == 0:
        print("Acceso RFID: denegado")
    else:
        print("Acceso RFID: sin lectura")

    if caja_estado == 1:
        print("Caja: abierta")
    else:
        print("Caja: cerrada")

    if estado_temp == 1:
        print("Estado temperatura: OK")
    elif estado_temp == 2:
        print("Estado temperatura: AVISO")
    elif estado_temp == 0:
        print("Estado temperatura: PELIGRO")
    else:
        print("Estado temperatura: desconocido")

    comprobar_alertas(
        temperatura,
        luz,
        tarjeta_detectada,
        acceso,
        estado_temp,
        caja_estado
    )


def comprobar_alertas(temperatura, luz, tarjeta_detectada, acceso, estado_temp, caja_estado):
    """
    Genera alertas según los valores recibidos.
    """

    print("\n--- ALERTAS ---")

    hay_alerta = False

    # Alerta de temperatura
    if estado_temp == 0:
        print("ALERTA: Temperatura peligrosa para el transporte del órgano.")
        hay_alerta = True
    elif estado_temp == 2:
        print("AVISO: Temperatura algo elevada. Revisar la caja.")
        hay_alerta = True

    # Alerta de acceso denegado
    if acceso == 0:
        print("ALERTA: Se ha usado una tarjeta no autorizada.")
        hay_alerta = True

    # Posible apertura forzosa
    # Interpretación:
    # - Hay mucha luz dentro
    # - La caja aparece abierta
    # - No consta acceso autorizado
    if luz > UMBRAL_LUZ_APERTURA and caja_estado == 1 and acceso != 1:
        print("ALERTA: Posible apertura forzosa detectada.")
        hay_alerta = True

    # Otra posible condición:
    # mucha luz aunque la caja debería estar cerrada
    if luz > UMBRAL_LUZ_APERTURA and caja_estado == 0:
        print("ALERTA: Hay luz dentro aunque la caja figura como cerrada.")
        hay_alerta = True

    if not hay_alerta:
        print("Sin alertas.")


def main():
    print("Sistema de lectura de caja de órganos desde ThingSpeak")
    print("Leyendo datos...")

    while True:
        datos = leer_datos_thingspeak()

        if datos:
            interpretar_datos(datos)
        else:
            print("No se han podido obtener datos.")

        time.sleep(INTERVALO_LECTURA)


if __name__ == "__main__":
    main()

import asyncio
import struct
import threading
import csv
import os
from datetime import datetime

from bleak import BleakClient
from flask import Flask, render_template, send_file
from flask_socketio import SocketIO

# ════════════════════════════════════════════
#  CONFIGURACIÓN
# ════════════════════════════════════════════
ESP32_MAC = "F4:2D:C9:73:DC:82"    # ← PON TU MAC AQUÍ

SENSOR_UUID  = "12340002-5678-9abc-def0-123456789abc"
CONTROL_UUID = "12340003-5678-9abc-def0-123456789abc"
HIST_UUID    = "12340004-5678-9abc-def0-123456789abc"

# ════════════════════════════════════════════
#  FLASK + SOCKET.IO
# ════════════════════════════════════════════
app = Flask(__name__)
app.config['SECRET_KEY'] = 'biorreactor2026'
socketio = SocketIO(app, cors_allowed_origins="*", async_mode='threading')

ble_client = None
ble_connected = False

# ★ Contador de muestras — nunca se reinicia
sample_counter = 0

os.makedirs("logs", exist_ok=True)
LOG_FILE = "logs/biorreactor_log.csv"

if not os.path.exists(LOG_FILE):
    with open(LOG_FILE, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["muestra", "timestamp", "temp", "setpoint", "duty",
                         "kp", "ki", "kd", "rpm", "flags", "severity"])


def decodificar_sensor(data):
    global sample_counter

    if len(data) < 30:
        return None

    temp, sp, duty, kp, ki, kd, rpm, flags, sev = struct.unpack(
        '<ffffffIBB', data)

    sample_counter += 1

    return {
        "muestra": sample_counter,
        "time": datetime.now().strftime("%H:%M:%S"),
        "temp": round(temp, 2),
        "setpoint": round(sp, 1),
        "duty": round(duty, 1),
        "kp": round(kp, 2),
        "ki": round(ki, 2),
        "kd": round(kd, 4),
        "rpm": rpm,
        "flags": flags,
        "severity": sev
    }


def decodificar_errores(flags):
    errores = []
    if flags & 0x01: errores.append({"tipo": "info",  "msg": "Boost activo"})
    if flags & 0x02: errores.append({"tipo": "info",  "msg": "Rampa activa"})
    if flags & 0x04: errores.append({"tipo": "info",  "msg": "Heater bloqueado"})
    if flags & 0x08: errores.append({"tipo": "warn",  "msg": "Temperatura alta"})
    if flags & 0x10: errores.append({"tipo": "warn",  "msg": "Temperatura baja"})
    if flags & 0x20: errores.append({"tipo": "warn",  "msg": "Motor sin RPM"})
    if flags & 0x40: errores.append({"tipo": "crit",  "msg": "Sensor PT100 fallo"})
    if flags & 0x80: errores.append({"tipo": "crit",  "msg": "Sobretemperatura"})
    return errores


# ════════════════════════════════════════════
#  BLE
# ════════════════════════════════════════════
def notification_handler(sender, data):
    paquete = decodificar_sensor(data)
    if paquete is None:
        return

    paquete["errores"] = decodificar_errores(paquete["flags"])
    socketio.emit('sensor_data', paquete)

    try:
        with open(LOG_FILE, 'a', newline='') as f:
            writer = csv.writer(f)
            writer.writerow([
                paquete["muestra"], paquete["time"],
                paquete["temp"], paquete["setpoint"],
                paquete["duty"], paquete["kp"], paquete["ki"],
                paquete["kd"], paquete["rpm"], paquete["flags"],
                paquete["severity"]
            ])
    except Exception as e:
        print(f"Error guardando log: {e}")


async def ble_loop():
    global ble_client, ble_connected

    while True:
        try:
            print(f"🔗 Conectando a {ESP32_MAC}...")
            async with BleakClient(ESP32_MAC, timeout=10.0) as client:
                ble_client = client
                ble_connected = True
                print("✅ Conectado al ESP32!")
                socketio.emit('ble_status', {'connected': True})

                await client.start_notify(SENSOR_UUID, notification_handler)

                while client.is_connected:
                    await asyncio.sleep(1)

                print("⚠️ Conexión perdida")

        except Exception as e:
            print(f"❌ Error BLE: {e}")

        ble_connected = False
        ble_client = None
        socketio.emit('ble_status', {'connected': False})
        print("🔄 Reintentando en 3 segundos...")
        await asyncio.sleep(3)


def start_ble_thread():
    loop = asyncio.new_event_loop()
    asyncio.set_event_loop(loop)
    loop.run_until_complete(ble_loop())


# ════════════════════════════════════════════
#  RUTAS WEB
# ════════════════════════════════════════════
@app.route('/')
def index():
    return render_template('index.html')


@app.route('/descargar_csv')
def descargar_csv():
    if os.path.exists(LOG_FILE):
        return send_file(LOG_FILE, as_attachment=True,
                         download_name='biorreactor_datos.csv')
    else:
        return "No hay datos todavía", 404


# ════════════════════════════════════════════
#  HISTORIAL BLE
# ════════════════════════════════════════════
@socketio.on('download_history')
def handle_download():
    global ble_client

    if ble_client is None or not ble_connected:
        socketio.emit('history_status', {'msg': 'No conectado'})
        return

    history_data = []

    def hist_handler(sender, data):
        if len(data) == 4:
            idx = struct.unpack('<H', data[:2])[0]
            if idx == 0xFFFF:
                count = struct.unpack('<H', data[2:4])[0]
                socketio.emit('history_complete', {
                    'data': history_data,
                    'total': count
                })
                print(f"📥 Historial recibido: {count} entradas")
        elif len(data) == 10:
            idx, time_s, temp = struct.unpack('<HIf', data)
            history_data.append({
                'index': idx,
                'time_s': time_s,
                'temp': round(temp, 2)
            })

    async def download():
        try:
            await ble_client.start_notify(HIST_UUID, hist_handler)
            await asyncio.sleep(0.5)
            await ble_client.write_gatt_char(CONTROL_UUID, bytes([0x10]))
            socketio.emit('history_status', {'msg': 'Descargando...'})
            await asyncio.sleep(30)
            await ble_client.stop_notify(HIST_UUID)
        except Exception as e:
            print(f"❌ Error descargando historial: {e}")
            socketio.emit('history_status', {'msg': f'Error: {e}'})

    loop = asyncio.new_event_loop()
    loop.run_until_complete(download())
    loop.close()


# ════════════════════════════════════════════
#  ARRANCAR
# ════════════════════════════════════════════
if __name__ == '__main__':
    print("🧬 Biorreactor Dashboard")
    print("=" * 40)

    ble_thread = threading.Thread(target=start_ble_thread, daemon=True)
    ble_thread.start()

    print("🌐 Dashboard en: http://localhost:5000")
    print("📱 Desde celular: http://<IP-de-tu-PC>:5000")
    print("=" * 40)
    socketio.run(app, host='0.0.0.0', port=5000, debug=False)
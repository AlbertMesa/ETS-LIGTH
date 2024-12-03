#buscar una manera de saber a quien le toca estar en verde aplicando un contador para no determinar la cantidad de cara

#confirmacion seria saber cuando ya tiene que mandar el proximo analisis de la cantidad de vehiculo

#PARA EL MICRO SI TAL VARIABLE NO ES VERDE Y EL TOPICO ES EL RECIBE EL QUE SE PONGA TODO ROJO

from flask import Flask, render_template_string
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import threading

app = Flask(__name__)
socketio = SocketIO(app)

# Tópicos MQTT
TOPIC_CAMARA_1       = "TEST/RBPI1"
TOPIC_CAMARA_2       = "TEST/RBPI2"
TOPIC_ESTADO_SEM_1   = "TEST/ESTADO_1"
TOPIC_ESTADO_SEM_2   = "TEST/ESTADO_2"
TOPIC_TIEMPO_ESTADO  = "TEST/tiempo"
TOPIC_PREDETERMINADO = "TEST/MODO"
TOPIC_EMERGENCIA     = "TEST/EMERGENCIA"
TOPIC_CONFIRMACION_1 = "TEST/SEMAFORO_1/conf"
TOPIC_CONFIRMACION_2 = "TEST/SEMAFORO_2/conf"

TOPIC_MICRO_1 = "TEST/SEMAFORO_1"
TOPIC_MICRO_2 = "TEST/SEMAFORO_2"



CANTIDAD_DE_SEMAFORO = 1 #CONTANDO EL 0

CONT = 0
listo_1 = None
listo_2 = None

# Plantilla HTML para la página
html_template = """
<!doctype html>
<html>
<head>
    <title>MQTT Live Data</title>
    <script src="https://cdnjs.cloudflare.com/ajax/libs/socket.io/4.3.2/socket.io.js"></script>
     <style>
        /* Estilos generales */
        body {
            font-family: Arial, sans-serif;
            background-color: #f4f6f9;
            color: #333;
            margin: 0;
            padding: 0;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            position: relative;
        }
        h1 {
            font-size: 1.5em;
            color: #4a90e2;
            margin-bottom: 10px;
            text-align: center;
        }
        #content {
            background-color: #ffffff;
            border-radius: 8px;
            box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.1);
            padding: 20px;
            width: 80%;
            max-width: 500px;
            text-align: center;
        }
        .mqtt-data {
            font-size: 1.2em;
            font-weight: bold;
            color: #4a4a4a;
            background: #e3f2fd;
            border-radius: 8px;
            padding: 15px;
            margin-top: 10px;
            margin-bottom: 20px;
        }
        /* Estilos para el cuadro del estado */
        #status-box {
            position: absolute;
            top: 20px;
            right: 20px;
            background-color: #ffffff;
            border-radius: 8px;
            box-shadow: 0px 4px 12px rgba(0, 0, 0, 0.1);
            padding: 10px;
            width: 200px;
            text-align: center;
        }
        #status-box h2 {
            font-size: 1.2em;
            color: #4a90e2;
            margin-bottom: 10px;
        }
        #status-box .status {
            font-size: 1em;
            font-weight: bold;
            color: #333;
            margin: 5px 0;
        }
            </style>
            <script type="text/javascript">
                var socket = io();
                let currentModeAuto = "Predeterminado";
                let currentModeEmer = "OFF";

                // Cambiar modo automático/predeterminado
                function toggleModeAuto() {
                    currentModeAuto = currentModeAuto === "Predeterminado" ? "Automático" : "Predeterminado";
                    document.getElementById('mode-status1').innerText = currentModeAuto;

                    const modeValue = currentModeAuto === "Automático" ? true : false;
                    socket.emit('change_mode_auto', { mode: modeValue });
                }

                // Cambiar modo emergencia ON/OFF
                function toggleModeEmer() {
                    currentModeEmer = currentModeEmer === "OFF" ? "ON" : "OFF";
                    document.getElementById('mode-status2').innerText = currentModeEmer;

                    const modeValue = currentModeEmer === "ON" ? true : false;
                    socket.emit('change_mode_emergency', { mode: modeValue });
                }
                
                // Actualiza los estados de las caras en la interfaz
                socket.on('cara_1', function(data) {
                    document.getElementById('cara_1').innerText = data.message1;
                });

                socket.on('cara_2', function(data) {
                    document.getElementById('cara_2').innerText = data.message2;
                });

                // Actualiza los estados de los semáforos
                socket.on('status_semaforo_1', function(data) {
                    document.getElementById('status_semaforo_1').innerText = data.estado1 || "Desconocido";
                });

                socket.on('status_semaforo_2', function(data) {
                    document.getElementById('status_semaforo_2').innerText = data.estado2 || "Desconocido";
                });
            </script>
</head>
<body>
    <div id="content">
        <h1>Cantidad de Carros en Cara 1</h1>
        <p id="cara_1" class="mqtt-data">Esperando mensaje...</p>
        
        <h1>Cantidad de Carros en Cara 2</h1>
        <p id="cara_2" class="mqtt-data">Esperando mensaje...</p>
    </div>

    <!-- Cuadro de estado de semáforos -->
    <div id="status-box">
        <h2>Estado de Semáforos</h2>
        <p class="status">Semáforo 1: <span id="status_semaforo_1">Desconocido</span></p>
        <p class="status">Semáforo 2: <span id="status_semaforo_2">Desconocido</span></p>
    </div>
    
    <div id="content">
        <button onclick="toggleModeAuto()">Cambiar a Automático</button>
        <p id="mode-status1">Predeterminado</p>
        
        <button onclick="toggleModeEmer()">Cambiar a ON</button>
        <p id="mode-status2">OFF</p>
    </div>
</body>
</html>
"""

# Callback de conexión MQTT
def on_connect(client, userdata, flags, rc):
    print("Conectado al broker MQTT con código de resultado " + str(rc))
    client.subscribe(TOPIC_CAMARA_1)
    client.subscribe(TOPIC_CAMARA_2)
    client.subscribe(TOPIC_ESTADO_SEM_1)
    client.subscribe(TOPIC_ESTADO_SEM_2)
    client.subscribe(TOPIC_TIEMPO_ESTADO)
    client.subscribe(TOPIC_CONFIRMACION_1)
    client.subscribe(TOPIC_CONFIRMACION_2)

# Callback de mensaje recibido
def on_message(client, userdata, msg):
    global mensaje1, mensaje2, mensaje3, mensaje4, listo_1, listo_2, CONT

    # Verificar de qué topic llegó el mensaje
    if msg.topic == TOPIC_CAMARA_1:
        mensaje1 = msg.payload.decode()
        print(f"Mensaje recibido en {TOPIC_CAMARA_1}: {mensaje1}")
        socketio.emit('cara_1', {'message1': mensaje1})
        
    elif msg.topic == TOPIC_CAMARA_2:
        mensaje2 = msg.payload.decode()
        print(f"Mensaje recibido en {TOPIC_CAMARA_2}: {mensaje2}")
        socketio.emit('cara_2', {'message2': mensaje2})
        
    elif msg.topic == TOPIC_ESTADO_SEM_1:
        mensaje3 = msg.payload.decode()
        print(f"Mensaje recibido en {TOPIC_ESTADO_SEM_1}: {mensaje3}")
        socketio.emit('status_semaforo_1', {'estado1': mensaje3})
        
    elif msg.topic == TOPIC_ESTADO_SEM_2:
        mensaje4 = msg.payload.decode()
        print(f"Mensaje recibido en {TOPIC_ESTADO_SEM_2}: {mensaje4}")
        socketio.emit('status_semaforo_2', {'estado2': mensaje4})
    
    elif msg.topic == TOPIC_CONFIRMACION_1:
        listo_1 = int(msg.payload.decode())
        print(f"Mensaje recibido en {TOPIC_CONFIRMACION_1}: {listo_1}")
        
    elif msg.topic == TOPIC_CONFIRMACION_2:
        listo_2 = int(msg.payload.decode())
        print(f"Mensaje recibido en {TOPIC_CONFIRMACION_2}: {listo_2}")
        
        
        
    if listo_1 is not None and listo_2 is not None:
        
        if CONT == 0:
            
            mqtt_client.publish(TOPIC_MICRO_1 , str("VERDE"))
            mqtt_client.publish(TOPIC_MICRO_2 , str("ROJO"))
            mqtt_client.publish(TOPIC_TIEMPO_ESTADO , "25")
            listo_1 = None
            listo_2 = None
            
        elif CONT == 1:
            mqtt_client.publish(TOPIC_MICRO_1 , str("ROJO"))
            mqtt_client.publish(TOPIC_MICRO_2 , str("VERDE"))
            mqtt_client.publish(TOPIC_TIEMPO_ESTADO , "25")
            listo_1 = None
            listo_2 = None
            
        if CONT >= CANTIDAD_DE_SEMAFORO:
            CONT = 0
        else: 
            CONT = CONT + 1
        
        
        
    

# Manejo del modo automático/predeterminado
@socketio.on('change_mode_auto')
def handle_change_mode_auto(data):
    mode_value = data.get('mode', 0)
    mqtt_client.publish(TOPIC_PREDETERMINADO, str(mode_value))
    print(f"Modo automático/predeterminado cambiado: {'Automático' if mode_value == "true" else 'Predeterminado'}")

# Manejo del modo emergencia ON/OFF63-+
@socketio.on('change_mode_emergency')
def handle_change_mode_emergency(data):
    mode_value = data.get('mode', 0)
    mqtt_client.publish(TOPIC_EMERGENCIA, str(mode_value))
    print(f"Modo emergencia cambiado: {'ON' if mode_value == "true" else 'OFF'}")

# Configura el cliente MQTT
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect("localhost", 1883, 60)

# Ejecuta MQTT en un hilo separado
def mqtt_thread():
    mqtt_client.loop_forever()

threading.Thread(target=mqtt_thread).start()

# Ruta de la página principal
@app.route("/")
def index():
    return render_template_string(html_template)

# Ejecuta la aplicación Flask y SocketIO
if __name__ == "__main__":
    socketio.run(app, debug=True)

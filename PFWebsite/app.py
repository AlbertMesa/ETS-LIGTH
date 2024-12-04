from flask import Flask, render_template
from flask_socketio import SocketIO
import paho.mqtt.client as mqtt
import threading

app = Flask(__name__)
socketio = SocketIO(app)

# Definición de los tópicos MQTT
TOPIC_CAMARA_1 = "TEST/SEMAFORO_1/RBPI"
TOPIC_CAMARA_2 = "TEST/SEMAFORO_2/RBPI"
TOPIC_ESTADO_SEM_1 = "TEST/ESTADO_1"
TOPIC_ESTADO_SEM_2 = "TEST/ESTADO_2"
TOPIC_TIEMPO_ESTADO = "TEST/TIEMPO"
TOPIC_TIEMPO_PREDETERMINADO = "TEST/TIEMPO/PREDETERMINADO"
TOPIC_PREDETERMINADO = "TEST/MODO"
TOPIC_EMERGENCIA = "TEST/EMERGENCIA"
TOPIC_CONFIRMACION_1 = "TEST/SEMAFORO_1/conf"
TOPIC_CONFIRMACION_2 = "TEST/SEMAFORO_2/conf"

TOPIC_MICRO_1 = "TEST/SEMAFORO_1"
TOPIC_MICRO_2 = "TEST/SEMAFORO_2"

CANTIDAD_DE_SEMAFORO = 1
CONT = 0
LISTO1 = None 
LISTO2 = None
TIEMPO = None

# Variable global para almacenar mensajes recibidos
mqtt_messages = {
    "CAMARA_1": None,
    "CAMARA_2": None,
    "ESTADO_SEM_1": None,
    "ESTADO_SEM_2": None,
    "TIEMPO_ESTADO": None,
    "EMERGENCIA": None,
    "CONFIRMACION_1": None,
    "CONFIRMACION_2": None
}

# Callback de conexión MQTT
def on_connect(client, userdata, flags, rc):
    print("Conectado al broker MQTT con código de resultado:", rc)
    # Suscribirse a todos los tópicos
    client.subscribe([
        (TOPIC_CAMARA_1, 0),
        (TOPIC_CAMARA_2, 0),
        (TOPIC_ESTADO_SEM_1, 0),
        (TOPIC_ESTADO_SEM_2, 0),
        (TOPIC_TIEMPO_ESTADO, 0),
        (TOPIC_CONFIRMACION_1, 0),
        (TOPIC_CONFIRMACION_2, 0)
    ])

# Callback de mensaje recibido
def on_message(client, userdata, msg):
    global mqtt_messages, CONT, LISTO1, LISTO2, TIEMPO
    topic = msg.topic
    payload = msg.payload.decode()

    # Actualizar los mensajes en la variable global
    if topic == TOPIC_CAMARA_1:
        mqtt_messages["CAMARA_1"] = payload
        print(f"Mensaje recibido en {TOPIC_CAMARA_1}: {mqtt_messages["CAMARA_1"]}")
        socketio.emit('cara_1', {'message1': mqtt_messages["CAMARA_1"]})
    
    elif topic == TOPIC_CAMARA_2:
        mqtt_messages["CAMARA_2"] = payload
        print(f"Mensaje recibido en {TOPIC_CAMARA_2}: {mqtt_messages["CAMARA_2"]}")
        socketio.emit('cara_2', {'message2': mqtt_messages["CAMARA_2"]})
        
    elif topic == TOPIC_ESTADO_SEM_1:
        mqtt_messages["ESTADO_SEM_1"] = payload
        print(f"Mensaje recibido en {TOPIC_ESTADO_SEM_1}: {mqtt_messages["ESTADO_SEM_1"]}")
        socketio.emit('status_semaforo_1', {'estado1': mqtt_messages["ESTADO_SEM_1"]})
    
    elif topic == TOPIC_ESTADO_SEM_2:
        mqtt_messages["ESTADO_SEM_2s"] = payload
        print(f"Mensaje recibido en {TOPIC_ESTADO_SEM_2}: {mqtt_messages["ESTADO_SEM_2s"]}")
        socketio.emit('status_semaforo_2', {'estado2': mqtt_messages["ESTADO_SEM_2s"]})
    
    elif topic == TOPIC_TIEMPO_ESTADO:
        mqtt_messages["TIEMPO_ESTADO"] = payload
        
    elif topic == TOPIC_CONFIRMACION_1:
        mqtt_messages["CONFIRMACION_1"] = payload
        LISTO1 = mqtt_messages["CONFIRMACION_1"] = payload
    
    elif topic == TOPIC_CONFIRMACION_2:
        mqtt_messages["CONFIRMACION_2"] = payload
        LISTO2 = mqtt_messages["CONFIRMACION_2"] = payload


    if  LISTO1 is not None and LISTO2 is not None:
        
        if CONT == 0:
            
            mqtt_client.publish(TOPIC_MICRO_1 , "VERDE")
            mqtt_client.publish(TOPIC_MICRO_2 , "ROJO")
            
            if int(mqtt_messages["CAMARA_1"]) >= 0 and int(mqtt_messages["CAMARA_1"]) < 11:
                TIEMPO = 20
            elif int(mqtt_messages["CAMARA_1"]) >= 11 and int(mqtt_messages["CAMARA_1"]) < 21:
                TIEMPO = 50
            elif int(mqtt_messages["CAMARA_1"]) >= 21 and int(mqtt_messages["CAMARA_1"]) < 31:
                TIEMPO = 60
            
            LISTO1 = None
            LISTO2 = None
            mqtt_client.publish(TOPIC_TIEMPO_ESTADO , TIEMPO)
            
        elif CONT == 1:
            mqtt_client.publish(TOPIC_MICRO_1 , str("ROJO"))
            mqtt_client.publish(TOPIC_MICRO_2 , str("VERDE"))
            
            if int(mqtt_messages["CAMARA_2"]) >= 0 and int(mqtt_messages["CAMARA_2"]) < 11:
                TIEMPO = 20
            elif int(mqtt_messages["CAMARA_2"]) >= 11 and int(mqtt_messages["CAMARA_2"]) < 21:
                TIEMPO = 50
            elif int(mqtt_messages["CAMARA_2"]) >= 21 and int(mqtt_messages["CAMARA_2"]) < 31:
                TIEMPO = 60
            LISTO1 = None
            LISTO2 = None
            mqtt_client.publish(TOPIC_TIEMPO_ESTADO , TIEMPO)
            
        if CONT >= CANTIDAD_DE_SEMAFORO:
            CONT = 0
        else: 
           CONT = CONT + 1

@socketio.on('change_mode_auto')
def handle_change_mode_auto(data):
    mode_value = data.get('mode', 0)
    mqtt_client.publish(TOPIC_PREDETERMINADO, str(mode_value))
    print(f"Modo automático/predeterminado cambiado: {'Automático' if mode_value == True else 'Predeterminado'}")

@socketio.on('change_mode_emergency')
def handle_change_mode_emergency(data):
    mode_value = data.get('mode', 0)
    mqtt_client.publish(TOPIC_EMERGENCIA, str(mode_value))
    print(f"Modo emergencia cambiado: {'ON' if mode_value == True else 'OFF'}")


@socketio.on('set_cycle_time')
def handle_set_timer(time):
    cycleTime = time.get('time', 0)
    mqtt_client.publish(TOPIC_TIEMPO_PREDETERMINADO, str(cycleTime))
    print(f"SET TIEMPO PREDETERMINADO: {cycleTime} SEGUNDOS")
    


# Configuración del cliente MQTT
mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# Conectar al broker MQTT (asegúrate de que la dirección y puerto son correctos)
mqtt_client.connect("localhost", 1883, 60)

# Ejecutar MQTT en un hilo separado
def mqtt_thread():
    mqtt_client.loop_forever()

threading.Thread(target=mqtt_thread).start()

# Rutas Flask
@app.route("/")
def index():
    return render_template("index.html")

@app.route("/backscreen")
def backscreen():
    return render_template("index_backscreen.html")

if __name__ == "__main__":
    app.run(debug=True)

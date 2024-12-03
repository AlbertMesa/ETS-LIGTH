import cv2
from ultralytics import YOLO
import paho.mqtt.client as mqtt
import time

# Configuración MQTT
BROKER = "192.168.1.35"  # Cambia esto por la IP/hostname de tu broker
PORT = 1883  # Puerto del broker MQTT
TOPIC_VEHICULOS = "test/RBPI1"  # Tema para vehículos detectados
TOPIC_AMBULANCIA = "test/AMBULANCIA"  # Tema para detectar ambulancias
INTERVALO_ENVIO = 5  # Intervalo en segundos para enviar datos al broker

# Función para conectarse al broker MQTT
def conectar_mqtt():
    cliente = mqtt.Client()
    try:
        cliente.connect(BROKER, PORT, 60)
        print("Conectado al broker MQTT.")
        return cliente
    except Exception as e:
        print(f"Error al conectar con MQTT: {e}")
        return None

def detectar_vehiculos_en_camara(cliente_mqtt):
    # Carga el modelo preentrenado de YOLO
    modelo = YOLO("yolov8n.pt")  # Modelo ligero de YOLOv8

    # Captura de video desde la cámara
    cap = cv2.VideoCapture(0)  # Usa "1", "2", etc. si tienes múltiples cámaras
    if not cap.isOpened():
        print("Error al acceder a la cámara.")
        return

    ultimo_envio = time.time()

    while True:
        ret, frame = cap.read()
        if not ret:
            print("No se pudo leer el frame de la cámara.")
            break

        # Realiza la detección de objetos
        resultados = modelo(frame)
        print(modelo.names)

        # Procesa los resultados
        detecciones = resultados[0].boxes
        contador_vehiculos = 0
        ambulancia_detectada = False

        for caja in detecciones:
            # Extrae información de cada detección
            x1, y1, x2, y2 = map(int, caja.xyxy[0])  # Coordenadas de la caja
            clase = int(caja.cls[0])  # Clase de objeto detectado
            nombre_clase = modelo.names[clase]  # Nombre de la clase detectada

            # Filtrar por clases de vehículos y ambulancias
            if clase in [2, 3, 5, 7]:  # Clases de vehículos en COCO
                contador_vehiculos += 1
                # Dibuja las cajas en la imagen
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                cv2.putText(frame, "Vehiculo", (x1, y1 - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
            elif nombre_clase.lower() == "ambulance":  # Clase para ambulancia
                ambulancia_detectada = True
                cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 0, 255), 2)
                cv2.putText(frame, "Ambulancia", (x1, y1 - 10),
                            cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 255), 2)

        # Enviar datos al broker MQTT cada INTERVALO_ENVIO segundos
        tiempo_actual = time.time()
        if cliente_mqtt and tiempo_actual - ultimo_envio >= INTERVALO_ENVIO:
            try:
                cliente_mqtt.publish(TOPIC_VEHICULOS, str(contador_vehiculos))
                print(f"Enviado: {contador_vehiculos} vehículos al tema '{TOPIC_VEHICULOS}'.")

                if ambulancia_detectada:
                    cliente_mqtt.publish(TOPIC_AMBULANCIA, "1")
                    print("Enviado: Ambulancia detectada.")
                else:
                    cliente_mqtt.publish(TOPIC_AMBULANCIA, "0")

                ultimo_envio = tiempo_actual
            except Exception as e:
                print(f"Error al enviar mensaje MQTT: {e}")

        # Mostrar el frame procesado
        cv2.putText(frame, f"Vehiculos detectados: {contador_vehiculos}", (20, 40),
                    cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)

        if ambulancia_detectada:
            cv2.putText(frame, "Ambulancia detectada!", (20, 80),
                        cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

        cv2.imshow("Deteccion de Vehiculos - Camara", frame)

        # Salir con la tecla 'q'
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Libera recursos
    cap.release()
    cv2.destroyAllWindows()

# Conectar al broker MQTT
cliente_mqtt = conectar_mqtt()

# Ejecutar detección desde la cámara
if cliente_mqtt:
    detectar_vehiculos_en_camara(cliente_mqtt)
    cliente_mqtt.disconnect()


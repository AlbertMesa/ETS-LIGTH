#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h> //libreria que nos permite el manejo dde kis oerifericos GPIO
#include "esp_wifi.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_system.h"
#include <freertos/timers.h> //Libreria para utilizar el timer 

#define WIFI_SSID               "Campus_ITLA"          // Nombre de la red WIFI
#define WIFI_PASSWORD           ""      //  contraseña
#define MQTT_BROKER_HOST        "mqtt://192.168.127.180"      // Dirección del broker (IP)
#define MQTT_BROKER_HOSTNAME    "192.168.127.180"
#define MQTT_BROKER_PORT         1883             // Puerto del broker MQTT (sin TLS)
#define MAX_TOPIC_LEN 50

//CONECIONES WIFI Y BROKET MQTT
/*#define WIFI_SSID               "FLIA mesa Dipres"          // Cambia por tu red Wi-Fi
#define WIFI_PASSWORD           "MesaD09171301"      // Cambia por la contraseña
#define MQTT_BROKER_HOST        "mqtt://192.168.1.35"      // Dirección del broker (IP o dominio)
#define MQTT_BROKER_HOSTNAME    "192.168.1.35"
#define MQTT_BROKER_PORT        1883             // Puerto del broker MQTT (sin TLS)*/

//TOPICOS DEL BROKER MQTT UTILIZADOS EN EL ESP
#define TEMA_PRINCIAL_ESP               "TEST/SEMAFORO1"
#define TEMA_CONFIRMACION               "TEST/SEMAFORO1/Conf"
#define TEMA_ESTADO_SEM_1               "TEST/ESTADO_1"
#define TEMA_TIEMPO                     "TEST/TIEMPO"
#define TOPIC_TIEMPO_PREDETERMINADO     "TEST/TIEMPO/PREDETERMINADO"
#define TEMA_MODO                       "TEST/MODO"
#define TEMA_EMERGENCIA                 "TEST/EMERGENCIA"
#define TEMA_ESP32_CONET                "TEST/ESP_CONECT"


//DEFINICION DE LOS ESTADOS A UTILIZAR
#define ESTADO_INIT         1
#define ESTADO_VERDE        2
#define ESTADO_AMARILLO     3
#define ESTADO_ROJO         4
#define ESTADO_EMERGENCIA   5
#define ESTADO_MODO         6
#define ESTADO_ERROR        7
#define ESTADO_ESPERA       8


#define PIN_LUZ_VERDE       33
#define PIN_LUZ_AMARILLO    25  
#define PIN_LUZ_ROJO        26

/*
19
5
2*/

//DECLARACION DE LAS FUNCIONES
int Func_ESTADO_INIT();
int Func_ESTADO_VERDE();
int Func_ESTADO_AMARILLO();
int Func_ESTADO_ROJO();
int Func_ESTADO_EMERGENCIA();
int Func_ESTADO_MODO();
int Func_ESTADO_ERROR();
int Func_ESTADO_ESPERA();

//PROTOTIPO DE FUNCIONES 
esp_err_t SET_TIMER(void);

//ESTADOS CON CUESTION DE TIEMPOS POSICION
volatile int ESTADO_ACTUAL = ESTADO_INIT;
volatile int ESTADO_SIGUIENTE = ESTADO_INIT;
volatile int ESTADO_ANTERIOR = ESTADO_INIT;


//ESTRUCTURA ENCARGADA DE LAS ENTRADAS, SALIDAS Y ERRORES
volatile struct INPUTS{
    unsigned int INPUT_EMER:          1;
    unsigned int INPUT_MODO:          1;
    unsigned int INPUT_TURNO:         1;
    unsigned int INPUT_TIEMPO_PREDE:  1;
    unsigned int BT_RUN_VERDE:        1;
    unsigned int BT_RUN_AMARI:        1;
    unsigned int BT_RUN_ROJO:         1;
    unsigned int INPUT_RUN_RBPI:      1;
}inputs;

volatile struct OUTPUTS{  
    unsigned int LV: 1;
    unsigned int LA: 1;
    unsigned int LR: 1;
}outputs;
/*volatile struct ERROR {
    unsigned int ERROR_LUZ_VERDE:    1;
    unsigned int ERROR_LUZ_AMARI:    1;
    unsigned int ERROR_LUZ_ROJA:     1;
    unsigned int ERROR_RASPBERRY:    1;

} errores;*/

static const char *TAG = "MQTT_EXAMPLE";

TimerHandle_t xTimers;
int timerID = 1;
int INTERVALO = 200;

 esp_mqtt_client_handle_t mqtt_client = NULL;


///////////
int CONTADOR_EMERGENCIA = 0;
int CONT_GENERAL = 0;
int state = 0;
int state_emergencia = 0;
int CONTADOR_MODO = false;
int TIEMPO = 0;
int CONT_AUTOMATICO = 0;
int TIEMPO_PREDETERMIANDO = 40;
int CONTADOR_POR_CICLO = 0;
int state_luz_prede = 0;
unsigned int LV_A = 0;
unsigned int LA_A = 0;
unsigned int LR_A = 0;
unsigned int HAB_CONT_AUTO = 0;
unsigned int HAB_USO_TIEMPO = 0;




// Función de callback para manejar los eventos MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;



    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "Conectado al broker MQTT");
            mqtt_client = client;

            esp_mqtt_client_subscribe(client, TEMA_PRINCIAL_ESP, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/SEMAFORO1");

            esp_mqtt_client_subscribe(client, TEMA_CONFIRMACION, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/SEMAFORO1/Conf");

            esp_mqtt_client_subscribe(client, TEMA_TIEMPO, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/tiempo");

            esp_mqtt_client_subscribe(client, TOPIC_TIEMPO_PREDETERMINADO, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/tiempo");

            esp_mqtt_client_subscribe(client, TEMA_MODO, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/MODO");

            esp_mqtt_client_subscribe(client, TEMA_EMERGENCIA, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/EMERGENCIA");

            esp_mqtt_client_subscribe(client, TEMA_ESP32_CONET, 0);
            ESP_LOGI(TAG, "Suscrito al tópico: TEST/ESP32_CONECTA2");

            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "Desconectado del broker MQTT");
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Mensaje recibido:");

            char topic_nuevo[MAX_TOPIC_LEN + 1]; // +1 para el carácter nulo
            strncpy(topic_nuevo, event->topic, event->topic_len);
            topic_nuevo[event->topic_len] = '\0'; // Agregar el carácter nulo


                if (!strcmp(topic_nuevo, TEMA_PRINCIAL_ESP))
                {
                    ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len, event->data);

                        if (!strncmp(event->data, "VERDE\n", event->data_len))
                        {
                            inputs.INPUT_TURNO = true;
                        }
                        else if (!strncmp(event->data, "ROJO\n", event->data_len))
                        {
                            inputs.INPUT_TURNO = false;
                        }

                }
                else if (!strcmp(topic_nuevo, TEMA_TIEMPO))
                {
                    char mensaje[event->data_len + 1]; // +1 para el carácter nulo
                    strncpy(mensaje, event->data, event->data_len); // Copiar el mensaje recibido
                    mensaje[event->data_len] = '\0'; // Asegurar la terminación de la cadena

                        ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len, event->data);
                        TIEMPO = atoi(mensaje); // Convierte la cadena a entero
                    
                        if (TIEMPO != 0 || mensaje[0] == '0') { // atoi retorna 0 si falla, manejar ese caso
                            printf("\nNúmero recibido en TEMA_TIEMPO: %d\n", TIEMPO);
                          
                        }
                    HAB_USO_TIEMPO = true;
                    
                }
                else if (!strcmp(topic_nuevo, TOPIC_TIEMPO_PREDETERMINADO))
                {
                
                     char mensaje[event->data_len + 1]; // +1 para el carácter nulo
                    strncpy(mensaje, event->data, event->data_len); // Copiar el mensaje recibido
                    mensaje[event->data_len] = '\0'; // Asegurar la terminación de la cadena

                        ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len, event->data);
                        TIEMPO_PREDETERMIANDO = atoi(mensaje); // Convierte la cadena a entero
                    
                        if (TIEMPO_PREDETERMIANDO != 0 || mensaje[0] == '0') { // atoi retorna 0 si falla, manejar ese caso
                            printf("\nNúmero recibido en TEMA_TIEMPO: %d\n", TIEMPO_PREDETERMIANDO);
                        }
                }

                else if (!strcmp(topic_nuevo, TEMA_EMERGENCIA))
                {
                        ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len-1, event->data);

                        if (!strncmp(event->data, "True\n", event->data_len))
                        {
                            inputs.INPUT_EMER = true;
                        }
                        else if (!strncmp(event->data, "False\n", event->data_len))
                        {
                            inputs.INPUT_EMER = false;
                        }
                }
                else if (!strcmp(topic_nuevo, TEMA_MODO))
                {
                         ESP_LOGI(TAG, "Mensaje: %.*s", event->data_len-1, event->data);

                        if (!strncmp(event->data, "True\n", event->data_len))
                        {
                            inputs.INPUT_MODO = true;
                        }
                        else if (!strncmp(event->data, "False\n", event->data_len))
                        {
                            inputs.INPUT_MODO = false;
                        }
                }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "Error en MQTT");
            break;
        default:
            break;
    }
}



// Función para publicar en MQTT
void publicar_mensaje(const char *topic, const char *mensaje) {
    {
    if (mqtt_client != NULL) {
        int msg_id = esp_mqtt_client_publish(mqtt_client, topic, mensaje, 0, 1, 0);
        if (msg_id >= 0) {
            ESP_LOGI(TAG, "Mensaje enviado al tópico '%s': %s", topic, mensaje);
        } else {
            ESP_LOGE(TAG, "Error al enviar mensaje al tópico '%s'", topic);
        }
    } else {
        ESP_LOGE(TAG, "Cliente MQTT no inicializado");
    }
}
}


// Función de callback para manejar Las interrupciones
void vTimerCallback( TimerHandle_t pxTimer){
    
    //state = !state;

    //ASIGNANDO VALORES DE SALIDAS
    if (outputs.LV == true)
    {
        gpio_set_level(PIN_LUZ_VERDE , 1);
    }else
    {
        gpio_set_level(PIN_LUZ_VERDE , 0);
    }

    if (outputs.LA == true)
    {
        gpio_set_level(PIN_LUZ_AMARILLO , 1);
    }else
    {
        gpio_set_level(PIN_LUZ_AMARILLO , 0);
    }

    if (outputs.LR == true)
    {
        gpio_set_level(PIN_LUZ_ROJO , 1);
    }else
    {
        gpio_set_level(PIN_LUZ_ROJO , 0);
    }
    
    
    if (CONTADOR_EMERGENCIA == false && CONTADOR_MODO == false )
    {
        CONT_GENERAL = 0;
    }

    //gpio_set_level(2,state);
    if (CONTADOR_EMERGENCIA == true  || CONTADOR_MODO == true)
    {
        CONT_GENERAL++;
    }


    if (HAB_CONT_AUTO == true)
    {
        CONT_AUTOMATICO++;
    }
    else
    {
        CONT_AUTOMATICO = 0;
    }

    
}


// Configurar Wi-Fi
void WIFI_INIT(void) {
    ESP_LOGI(TAG, "Inicializando Wi-Fi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Conectando a Wi-Fi...");
    esp_wifi_connect();
}


void mqtt_app_start(void) {
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri =       MQTT_BROKER_HOST,
        .broker.address.hostname =  MQTT_BROKER_HOSTNAME,  // Cambia por la IP o dominio de tu broker
        .broker.address.port =      MQTT_BROKER_PORT,             // Puerto sin TLS
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "No se pudo inicializar el cliente MQTT");
        return;
    }

    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(client);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Cliente MQTT iniciado con éxito");
        publicar_mensaje(TEMA_ESP32_CONET, "ESP CONECTADA");
    } else {
        ESP_LOGE(TAG, "Error al iniciar el cliente MQTT: %s", esp_err_to_name(err));
    }
}



void app_main(void) {
    // Inicializar almacenamiento no volátil (NVS)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // Inicializar Wi-Fi y MQTT
    WIFI_INIT();
    mqtt_app_start();

    while (1)
    {
        if (ESTADO_SIGUIENTE == ESTADO_INIT) {
            ESTADO_SIGUIENTE = Func_ESTADO_INIT();
        }
        if (ESTADO_SIGUIENTE == ESTADO_VERDE) {
            ESTADO_SIGUIENTE = Func_ESTADO_VERDE();
        }
        if (ESTADO_SIGUIENTE == ESTADO_AMARILLO) {
            ESTADO_SIGUIENTE = Func_ESTADO_AMARILLO();
        }
        if (ESTADO_SIGUIENTE == ESTADO_ROJO) {
            ESTADO_SIGUIENTE = Func_ESTADO_ROJO();
        }
        if (ESTADO_SIGUIENTE == ESTADO_EMERGENCIA) {
            ESTADO_SIGUIENTE = Func_ESTADO_EMERGENCIA();
        }
        if (ESTADO_SIGUIENTE == ESTADO_MODO) {
            ESTADO_SIGUIENTE = Func_ESTADO_MODO();
        }
        if (ESTADO_SIGUIENTE == ESTADO_ERROR) {
            ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
        }
        if (ESTADO_SIGUIENTE == ESTADO_ESPERA) {
            ESTADO_SIGUIENTE = Func_ESTADO_ERROR();
        }

}
        
    
    
}

int Func_ESTADO_INIT(){

        printf("\nFUNCION INIT\n");
    gpio_config_t IO_CONFIG;

    //CONFIGURANDO LAS ENTRADAS
  /*  IO_CONFIG.mode = GPIO_MODE_INPUT;
    IO_CONFIG.pin_bit_mask = (1 << 13) | (1 << 12) | (1 << 14) | (1 << 27) | (1 << 26);
    IO_CONFIG.pull_down_en = GPIO_PULLDOWN_ENABLE;
    IO_CONFIG.pull_up_en = GPIO_PULLUP_DISABLE;
    IO_CONFIG.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&IO_CONFIG);*/

    //CONFIGURANDO LAS SALIDAS
    IO_CONFIG.mode = GPIO_MODE_OUTPUT;
    IO_CONFIG.pin_bit_mask = (1ULL << PIN_LUZ_VERDE) | (1 << PIN_LUZ_AMARILLO) | (1 << PIN_LUZ_ROJO);
    IO_CONFIG.pull_down_en = GPIO_PULLDOWN_DISABLE;
    IO_CONFIG.pull_up_en = GPIO_PULLUP_DISABLE;
    IO_CONFIG.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&IO_CONFIG);

    CONTADOR_EMERGENCIA = false;
    CONTADOR_POR_CICLO = (60*5);

    //INICIALIZANDO TIMER
    printf("Inicializando configuracion del timer...");

    xTimers = xTimerCreate("Timer", 
                          (pdMS_TO_TICKS(INTERVALO)),
                          pdTRUE, 
                          ( void * )timerID, 
                          vTimerCallback
);
        if (xTimers == NULL)
        {
            printf("El timer no fue creado");
        }else 
        {
            if (xTimerStart(xTimers, 0) != pdPASS)
            {
                printf("El timer podria no ser seteado en el estado activo");
            }
            
        }

    if (inputs.INPUT_MODO == 1)
    {
        return ESTADO_MODO;
    }
    if (inputs.INPUT_EMER == 1)
    {
        return ESTADO_EMERGENCIA;
    }

    return ESTADO_ESPERA;
}

int Func_ESTADO_VERDE(){

     printf("\nFUNCION VERDE AUTOMATICO\n");
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_VERDE;

    outputs.LV = true;
    outputs.LA = false;
    outputs.LR = false;

    HAB_CONT_AUTO = true;
    publicar_mensaje(TEMA_ESTADO_SEM_1, "VERDE");

    LV_A = (TIEMPO - 3) * 5;
    LA_A = (LV_A) + (3*5);
    

    CONTADOR_EMERGENCIA = false;


    while (true)
    {
        if (inputs.INPUT_EMER == true)
        {
            return ESTADO_EMERGENCIA;
        }
        if (inputs.INPUT_MODO == true)
        {
            return ESTADO_MODO;
        }
        

        if (inputs.INPUT_TURNO == true)//comienza en verde
        {
            if (CONT_AUTOMATICO == LA_A)
            {
                
                return ESTADO_AMARILLO;
            }
            
        }
        
    }

}

int Func_ESTADO_AMARILLO(){

    printf("\nFUNCION AMARILLO AUTOMATICO\n");
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_AMARILLO;

    outputs.LV = false;
    outputs.LA = true;
    outputs.LR = false;

    CONTADOR_EMERGENCIA = false;

    publicar_mensaje(TEMA_ESTADO_SEM_1, "AMARILLO");

    while (true)
    {
        if (inputs.INPUT_EMER == true)
        {
            return ESTADO_EMERGENCIA;
        }
        if (inputs.INPUT_MODO == true)
        {
            return ESTADO_MODO;
        }
       
       if (inputs.INPUT_TURNO == true)//comienza en verde
        {
            if (CONT_AUTOMATICO == LA_A)
            {
                publicar_mensaje(TEMA_CONFIRMACION, "1");
                CONT_AUTOMATICO = 0;
                HAB_CONT_AUTO = false;
                return ESTADO_ESPERA;
                
            }
            
        }
    }
}

int Func_ESTADO_ROJO(){
printf("\nFUNCION ROJO AUTOMATICO\n");

    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ROJO;

    outputs.LV = false;
    outputs.LA = false;
    outputs.LR = true;
    

    CONTADOR_EMERGENCIA = false;

    LR_A = TIEMPO * 5;

    publicar_mensaje(TEMA_ESTADO_SEM_1, "ROJO");

    while (true)
    {
        if (inputs.INPUT_EMER == true)
        {
            return ESTADO_EMERGENCIA;
        }
        if (inputs.INPUT_MODO == true)
        {
            return ESTADO_MODO;
        }

            if (CONT_AUTOMATICO == LR_A)
            {
                publicar_mensaje(TEMA_CONFIRMACION, "1");
                CONT_AUTOMATICO = 0;
                HAB_CONT_AUTO = false;
                return ESTADO_ESPERA;
            }
    }

}
//CASI TERMINADO, AFINAR DETALLES
int Func_ESTADO_EMERGENCIA(){

    printf("\nFUNCION EMERGENCIA\n");//prueba
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_EMERGENCIA;

    outputs.LV = false;
    outputs.LA = false;
    outputs.LR = false;

    CONTADOR_EMERGENCIA = true;

    while (true)
    {

        if (inputs.INPUT_EMER == false)
        {   
            outputs.LA = false;
            return ESTADO_ESPERA;
        }
        
        if (CONTADOR_EMERGENCIA == true)
        {
            if (CONT_GENERAL == 2)
            {   

                state_emergencia = !state_emergencia;
                gpio_set_level(PIN_LUZ_AMARILLO,state_emergencia);
                CONT_GENERAL = 0;
            }
            
        }
        
    }
}
//LA LOGICA ES QUE CUANDO SE INGRESE EL MODO PREDETERMINADO ( MANUAL ) DURE 40 SEGUNDOS EN VERDE, 5 EN AMARILLO
//Y LAS SUMA DE LOS DOS EN ROJO APROVECHANDO LAS INTERRUPCIONES PARA REALIZAR UN CONTADOR

int Func_ESTADO_MODO(){
    
    printf("\nFUNCION MODO\n");//prueba

    outputs.LV = false;
    outputs.LA = false;
    outputs.LR = false;

    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_MODO;

     int state_led = 0;
    CONTADOR_POR_CICLO = TIEMPO_PREDETERMIANDO;

    unsigned int LUZ_VERDE =    ((CONTADOR_POR_CICLO / 2)*5) - (3*5);
    unsigned int LUZ_AMARILLA= ((LUZ_VERDE) + (3*5));
    unsigned int LUZ_ROJA = (((CONTADOR_POR_CICLO / 2)*5) + LUZ_AMARILLA);

    CONTADOR_EMERGENCIA = false;
    CONTADOR_MODO = true;

    

    while (true)
    {
        
        if (inputs.INPUT_MODO == false)
        {
            return ESTADO_ESPERA;
        }
        if (inputs.INPUT_EMER == true)
        {
            CONT_GENERAL = 0;
            return ESTADO_EMERGENCIA;
        }            
  
            if (CONT_GENERAL == LUZ_ROJA)
            {
                CONTADOR_POR_CICLO = TIEMPO_PREDETERMIANDO;
                LUZ_VERDE =    ((CONTADOR_POR_CICLO / 2)*5) - (3*5);
                LUZ_AMARILLA= ((LUZ_VERDE) + (3*5));
                LUZ_ROJA = (((CONTADOR_POR_CICLO / 2)*5) + LUZ_AMARILLA);
                CONT_GENERAL = 0;
                state_luz_prede = 0;
                 
            }
        if (state_luz_prede == 0)
        {
            outputs.LR = true;
        }else if (state_luz_prede == 1)
        {
            outputs.LV = true;
        }else if (state_luz_prede == 2)
        {
            outputs.LR = true;
        }

        if (CONT_GENERAL <= 0)
        {
            printf("\nESTADO DEL SEMAFORO PREDETERMINADO EN VERDE\n");
            
            outputs.LV = true;
            outputs.LA = false;
            outputs.LR = false;
            publicar_mensaje(TEMA_ESTADO_SEM_1, "VERDE");
            state_luz_prede = 2;
        }
         if (CONT_GENERAL == LUZ_VERDE )
        {   printf("\nESTADO DEL SEMAFORO PREDETERMINADO EN AMARILLO\n");
            
            outputs.LV = false;
            outputs.LA = true;
            outputs.LR = false;
            publicar_mensaje(TEMA_ESTADO_SEM_1, "AMARILLO");
            state_luz_prede = 1;
        }
        if (CONT_GENERAL == LUZ_AMARILLA )
        {
            printf("\nESTADO DEL SEMAFORO PREDETERMINADO EN ROJO\n");
            outputs.LV = false;
            outputs.LA = false;
            outputs.LR = true;
            publicar_mensaje(TEMA_ESTADO_SEM_1, "ROJO");
            state_luz_prede = 0;
        }
        
    }
    
}

int Func_ESTADO_ERROR(){
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ERROR;

    outputs.LV = false;
    outputs.LA = false;
    outputs.LR = false;

    CONTADOR_EMERGENCIA = false;

    while (true)
    {
        if (inputs.INPUT_EMER == true)
        {
            return ESTADO_EMERGENCIA;
        }
        if (inputs.INPUT_MODO == true)
        {
            return ESTADO_MODO;
        }

        return ESTADO_ESPERA;
    }
    
}

int Func_ESTADO_ESPERA(){

    printf("\nFUNCION ESPERA\n");
    ESTADO_ANTERIOR = ESTADO_ACTUAL;
    ESTADO_ACTUAL = ESTADO_ESPERA;


    while (1)
    {
        if (inputs.INPUT_EMER == true)
        {
            return ESTADO_EMERGENCIA;
        }else
        {
            if (inputs.INPUT_MODO == false)
            {   
                if (HAB_USO_TIEMPO == true)
                {
                   if (inputs.INPUT_TURNO == true)
                    {
                        return ESTADO_VERDE;
                    }
                    if (inputs.INPUT_TURNO == false)
                    {
                        return ESTADO_ROJO;
                    } 
                }
            }
           if (inputs.INPUT_MODO == true)
            {

                return ESTADO_MODO;
            }
        
        }   
    }
}
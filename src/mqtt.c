#include "mongoose.h"

char* mqtt_buffer;
data_in_service mqtt_in;

extern void mg_close_conn();

TimerHandle_t mqttReconnectTimer;
char s_address[100];
static const char *s_user_name = "", *s_password = "";
static struct mg_mqtt_topic_expression s_topic_expr = {NULL, 0};
struct mg_connection * mync = NULL;
struct mg_mgr mgr;
bool mqttFlag = false;

static void ev_handler(struct mg_connection *nc, int ev, void *p) {
    struct mg_mqtt_message *msg = (struct mg_mqtt_message *) p;
    mync = nc;
    (void) nc;

    if (ev != MG_EV_POLL) ESP_LOGU("MQTT", "USER HANDLER GOT EVENT %d", ev);
    switch (ev) {
        case MG_EV_CONNECT: {
            struct mg_send_mqtt_handshake_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.user_name = s_user_name;
            opts.password = s_password;
            opts.keep_alive = 60;

            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, vin, opts);
            break;
        }
        case MG_EV_SEND:{
            xTimerStart(mqttReconnectTimer, 0);
            break;
        }
        case MG_EV_RECV:{
            xTimerStop(mqttReconnectTimer, 0);
            break;
        }
        case MG_EV_MQTT_CONNACK:
            if (msg->connack_ret_code != MG_EV_MQTT_CONNACK_ACCEPTED) {
                ESP_LOGE("MQTT", "Got mqtt connection error: %d", msg->connack_ret_code);
                exit(1);
            }
            s_topic_expr.topic = vin;
            ESP_LOGU("MQTT", "Subscribing to '%s'", vin);
            mg_mqtt_subscribe(nc, &s_topic_expr, 1, 42);
            break;
        case MG_EV_MQTT_PUBACK:
            ESP_LOGI("MQTT", "Message publishing acknowledged (msg_id: %d)", msg->message_id);
            break;
        case MG_EV_MQTT_SUBACK:
            mg_mqtt_publish(nc, "UID/announce", 65, MG_MQTT_QOS(1), imprinting, strlen(imprinting));
            break;
        case MG_EV_MQTT_PUBLISH: {
            free(mqtt_buffer);
            mqtt_buffer = (char *)malloc(sizeof(char)*(msg->payload.len+1));
            memcpy(mqtt_buffer, msg->payload.p, (int) msg->payload.len);
            mqtt_buffer[msg->payload.len] = 0;
            mqtt_in.msg = mqtt_buffer;
            mqtt_in.type = 0;
            mqtt_in.len = (int) msg->payload.len;
            ESP_LOGU("MQTT", "%s:%d", mqtt_in.msg, mqtt_in.len);
            xQueueSend(provider_queue, (void *) &mqtt_in, portMAX_DELAY);
            break;
        }
        case MG_EV_CLOSE:{
            ESP_LOGI("MQTT", "Connection closed");
            mg_mgr_free(&mgr);
            mqttFlag = false;
            break;
        }
    }
}

void mqtt_send(data_in_service* arg){
    mg_mqtt_publish(mync, arg->serviceUserAddress, 65, MG_MQTT_QOS(1), arg->msg, arg->len);
}

void mqtt_reconnect(){
    xTimerStop(mqttReconnectTimer, 0);
    mg_close_conn(mync);
}

static void mqtt(){
    vTaskDelay(2500);
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(5000), pdFALSE, (void*)0, mqtt_reconnect);
    sprintf(s_address,"%s:%d",MQTT_HOST,MQTT_PORT);
    while(1){
        if(apFlag){
            mg_mgr_init(&mgr, NULL);
            if ((mync = mg_connect(&mgr, s_address, ev_handler)) == NULL) {
                fprintf(stderr, "mg_connect(%s) failed\n", s_address);
                vTaskDelay(5000);
                continue;
            } else mqttFlag = true;
        }
        for (;;) {
            if(apFlag && mync !=NULL && mqttFlag) mg_mgr_poll(&mgr, 1000);
            else {
                mync = NULL;
                break;
            }
            vTaskDelay(1000);
        }
        vTaskDelay(1000);
    }
}

void start_mqtt() {
    xTaskCreatePinnedToCore(mqtt, "mqtt", 1024*4, NULL, 1, NULL, 0);
    return;
}
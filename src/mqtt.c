#include "mongoose.h"
#include "global_var.h"

char* mqtt_buffer;
data_header mqtt_in;

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

    if (ev != MG_EV_POLL) printf("MQTT: USER HANDLER GOT EVENT %d\n", ev);
    switch (ev) {
        case MG_EV_CONNECT: {
            struct mg_send_mqtt_handshake_opts opts;
            memset(&opts, 0, sizeof(opts));
            opts.user_name = s_user_name;
            opts.password = s_password;
            opts.keep_alive = 60;

            mg_set_protocol_mqtt(nc);
            mg_send_mqtt_handshake_opt(nc, myname, opts);
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
                printf("MQTT: Got mqtt connection error: %d\n", msg->connack_ret_code);
                exit(1);
            }
            s_topic_expr.topic = myname;
            printf("MQTT: Subscribing to '%s'\n", myname);
            mg_mqtt_subscribe(nc, &s_topic_expr, 1, 42);
            break;
        case MG_EV_MQTT_PUBACK:
            printf("MQTT: Message publishing acknowledged (msg_id: %d)\n", msg->message_id);
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
            printf("MQTT: %s:%d\n", mqtt_in.msg, mqtt_in.len);
            if(!strncmp(msg->topic.p, myname, msg->topic.len)) xQueueSend(provider_queue, (void *) &mqtt_in, portMAX_DELAY);
            else xQueueSend(user_queue, (void *) &mqtt_in, portMAX_DELAY);

            break;
        }
        case MG_EV_CLOSE:{
            printf("MQTT: Connection closed\n");
            mg_mgr_free(&mgr);
            mqttFlag = false;
            break;
        }
    }
}

void mqtt_unsubscribe(char * topic){
    s_topic_expr.topic = topic;
    printf("MQTT: Unsubscribing to '%s'\n", topic);
    mg_mqtt_unsubscribe(mync, &s_topic_expr, 1, 42);
}

void mqtt_subscribe(char * topic){
    s_topic_expr.topic = topic;
    printf("MQTT: Subscribing to '%s'\n", topic);
    mg_mqtt_subscribe(mync, &s_topic_expr, 1, 42);
}

void mqtt_send(data_header* out){
    printf("MQTT %s -- %d ret = %s\n", out->msg, out->len, out->destination);
    mg_mqtt_publish(mync, out->destination, 65, MG_MQTT_QOS(1), out->msg, out->len);
}

void mqtt_reconnect(){
    xTimerStop(mqttReconnectTimer, 0);
    printf("mqtt socket: closing...\n");
    mg_close_conn(mync);
}

static void mqtt(){
    vTaskDelay(2500 / portTICK_PERIOD_MS);
    mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(15000), pdFALSE, (void*)0, mqtt_reconnect);
    sprintf(s_address,"%s:%d\n", MQTT_HOST,MQTT_PORT);
    while(1){
        if(apFlag){
            mg_mgr_init(&mgr, NULL);
            if ((mync = mg_connect(&mgr, s_address, ev_handler)) == NULL) {
                fprintf(stderr, "mg_connect(%s) failed\n", s_address);
                vTaskDelay(5000 / portTICK_PERIOD_MS);
                continue;
            } else mqttFlag = true;
        }
        for (;;) {
            if(apFlag && mync !=NULL && mqttFlag) mg_mgr_poll(&mgr, 1000);
            else {
                mync = NULL;
                break;
            }
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

void start_mqtt() {
    xTaskCreatePinnedToCore(mqtt, "mqtt", 1024*4, NULL, 1, NULL, 0);
    return;
}
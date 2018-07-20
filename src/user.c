#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "mongoose.h"

#include "UID_message.h"
#include "UID_dispatch.h"
#include "UID_capBAC.h"
#include "yajl/yajl_tree.h"
#include "UID_utils.h"
#include "UID_time.h"

#include "global_var.h"

int MY_parse_result(uint8_t *buffer, size_t size, UID_ClientChannelCtx *ctx, char *res, size_t rsize, int64_t id)
{
	BTC_Address sender;
	int error = 0;
	int64_t sID;

	int ret = UID_parseRespMsg(buffer, size, sender, sizeof(sender), &error, res, rsize, &sID);
	if ( ret ) return ret;
	if (error) return UID_MSG_RPC_ERROR | error;
	if (strcmp(sender, ctx->peerid)) return UID_MSG_INVALID_SENDER;
	if (sID != id) return UID_MSG_ID_MISMATCH;
	return 0;
}

char * service_user(data_header* out, int method)
{
	int ret = 0;

	UID_ClientChannelCtx ctx;
	if ( UID_MSG_OK != (ret = UID_createChannel(out->destination, &ctx)) ) {
		printf("UID_open_channel(%s) return %d\n", out->destination, ret);
		return NULL;
	}

	mqtt_subscribe(ctx.myid);

	vTaskDelay(500 / portTICK_PERIOD_MS);

	uint8_t buffer[1024];
	size_t size = sizeof(buffer);
	int64_t id;
	if ( UID_MSG_OK != (ret = UID_formatReqMsg(ctx.myid, method, out->msg, buffer, &size, &id)) ) {
		printf("UID_format_request() return %d\n", ret);
		mqtt_unsubscribe(ctx.myid);
		return NULL;
	}
	printf("UID_format_request %s -- %d ret = %d\n", buffer, size, ret);

	out->msg = buffer;
	if(out->type == 0){
		out->len = strlen(buffer);
		mqtt_send(out);
	}

	printf("---- WaitMsg\n");
	data_header msg;
	if(xQueueReceive(user_queue, &msg, (TickType_t )(15000/portTICK_PERIOD_MS)) == pdFALSE) //wait for 15s
		return NULL;

	// client
	char * result = NULL;
	result = (char*)malloc(sizeof(char)*1024);
	memset(result, 0, 1024);
	if ( UID_MSG_OK != (ret = MY_parse_result(msg.msg, msg.len, &ctx, result, 1024, id))) {
		printf("UID_parse_result() return %d\n", ret);
		mqtt_unsubscribe(ctx.myid);
		free(result);
		return NULL;
	}
	printf("UID_parse_result() %s\n", result);

	UID_closeChannel(&ctx);
	return result;
}
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

//da service_out, report_out;

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


void *service_user(void *arg)
{
	int ret;

	char machine[UID_NAME_LENGHT] = {0};
	int method ;
	char param[50] = {0};

	while(1)
	{
		/*UID_ClientChannelCtx ctx;
		if ( UID_MSG_OK != (ret = UID_createChannel(machine, &ctx)) ) {
			error(0, 0, "UID_open_channel(%s) return %d\n", machine, ret);
			continue;
		}

		uint8_t buffer[1024];
		size_t size = sizeof(buffer);
		int64_t id;
		if ( UID_MSG_OK != (ret = UID_formatReqMsg(ctx.myid, method, param, buffer, &size, &id)) ) {
			error(0, 0, "UID_format_request() return %d\n", ret);
			continue;
		}
		DBG_Print("UID_format_request %s -- %d ret = %d\n",buffer,size, ret);

		mqttUserSendMsg(machine, ctx.myid, buffer, size - 1);

		uint8_t *msg;
		DBG_Print("-->\n");
		
		
		mqttUserWaitMsg(&msg, &size);
		
		
		
		DBG_Print("<--\n");

		DBG_Print("--------->> %s\n", msg);

		// client
		char result[1024] = "";
		if ( UID_MSG_OK != (ret = MY_parse_result(msg, size, &ctx, result, sizeof(result), id))) {
			error(0, 0, "UID_parse_result() return %d\n", ret);
			continue;
		}
		DBG_Print("UID_parse_result() %s\n", result);

		free(msg);

		UID_closeChannel(&ctx);*/
		vTaskDelay(2000 / portTICK_PERIOD_MS);
	}

	return arg;
}
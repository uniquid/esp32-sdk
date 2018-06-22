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

data_in_service service_out, report_out;
uint8_t owner[]={0x00, 0x00, 0x00, 0x20, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
char * hacked = "###HACKED###";
char * log_out = NULL;

void error_logging(int64_t time, char * msg, int err){
	char header[100];
	sprintf(header,"{\"time\":%" PRId64 ",\"error\":%d,\"msg\":",time,err);
	char * log = (char*)malloc(sizeof(char)*(strlen(msg)+strlen(header)+3));
	sprintf(log,"%s%s},", header, msg);
	xSemaphoreTake(OBDaccess, portMAX_DELAY);
	ResetOBD = true;
	logger_save(log);
	xSemaphoreGive(OBDaccess);
	free(log);
}

// example of usedefined method
void user_32(char *param, char *result, size_t size)
{
	size_t logsize = check_if_hacked();
	size_t b64size = ((4 * logsize / 3) + 3) & ~3;
	if(logsize!=0){
		char *res = (char *)malloc(sizeof(char)*(size));
		memset(res, 0, size); 
		if(b64size>=size){
			xSemaphoreTake(OBDaccess, portMAX_DELAY);
			ResetOBD = true;
			logger_delete();
			xSemaphoreGive(OBDaccess);
			snprintf(res,size,"[{\"time\":%" PRId64 ",\"error\":%d,\"msg\":\"%s\"}]", UID_getTime(), UID_MSG_SMALL_BUFFER, "buffer log is too small");
			cs_base64_encode(res, strlen(res), result);
		} else{
			xSemaphoreTake(OBDaccess, portMAX_DELAY);
			ResetOBD = true;
			logger_load(res);
			xSemaphoreGive(OBDaccess);
			cs_base64_encode(res, strlen(res), result);
		}
		free(res);
	}
	else
		snprintf(result, size, "");
}

void user_33(char *param, char *result, size_t size)
{
	snprintf(result, size, "");
}

void user_34(char *param, char *result, size_t size)
{
	printf(">>> 34\n");
#ifdef EMULATOR_PRESENT
	if(!strcmp(param, "open"))
		snprintf(result, size, "%d", OBD_getPID(0x48)); //PID_ABSOLUTE_THROTTLE_POS_C
	else if(!strcmp(param, "close"))
		snprintf(result, size, "%d", OBD_getPID(0x49)); //PID_ACC_PEDAL_POS_D
	else if(!strcmp(param, "status"))
		snprintf(result, size, "%d", OBD_getPID(0x4A)); //PID_ACC_PEDAL_POS_E
#else
static int status;
	vTaskDelay(150);
	if(!strcmp(param, "open")) {
		snprintf(result, size, "%d", status = 1);
		printf("\n################\n## DOOR  OPEN ##\n################\n\n");
	}
	else if(!strcmp(param, "close")) {
		snprintf(result, size, "%d", status = 0);
		printf("\n################\n## DOOR CLOSE ##\n################\n\n");
	}
	else if(!strcmp(param, "status"))
		snprintf(result, size, "%d", status); //PID_ACC_PEDAL_POS_E
#endif
}

void user_35(char *param, char *result, size_t size)
{
#ifdef EMULATOR_PRESENT
	printf(">>> 35\n");
	if(!strcmp(param, "open"))
		snprintf(result, size, "%d", OBD_getPID(0x4B)); //PID_ACC_PEDAL_POS_F
	else if(!strcmp(param, "close"))
		snprintf(result, size, "%d", OBD_getPID(0x4C)); //PID_COMMANDED_THROTTLE_ACTUATOR
	else if(!strcmp(param, "status")){
		if(OBD_getPID(0x5A)==2)
			snprintf(result, size, "0");
		else
			snprintf(result, size, "1");
	}
#else
static int status;
	vTaskDelay(150);
	if(!strcmp(param, "open")) {
		snprintf(result, size, "%d", status = 1);
		printf("\n#################\n## TRUNK  OPEN ##\n#################\n\n");
	}
	else if(!strcmp(param, "close")) {
		snprintf(result, size, "%d", status = 0);
		printf("\n#################\n## TRUNK CLOSE ##\n#################\n\n");
	}
	else if(!strcmp(param, "status"))
		snprintf(result, size, "%d", status); //PID_ACC_PEDAL_POS_E
#endif
}

void user_36(char *param, char *result, size_t size)
{
#ifdef EMULATOR_PRESENT
	printf(">>> 36\n");
	if(!strcmp(param, "start"))
		snprintf(result, size, "%d", OBD_getPID(0x0B)); //PID_INTAKE_MAP
	else if(!strcmp(param, "stop"))
		snprintf(result, size, "%d", OBD_getPID(0x0F)); //PID_INTAKE_TEMP
	else if(!strcmp(param, "status"))
		snprintf(result, size, "%d", OBD_getPID(0x4D)); //PID_TIME_WITH_MIL
#else
static int status;
	vTaskDelay(150);
	if(!strcmp(param, "start")) {
		snprintf(result, size, "%d", status = 1);
		printf("\n##################\n## ENGINE START ##\n##################\n\n");
	}
	else if(!strcmp(param, "stop")) {
		snprintf(result, size, "%d", status = 0);
		printf("\n##################\n## ENGINE  STOP ##\n##################\n\n");
	}
	else if(!strcmp(param, "status"))
		snprintf(result, size, "%d", status); //PID_ACC_PEDAL_POS_E
#endif
}

int perform_request(uint8_t *buffer, size_t size, uint8_t *response, size_t *rsize, UID_ServerChannelCtx *channel_ctx, int * err)
{
    int ret, method, error;
	int64_t sID;
	BTC_Address sender;
	char params[1024] = {0}, result[1024] = {0};

	// parse the request
	ret = UID_parseReqMsg(buffer, size, sender, sizeof(sender), &method, params, sizeof(params), &sID);
	if (ret) return ret;
	if (strcmp(sender,channel_ctx->contract.serviceUserAddress)) return UID_MSG_INVALID_SENDER;

	// check the contract for permission
    if(UID_checkPermission(method, channel_ctx->contract.profile)) {
		if (UID_RPC_RESERVED > method) {
			// Uniquid method. call UID_performRequest
		    error = UID_performRequest(method, params, result, sizeof(result));
		}
		else {
			// user method.
			switch(method) {
				case 32:  //LOG
					user_32(params, result, sizeof(result));
					error = 0;
					break;
				case 33:  //XXX
					user_33(params, result, sizeof(result));
					error = 0;
					break;
				case 34:  //DOOR open/close/status
					user_34(params, result, sizeof(result));
					error = 0;
					break;
				case 35: //TRUNK open/close/status
					user_35(params, result, sizeof(result));
					error = 0;
					break;
				case 36: //XXX
					user_36(params, result, sizeof(result));
					error = 0;
					break;
				default:
					error = UID_DISPATCH_NOTEXISTENT;
					break;
			}
		}
    }
    else {
		// no permission for the method
		error = UID_DISPATCH_NOPERMISSION;
    }

	if(error!=UID_MSG_OK)
		*err = error;

	printf(">>>>>>>>> result %s \n", result);

	// format the response message
	ret = UID_formatRespMsg(channel_ctx->contract.serviceProviderAddress, result, error, sID, response, rsize);
	//printf("### format_resp ret %d\n", ret);
	if (ret) return ret;
    return UID_MSG_OK;
}

/**
+ * Check the message for capability
+ */
int decodeCapability(uint8_t *msg)
{
	UID_UniquidCapability cap = {0};
	yajl_val node, v;
	int ret = 0;

	const char * assigner[] = { "assigner", (const char *) 0 };
	const char * resourceID[] = { "resourceID", (const char *) 0 };
	const char * assignee[] = { "assignee", (const char *) 0 };
	const char * rights[] = { "rights", (const char *) 0 };
	const char * since[] = { "since", (const char *) 0 };
	const char * until[] = { "until", (const char *) 0 };
	const char * assignerSignature[] = { "assignerSignature", (const char *) 0 };

    // parse message
	node = yajl_tree_parse((char *)msg, NULL, 0);
    if (node == NULL) return 0; // parse error. not a capability

    v = yajl_tree_get(node, assigner, yajl_t_string);
    if (v == NULL) goto clean_return;
    if (sizeof(cap.assigner) <= (size_t)snprintf(cap.assigner, sizeof(cap.assigner), "%s", YAJL_GET_STRING(v)))
		goto clean_return;

    v = yajl_tree_get(node, resourceID, yajl_t_string);
    if (v == NULL) goto clean_return;
    if (sizeof(cap.resourceID) <= (size_t)snprintf(cap.resourceID, sizeof(cap.resourceID), "%s", YAJL_GET_STRING(v)))
		goto clean_return;

    v = yajl_tree_get(node, assignee, yajl_t_string);
    if (v == NULL) goto clean_return;
    if (sizeof(cap.assignee) <= (size_t)snprintf(cap.assignee, sizeof(cap.assignee), "%s", YAJL_GET_STRING(v)))
		goto clean_return;

    v = yajl_tree_get(node, rights, yajl_t_string);
    if (v == NULL) goto clean_return;
	if (sizeof(cap.rights) != fromhex(YAJL_GET_STRING(v), (uint8_t *)&(cap.rights), sizeof(cap.rights)))
		goto clean_return;

    v = yajl_tree_get(node, since, yajl_t_number);
    if (v == NULL) goto clean_return;
	cap.since = YAJL_GET_INTEGER(v);

    v = yajl_tree_get(node, until, yajl_t_number);
    if (v == NULL) goto clean_return;
	cap.until = YAJL_GET_INTEGER(v);

    v = yajl_tree_get(node, assignerSignature, yajl_t_string);
    if (v == NULL) goto clean_return;
    if (sizeof(cap.assignerSignature) <= (size_t)snprintf(cap.assignerSignature, sizeof(cap.assignerSignature), "%s", YAJL_GET_STRING(v)))
		goto clean_return;

	// parsing OK. Will return 1
    ret = 1;

	// receive the capability
	int recv = UID_receiveProviderCapability(&cap);
	if (recv != UID_CAPBAC_OK) {
		ESP_LOGU("PROVIDER","ERROR receiving capability: UID_receiveProviderCapability() returns %d", recv);
	}
	else {
		ESP_LOGU("PROVIDER", "Valid capability received!");
	}

clean_return:
    if (NULL != node) yajl_tree_free(node);
    return ret;
}

uint8_t rbuffer[2048];	

/**
 * thread implementing a Service Provider
 */
void service_provider(data_in_service* arg)
{
	int ret, error = 0;

	// create the contest for the communication (contract, identities of the peers, etc)
	UID_ServerChannelCtx sctx;

	uint8_t sbuffer[1024] = {0};
	size_t ssize = sizeof(sbuffer);

	ESP_LOGU("PROVIDER", "pre-accept %s - %d - %d", arg->msg, arg->len, strlen(arg->msg));
	ret = UID_accept_channel((uint8_t*)(arg->msg), strlen(arg->msg)+1, &sctx, sbuffer, &ssize);
	if ( UID_MSG_OK != ret) {
		if(!decodeCapability((uint8_t*)arg->msg)){
+			ESP_LOGU("PROVIDER", "error accept %d", ret);
			error_logging(UID_getTime(), arg->msg, ret);
		}
		return;
	}

	//uint8_t rbuffer[2048] = {0};
	size_t rsize = sizeof(rbuffer);
	memset(rbuffer,0,rsize);
	// perform the request
	if ( UID_MSG_OK != (ret = perform_request(sbuffer, ssize, rbuffer, &rsize, &sctx, &error))) {
		ESP_LOGU("PROVIDER", "error perform %d", ret);
		error_logging(UID_getTime(), arg->msg, ret);
		return;
	}

	if(error!=UID_MSG_OK)
		error_logging(UID_getTime(), arg->msg, error);

	ESP_LOGU("PROVIDER", "it's ok!");
	service_out.msg = rbuffer;
	service_out.type = arg->type;
	if(service_out.type == 0){
		service_out.len = rsize-1;
		sprintf(service_out.serviceUserAddress, "%s", sctx.contract.serviceUserAddress);
		mqtt_send(&service_out);
	} else if(service_out.type == 1){
		if (UID_checkPermission(32, sctx.contract.profile)){
			if(check_if_hacked()!=0){
				report_out.msg = hacked;
				report_out.type = arg->type;
				report_out.len = strlen(hacked);
				sprintf(report_out.serviceUserAddress, "7"); //LOG BLE
				ble_send(&report_out);
			}
		}

		service_out.len = rsize-1;
		sprintf(service_out.serviceUserAddress, "2"); //TX BLE
		ble_send(&service_out);
	}
	
	UID_closeServerChannel(&sctx);
}

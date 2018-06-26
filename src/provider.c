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

data_in_service service_out, report_out;

// example of usedefined method
void user_32(char *param, char *result, size_t size)
{
	snprintf(result, size, param);
}

void user_33(char *param, char *result, size_t size)
{
	snprintf(result, size, param);
}

void user_34(char *param, char *result, size_t size)
{
	snprintf(result, size, param);
}

void user_35(char *param, char *result, size_t size)
{
	snprintf(result, size, param);
}

void user_36(char *param, char *result, size_t size)
{
	snprintf(result, size, param);
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
				case 32:
					user_32(params, result, sizeof(result));
					error = 0;
					break;
				case 33:
					user_33(params, result, sizeof(result));
					error = 0;
					break;
				case 34:
					user_34(params, result, sizeof(result));
					error = 0;
					break;
				case 35:
					user_35(params, result, sizeof(result));
					error = 0;
					break;
				case 36:
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
		printf("PROVIDER: ERROR receiving capability: UID_receiveProviderCapability() returns %d", recv);
	}
	else {
		printf("PROVIDER: Valid capability received!");
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

	printf("PROVIDER: pre-accept %s - %d - %d", arg->msg, arg->len, strlen(arg->msg));
	ret = UID_accept_channel((uint8_t*)(arg->msg), strlen(arg->msg)+1, &sctx, sbuffer, &ssize);
	if ( UID_MSG_OK != ret) {
		if(!decodeCapability((uint8_t*)arg->msg)){
+			printf("PROVIDER: error accept %d", ret);
		}
		return;
	}

	//uint8_t rbuffer[2048] = {0};
	size_t rsize = sizeof(rbuffer);
	memset(rbuffer,0,rsize);
	// perform the request
	if ( UID_MSG_OK != (ret = perform_request(sbuffer, ssize, rbuffer, &rsize, &sctx, &error))) {
		printf("PROVIDER: error perform %d", ret);
		return;
	}

	printf("PROVIDER: it's ok!");
	service_out.msg = rbuffer;
	service_out.type = arg->type;
	if(service_out.type == 0){
		service_out.len = rsize-1;
		sprintf(service_out.serviceUserAddress, "%s", sctx.contract.serviceUserAddress);
		mqtt_send(&service_out);
	}	
	UID_closeServerChannel(&sctx);
}

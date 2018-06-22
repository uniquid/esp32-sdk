/*
 * @file   UID_httpal.c
 *
 * @date   09/dec/2017
 * @author M. Palumbi
 */

/**
 * @file UID_httpal.h
 *
 * http access abstraction layer
 *
 */

#include <string.h>
#include "mongoose.h"
#include "UID_httpal.h"
//#include "UID_log.h"

static volatile int exit_flag0 = -1;
static char * volatile buffer0 = NULL;
static volatile size_t size0 = 0;

static void ev_handler0(struct mg_connection *c, int ev, void *p)
{
    if (ev == MG_EV_HTTP_REPLY) 
    {
        struct http_message *hm = (struct http_message *)p;
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
        if(hm->body.len < size0)
        {
            memcpy(buffer0, hm->body.p, hm->body.len);
            buffer0[hm->body.len] = 0;
            exit_flag0 = -3;
        }
        else{
            exit_flag0 = -2;
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        int flag = exit_flag0;
        exit_flag0 = 1;
        if(-3 == flag) exit_flag0 = 0;
        if(-2 == flag) exit_flag0 = 2;
    }
}

/**
 * Get data from url
 *
 * @param[in]  curl   pointer to an initialized UID_HttpOBJ struct
 * @param[in]  url    url to contact
 * @param[out] buffer pointer to buffer to be filled
 * @param[in]  size   size of buffer
 *
 * @return     UID_HTTP_OK no error
 */
int UID_httpget(UID_HttpOBJ *curl, char *url, char *buffer, size_t size)
{
    struct mg_mgr mgr;

    exit_flag0 = -1;
    buffer0 = buffer;
    size0 = size;

    mg_mgr_init(&mgr, NULL);
    mg_connect_http(&mgr, MG_CB(ev_handler0,44), url, NULL, NULL);


    while (exit_flag0 < 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return (exit_flag0 == 0 ? UID_HTTP_OK : UID_HTTP_GET_ERROR);
}

static volatile int exit_flag1 = -1;
static char * volatile buffer1 = NULL;
static volatile size_t size1 = 0;

static void ev_handler1(struct mg_connection *c, int ev, void *p)
{
    if (ev == MG_EV_HTTP_REPLY) 
    {
        struct http_message *hm = (struct http_message *)p;
        c->flags |= MG_F_CLOSE_IMMEDIATELY;
        if(hm->body.len < size1)
        {
            memcpy(buffer1, hm->body.p, hm->body.len);
            buffer1[hm->body.len] = 0;
            exit_flag1 = -3;
        }
        else{
            fwrite(hm->body.p, 1, hm->body.len, stdout);
            printf("/n====> %d %d\n", size1, hm->body.len);
            exit_flag1 = -2;
        }
    }
    else if (ev == MG_EV_CLOSE)
    {
        int flag = exit_flag1;
        exit_flag1 = 1;
        if(-3 == flag) exit_flag1 = 0;
        if(-2 == flag) exit_flag1 = 2;
    }
}


int UID_httppost(UID_HttpOBJ *curl, char *url, char *postdata, char *ret, size_t size)
{
    struct mg_mgr mgr;

    exit_flag1 = -1;
    buffer1 = ret;
    size1 = size;

    mg_mgr_init(&mgr, NULL);
    mg_connect_http(&mgr, MG_CB(ev_handler1,44), url, "Content-Type: application/x-www-form-urlencoded\r\n", postdata);


    while (exit_flag1 < 0) {
        mg_mgr_poll(&mgr, 1000);
    }
    mg_mgr_free(&mgr);

    return (exit_flag1 == 0 ? UID_HTTP_OK : UID_HTTP_GET_ERROR);
}

UID_HttpOBJ *UID_httpinit()
{
    return NULL;
}

int UID_httpcleanup(UID_HttpOBJ *curl)
{
    return UID_HTTP_OK;
}

/* Enna Media Server - a library and daemon for medias indexation and streaming
 *
 * Copyright (C) 2012 Enna Team. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <Ecore.h>
#include <Ecore_Con.h>

#include "Ems.h"
#include "ems_private.h"
#include "ems_utils.h"
#include "ems_database.h"
#include "cJSON.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define EMS_TMDB_API_KEY "ba2eed549e5c84e712931fef2b69bfb1"
#define EMS_TMDB_QUERY_SEARCH "http://api.themoviedb.org/2.1/Movie.search/en/json/%s/%s"
#define EMS_TMDB_QUERY_INFO   "http://api.themoviedb.org/2.1/Movie.getInfo?id=%s&api_key=%s"
#define VH_ISALNUM(c) isalnum ((int) (unsigned char) (c))
#define VH_ISGRAPH(c) isgraph ((int) (unsigned char) (c))
#define VH_ISSPACE(c) isspace ((int) (unsigned char) (c))
#define IS_TO_DECRAPIFY(c)                      \
  ((unsigned) (c) <= 0x7F                       \
   && (c) != '\''                               \
   && !VH_ISSPACE (c)                           \
   && !VH_ISALNUM (c))

typedef struct _Ems_Tmdb_Req Ems_Tmdb_Req;
typedef enum _Ems_Request_State Ems_Request_State;

enum _Ems_Request_State
  {
    EMS_REQUEST_STATE_SEARCH,
    EMS_REQUEST_STATE_INFO
  };

struct _Ems_Tmdb_Req
{
   const char *filename;
   const char *search;
   Eina_Strbuf *buf;
   Ecore_Con_Url *ec_url;
   Ems_Request_State state;
   void (*end_cb)(void *data, const char *filename);
   void *data;
};

typedef void (*Ems_Grabber_End_Cb)(void *data, const char *filename);

static Eina_Hash *_hash_req = NULL;

static void
_request_free_cb(Ems_Tmdb_Req *req)
{
   if (!req)
     return;
   if (req->filename) eina_stringshare_del(req->filename);
   if (req->search) eina_stringshare_del(req->search);
   if (req->buf) eina_strbuf_free(req->buf);
   free(req);
}


static Eina_Bool
_grabber_tmdb_init(void)
{
   INF("Init TMDb grabber");
   ecore_con_init();
   ecore_con_url_init();

   _hash_req = eina_hash_pointer_new((Eina_Free_Cb)_request_free_cb);

   return EINA_TRUE;
}


static void
_grabber_tmdb_shutdown(void)
{
   INF("Shutdown TMDb grabber");
   eina_hash_free(_hash_req);
   ecore_con_url_shutdown();
   ecore_con_shutdown();
}


Eina_Bool
_search_data_cb(void *data __UNUSED__, int type __UNUSED__, Ecore_Con_Event_Url_Data *ev)
{
   Ems_Tmdb_Req *req = eina_hash_find(_hash_req, ev->url_con);

   if (!req || ev->url_con != req->ec_url)
     return ECORE_CALLBACK_RENEW;

   if (req->buf)
     eina_strbuf_append_length(req->buf, (char*)&ev->data[0], ev->size);

   return ECORE_CALLBACK_RENEW;
}

#define GETVAL(val, type, eina_type)                                    \
  do {                                                                  \
     cJSON *it;                                                         \
     Eina_Value v;                                                      \
     it = cJSON_GetObjectItem(m, #val);                                 \
     eina_value_setup(&v, eina_type);                                   \
     if (it) {                                                          \
         eina_value_set(&v, it->type);                                  \
         ems_database_meta_insert(ems_config->db, req->filename, #val, &v); \
     }                                                                  \
     eina_value_flush(&v);                                              \
  } while(0);                                                           \

#define GETVALSTR(val, type, eina_type)                                 \
  do {                                                                  \
     cJSON *it;                                                         \
     Eina_Value v;                                                      \
     it = cJSON_GetObjectItem(m, #val);                                 \
     eina_value_setup(&v, eina_type);                                   \
     if (it) {                                                          \
         eina_value_set(&v, eina_stringshare_add(it->type));            \
         ems_database_meta_insert(ems_config->db, req->filename, #val, &v); \
     }                                                                  \
     eina_value_flush(&v);                                              \
  } while(0);                                                           \


static Eina_Bool
_search_complete_cb(void *data __UNUSED__, int type __UNUSED__, void *event_info)
{
   Ecore_Con_Event_Url_Complete *url_complete = event_info;

   Ems_Tmdb_Req *req = eina_hash_find(_hash_req, url_complete->url_con);

   if (!req || url_complete->url_con != req->ec_url)
     {
        ERR("There is maybe a problem here ?");
        return EINA_TRUE;
     }

   DBG("download completed for %s with status code: %d", req->filename, url_complete->status);
   if (url_complete->status != 200)
     {
        if (req->end_cb)
          req->end_cb(req->data, req->filename);
        eina_hash_del(_hash_req, req->ec_url, req);
        ecore_con_url_free(req->ec_url);
        return EINA_FALSE;
     }


   switch(req->state)
     {
      case EMS_REQUEST_STATE_SEARCH:
         if (req->buf)
           {
              cJSON *root;
              cJSON *m;
              int size = 0;

              //DBG("Search request data : %s", eina_strbuf_string_get(req->buf));
              root = cJSON_Parse(eina_strbuf_string_get(req->buf));
              if (root)
                size = cJSON_GetArraySize(root);

              //DBG("Size %d", size);

              if (!size)
                {
                   DBG("No result found");
                   goto end_req;
                   return EINA_FALSE;
                }
              else if (size > 1)
                {
                   DBG("More then one result, take the first");
                }

              m = cJSON_GetArrayItem(root, 0);
              if (!m)
                {
                   ERR("Unable to get movie info");
                   goto end_req;
                }

              ems_database_transaction_begin(ems_config->db);
              GETVAL(score, valuedouble, EINA_VALUE_TYPE_DOUBLE);
              GETVAL(popularity, valueint, EINA_VALUE_TYPE_INT);
              GETVAL(translated, valueint, EINA_VALUE_TYPE_INT);
              GETVAL(adult, valueint, EINA_VALUE_TYPE_INT);
              GETVALSTR(language, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(original_name, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(name, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(alternative_name, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVAL(id, valueint, EINA_VALUE_TYPE_INT);
              GETVAL(imdb_id, valueint, EINA_VALUE_TYPE_INT);
              GETVALSTR(type, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(url, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVAL(votes, valueint, EINA_VALUE_TYPE_INT);
              GETVAL(rating, valuedouble, EINA_VALUE_TYPE_DOUBLE);
              GETVALSTR(certification, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(overview, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVALSTR(released, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              GETVAL(version, valueint, EINA_VALUE_TYPE_INT);
              GETVALSTR(last_modified_at, valuestring, EINA_VALUE_TYPE_STRINGSHARE);
              ems_database_transaction_end(ems_config->db);
              cJSON_Delete(root);
           end_req:
              if (req->end_cb)
                  req->end_cb(req->data, req->filename);
              eina_hash_del(_hash_req, req->ec_url, req);
              ecore_con_url_free(req->ec_url);
              return EINA_FALSE;
           }
         else
           {
              if (req->end_cb)
                req->end_cb(req->data, req->filename);
           }
         break;
      case EMS_REQUEST_STATE_INFO:;
      default:
         break;
     }

   if (req->end_cb)
     req->end_cb(req->data, req->filename);
   ecore_con_url_free(req->ec_url);
   eina_hash_del(_hash_req, req->ec_url, req);

   return EINA_FALSE;
}




/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

EAPI void
ems_grabber_grab(const char *filename, Ems_Media_Type type, Ems_Grabber_End_Cb end_cb, void *data)
{
   char url[PATH_MAX];
   Ecore_Con_Url *ec_url = NULL;
   char *tmp;
   char *search;
   Ems_Tmdb_Req *req;

   if (type != EMS_MEDIA_TYPE_VIDEO)
     return;

   DBG("Grab %s of type %d", filename, type);

   tmp = ems_utils_decrapify(filename);
   if (tmp)
     {
        search = ems_utils_escape_string(tmp);
        free(tmp);
        if (!search)
          return;
     }

   snprintf(url, sizeof (url), EMS_TMDB_QUERY_SEARCH,
            EMS_TMDB_API_KEY, search);

   DBG("Search for %s", url);

   ec_url = ecore_con_url_new(url);
   if (!ec_url)
     {
        ERR("error when creating ecore con url object.");
        return;
     }

   req = calloc(1, sizeof(Ems_Tmdb_Req));
   req->filename = eina_stringshare_add(filename);
   req->search = eina_stringshare_add(search);
   if (search)
     free(search);
   req->ec_url = ec_url;
   req->end_cb = end_cb;
   req->data = data;
   req->buf = eina_strbuf_new();

   ecore_event_handler_add(ECORE_CON_EVENT_URL_COMPLETE, (Ecore_Event_Handler_Cb)_search_complete_cb, NULL);
   ecore_event_handler_add(ECORE_CON_EVENT_URL_DATA, (Ecore_Event_Handler_Cb)_search_data_cb, NULL);
   req->state = EMS_REQUEST_STATE_SEARCH;

   eina_hash_add(_hash_req, ec_url, req);

   if (!ecore_con_url_get(ec_url))
     {
        ERR("could not realize request.");
        eina_hash_del(_hash_req, ec_url, req);
        ecore_con_url_free(ec_url);
     }
}

EINA_MODULE_INIT(_grabber_tmdb_init);
EINA_MODULE_SHUTDOWN(_grabber_tmdb_shutdown);

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

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

#include <sys/stat.h>

#include <Ecore.h>
#include <Eio.h>

#include "ems_private.h"
#include "ems_scanner.h"
#include "ems_config.h"
#include "ems_database.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

typedef struct _Ems_Scanner Ems_Scanner;

struct _Ems_Scanner
{
   int is_running;
   Ecore_Timer *schedule_timer;
   Eina_List *scan_files;
   int progress;
   double start_time;
   Ems_Database *db;
   Eina_List *eio_files;
};

static Eina_Bool _file_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info);
static void _file_done_cb(void *data, Eio_File *handler);
static void _file_error_cb(void *data, Eio_File *handler, int error);

static Ems_Scanner *_scanner = NULL;

static Eina_Bool
_schedule_timer_cb(void *data)
{

   ems_scanner_start();

   _scanner->schedule_timer = NULL;

   return EINA_FALSE;
}

static Eina_Bool
_ems_util_has_suffix(const char *name, const char *extensions)
{
   Eina_Bool ret = EINA_FALSE;
   int i;
   char *ext;
   char **arr = eina_str_split(extensions, ",", 0);

   if (!arr)
     return EINA_FALSE;

   for (i = 0; arr[i]; i++)
     {
        if (eina_str_has_extension(name, arr[i]))
          {
             ret = EINA_TRUE;
             break;
          }
     }
   free(arr[0]);
   free(arr);

   return ret;
}


static Eina_Bool
_file_filter_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   const char *ext = NULL;
   Ems_Directory *dir = data;

   if (*(info->path + info->name_start) == '.' )
     return EINA_FALSE;

   switch (dir->type)
     {
      case EMS_MEDIA_TYPE_VIDEO:
         ext = ems_config->video_extensions;
         break;
      case EMS_MEDIA_TYPE_MUSIC:
         ext = ems_config->music_extensions;
         break;
      case EMS_MEDIA_TYPE_PHOTO:
         ext = ems_config->photo_extensions;
         break;
      default:
         ERR("Unknown type %d", dir->type);
         return EINA_FALSE;
     }

   if ( info->type == EINA_FILE_DIR ||
        _ems_util_has_suffix(info->path + info->name_start, ext))
     return EINA_TRUE;
   else
     return EINA_FALSE;
}

static void
_file_main_cb(void *data, Eio_File *handler, const Eina_File_Direct_Info *info)
{
   Ems_Directory *dir = data;

   if (*(info->path + info->name_start) == '.' )
     return;

   if (info->type == EINA_FILE_DIR)
     {
        //DBG("[DIR] %s", info->path);
     }
   else
     {
        const char *type;
        struct stat st;

        switch (dir->type)
          {
           case EMS_MEDIA_TYPE_VIDEO:
              type = eina_stringshare_add("video");
              break;
           case EMS_MEDIA_TYPE_MUSIC:
              type = eina_stringshare_add("music");
              break;
           case EMS_MEDIA_TYPE_PHOTO:
              type = eina_stringshare_add("photo");
              break;
           default:
              ERR("Unknown type %d", dir->type);
              type = NULL;
              break;
          }

        DBG("[FILE] [%s] %s", type, info->path);
        eina_stringshare_del(type);
        /* TODO : add this file only if it doesn't exists in the db */
        _scanner->scan_files = eina_list_append(_scanner->scan_files,
                                                eina_stringshare_add(info->path));

        lstat(info->path, &st);

        ems_database_file_insert(_scanner->db, info->path, (int64_t)st.st_mtime);
        if (!eina_list_count(_scanner->scan_files) % 100)
          {
             ems_database_transaction_end(_scanner->db);
             ems_database_transaction_begin(_scanner->db);
          }
        /* TODO: add this file in the database */
        /* TODO: Add this file in the scanner list */
     }
}

static void
_file_done_cb(void *data, Eio_File *handler)
{
   Ems_Directory *dir = data;

   _scanner->is_running--;

   if (!_scanner->is_running)
     {
        const char *f;
        Eio_File *eio_file;
        ems_database_transaction_end(_scanner->db);
        ems_database_release(_scanner->db);

	/* Schedule the next scan */
	if (_scanner->schedule_timer)
	  ecore_timer_del(_scanner->schedule_timer);
	if (ems_config->scan_period)
	  {
	     _scanner->schedule_timer = ecore_timer_add(ems_config->scan_period, _schedule_timer_cb, NULL);
	     /* TODO: covert time into a human redeable value */
	     INF("Scan finished in %3.3fs, schedule next scan in %d seconds", ems_config->scan_period, ecore_time_get() - _scanner->start_time);
	  }
	else
	  {
	     INF("Scan finished in %3.3fs, scan schedule disabled according to the configuration.", ecore_time_get() - _scanner->start_time);
	  }

        INF("%d file scanned\n", eina_list_count(_scanner->scan_files));
        /* Free the scan list */
        EINA_LIST_FREE(_scanner->scan_files, f)
          eina_stringshare_del(f);

        EINA_LIST_FREE(_scanner->eio_files, eio_file)
          eio_file_cancel(eio_file);

        _scanner->eio_files = NULL;
        _scanner->scan_files = NULL;
        _scanner->progress = 0;
        _scanner->start_time = 0;
     }
}

static void
_file_error_cb(void *data, Eio_File *handler, int error)
{
   Ems_Directory *dir = data;
   /* _scanner->is_running--; */
   ERR("Unable to parse %s", dir->path);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

Eina_Bool
ems_scanner_init(void)
{
   _scanner = calloc(1, sizeof(Ems_Scanner));
   if (!_scanner)
     return EINA_FALSE;

   _scanner->db = ems_database_new("test.db");

   if (!_scanner->db)
     return EINA_FALSE;


   return EINA_TRUE;
}

void
ems_scanner_shutdown(void)
{
   if (_scanner)
     {
        const char *f;
        Eio_File *eio_file;

        if (_scanner->schedule_timer)
          ecore_timer_del(_scanner->schedule_timer);

        if (_scanner->scan_files)
          EINA_LIST_FREE(_scanner->scan_files, f)
            eina_stringshare_del(f);
        if (_scanner->eio_files)
          EINA_LIST_FREE(_scanner->eio_files, eio_file)
            eio_file_cancel(eio_file);

	if (_scanner->schedule_timer)
	  ecore_timer_del(_scanner->schedule_timer);

        free(_scanner);
     }
}

void
ems_scanner_start(void)
{
   Eina_List *l;
   Ems_Directory *dir;
   Eio_File *f;

   if (!_scanner)
     {
        ERR("Init scanner first : ems_scanner_init()");
     }

   if (_scanner->is_running)
     {
        WRN("Scanner is already running, did you try to run the scanner twice ?");
        return;
     }

   _scanner->start_time = ecore_time_get();
   ems_database_prepare(_scanner->db);
   ems_database_transaction_begin(_scanner->db);
   /* TODO : get all files in the db and see if they exist on the disk */

   /* Scann all files on the disk */
   INF("Scanning videos directories :");
   EINA_LIST_FOREACH(ems_config->video_directories, l, dir)
     {
        _scanner->is_running++;
        INF("Scanning %s: %s", dir->label, dir->path);
        f = eio_dir_stat_ls(dir->path,
                            _file_filter_cb,
                            _file_main_cb,
                            _file_done_cb,
                            _file_error_cb,
                            dir);
        _scanner->eio_files = eina_list_append(_scanner->eio_files, f);
     }

   INF("Scanning tvshow directories :");
   EINA_LIST_FOREACH(ems_config->tvshow_directories, l, dir)
     {
        _scanner->is_running++;
        INF("Scanning %s: %s", dir->label, dir->path);
        f = eio_dir_stat_ls(dir->path,
                            _file_filter_cb,
                            _file_main_cb,
                            _file_done_cb,
                            _file_error_cb,
                            dir);
        _scanner->eio_files = eina_list_append(_scanner->eio_files, f);
     }

   INF("Scanning tvshow directories :");
   EINA_LIST_FOREACH(ems_config->music_directories, l, dir)
     {
        _scanner->is_running++;
        INF("Scanning %s: %s", dir->label, dir->path);
        f = eio_dir_stat_ls(dir->path,
                            _file_filter_cb,
                            _file_main_cb,
                            _file_done_cb,
                            _file_error_cb,
                            dir);
        _scanner->eio_files = eina_list_append(_scanner->eio_files, f);
     }

   INF("Scanning photo directories :");
   EINA_LIST_FOREACH(ems_config->photo_directories, l, dir)
     {
        _scanner->is_running++;
        INF("Scanning %s: %s", dir->label, dir->path);
        f = eio_dir_stat_ls(dir->path,
                            _file_filter_cb,
                            _file_main_cb,
                            _file_done_cb,
                            _file_error_cb,
                            dir);
        _scanner->eio_files = eina_list_append(_scanner->eio_files, f);
     }

}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

void
ems_scanner_state_get(Ems_Scanner_State *state, double *percent)
{

   if (!_scanner)
     return;

   if (_scanner->is_running && state)
     *state = EMS_SCANNER_STATE_RUNNING;
   else if (state)
     *state = EMS_SCANNER_STATE_IDLE;

   if (eina_list_count(_scanner->scan_files) && percent)
     *percent = (double) _scanner->progress / eina_list_count(_scanner->scan_files);
   else if (percent)
     *percent = 0.0;

   INF("Scanner is %s (%3.3f%%)", _scanner->is_running ? "running" : "in idle", (double) _scanner->progress * 100.0 / eina_list_count(_scanner->scan_files));

}


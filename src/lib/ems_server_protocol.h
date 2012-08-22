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

#ifndef _EMS_SERVER_PROTOCOL_H_
#define _EMS_SERVER_PROTOCOL_H_
#include <Eina.h>
#include <Eet.h>

typedef struct _Medias Medias;
typedef struct _Medias_Req Medias_Req;
typedef struct _Medias_Infos Medias_Infos;
typedef struct _Media_Infos_Req Media_Infos_Req;
typedef struct _Media_Infos Media_Infos;
typedef enum _Ems_Server_Protocol_Type Ems_Server_Protocol_Type;
typedef struct _Ems_Server_Protocol Ems_Server_Protocol;

struct _Medias_Req
{
   Ems_Collection *collection;
};

struct _Medias
{
   Eina_List *files;
};

struct _Media_Infos_Req
{
   const char *uuid;
   const char *metadata;
};


struct _Media_Infos
{
   const char *value;
};

void ems_server_protocol_init(void);

extern Eet_Data_Descriptor *ems_medias_req_edd;
extern Eet_Data_Descriptor *ems_medias_add_edd;
extern Eet_Data_Descriptor *ems_media_infos_edd;
extern Eet_Data_Descriptor *ems_media_infos_req_edd;

#endif /* _EMS_SERVER_PROTOCOL_H_ */

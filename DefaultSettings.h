#ifndef DEFAULTSETTINGS_H
#define DEFAULTSETTINGS_H

/* You can redefine all these value in your user setting file
 * in ~/.config/Enna/EnnaMediaServer.conf
 */

#define EMS_HTTP_PORT 7336
#define EMS_WEBSOCKET_PORT 7337

#define EMS_HTTP_PORT 7336

/* DATABASE
 * --------- */
// database/path
#define EMS_DATABASE_PATH "database.sqlite"
// database/create_script
#define EMS_DATABASE_CREATE_SCRIPT "database.sql"
// database/version
#define EMS_DATABASE_VERSION 1

#define EMS_CACHE_DIRECTORY "~/.cache/enna-media-server"

/* PLAYER
 * --------- */
// player/host
// Here you can set an IP address or a UNIX socket
// IMPORTANT: you must use either a socket or a patched MPD to use file:// protocol
#define EMS_MPD_IP "/run/mpd/socket"
// player/port
#define EMS_MPD_IP "127.0.0.1"
// player/port
#define EMS_MPD_PORT 6600
// player/timeout
#define EMS_MPD_TIMEOUT 5000

// player/status_period
#define EMS_MPD_STATUS_PERIOD 1000
// player/retry_period
#define EMS_MPD_CONNECTION_RETRY_PERIOD 10000
// player/password (optional - if no password, let it empty)
#define EMS_MPD_PASSWORD ""

#endif // DEFAULTSETTINGS_H

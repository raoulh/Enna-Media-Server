######################################################################
# Automatically generated by qmake (3.0) lun. janv. 19 23:37:31 2015
######################################################################

QT += testlib
QT += core network websockets sql dbus
QT -= gui
QT_CONFIG -= no-pkg-config
CONFIG += c++11 link_pkgconfig

SUBDIRS +=

PKGCONFIG += libmpdclient libcdio libcdio_cdda libcdio_paranoia flac flac++ sndfile taglib

TEMPLATE = app
TARGET = enna-media-server
INCLUDEPATH +=

# Input
HEADERS += Database.h \
           DirectoryWorker.h \
           DiscoveryServer.h \
           sha1.h \
           WebSocketServer.h \
           Application.h \
           Data.h \
           DefaultSettings.h \
           WebSocket.h \
           JsonApi.h \
           Player.h \
           SmartmontoolsNotifier.h \
           CdromManager.h \
           HttpServer.h \
           external/http-parser/http_parser.h \
           HttpClient.h \
           CdromManager.h \
           MetadataManager.h \
           MetadataPlugin.h \
           FlacPlugin.h \
           LocalFileScanner.h \
           SndfilePlugin.h \
           DsdPlugin.h \
           CoverLocalPlugin.h \
           CdromRipper.h \
           WavEncoder.h \
           FlacEncoder.h \
           TagLibPlugin.h \
           SoundCardManager.h

SOURCES += Database.cpp \
           DirectoryWorker.cpp \
           DiscoveryServer.cpp \
           main.cpp \
           sha1.cpp \
           WebSocketServer.cpp \
           Application.cpp \
           JsonApi.cpp \
           Player.cpp \
           SmartmontoolsNotifier.cpp \
           CdromManager.cpp \
           HttpServer.cpp \
           external/http-parser/http_parser.c \
           HttpClient.cpp \
           MetadataManager.cpp \
           FlacPlugin.cpp \
           LocalFileScanner.cpp \
           SndfilePlugin.cpp \
           DsdPlugin.cpp \
           CoverLocalPlugin.cpp \
           CdromRipper.cpp \
           WavEncoder.cpp \
           FlacEncoder.cpp \
           TagLibPlugin.cpp \
           SoundCardManager.cpp

DISTFILES +=

equals(EMS_PLUGIN_GRACENOTE, "yes") {
    message("Enable Gracenote plugin")
    DEFINES += EMS_PLUGIN_GRACENOTE
    PKGCONFIG += gnsdk
    HEADERS += GracenotePlugin.h
    SOURCES += GracenotePlugin.cpp
}

# INSTALL RULES
#
isEmpty(PREFIX) {
    PREFIX = /usr/local
}
DEFINES += EMS_INSTALL_PREFIX="$$PREFIX"

# install the binary
binary.path = $$PREFIX/bin
binary.files = $$TARGET
INSTALLS += binary

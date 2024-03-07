
ACQ_PRI_FILE=$$(UTSRV)/include/acq/acq.pri
!exists($${ACQ_PRI_FILE}) {
    error($${ACQ_PRI_FILE} is not exist!)
}
include($${ACQ_PRI_FILE})

QT -= gui

TARGET = acqprotocolpluginmtrlock
TEMPLATE = lib
DESTDIR = $${LIB_PATH}
CONFIG += debug console plugin
DEFINES +=ACQ_PROTOCOL104_PLUGIN_INTERFACE_LIB
QMAKE_CXXFLAGS += -std=c++0x

unix {
DEFINES += PLATFORM_LINUX
}

win32 {
DEFINES += PLATFORM_WIN32
}

OBJECTS_DIR = $${ACQ_OBJ_PATH}/yfk-acq-plugin/acq-protocol-plugin-104
MOC_DIR = $${ACQ_OBJ_PATH}/yfk-acq-plugin/acq-protocol-plugin-104

INCLUDEPATH += $${YFKACQ_INC_PATH} \
        $${YFKACQ_INC_PATH}/acq-util/acq-json \
        $${YFKACQ_INC_PATH}/acq-util/acq-logger \
        $${ACQ_INC_PATH} \
        $${ACQ_INC_PATH}/yfk-acq-plugin \
        $${COMMON_INC_PATH}/utlog

HEADERS = protocol_plugin104.h \
    plugin104_define.h \
    proc_thread.h \
#   data_type.h \
    data/init_json_data.h \
    data/send_json_data.h \
    iec104/iec104.h \
    iec104/iec104def.h \
    iec104/iec104zf.h \
    iec104/ieccomm.h \
    iec104/poll.h \
    iec104/proto_def.h \
    iec104/protocol.h \
    iec104/prottype_def.h \
    iec104/buffer_t.h \
    iec104/scn_routeinf.h \
    iec104/scn_rtuinf.h \
    iec104/scn_rawdatadb.h \
    iec104/sicd.h \
    iec104/proto_cmdmem.h \
    iec104/scn_commandmem.h \
    utils/command_util.h \
    utils/converter_util.h \
    protocol_session.h \
    task_thread.h \
    iec104/scn_tasklist.h

SOURCES += protocol_plugin104.cpp \
    proc_thread.cpp \
    iec104/iec104.cxx \
    iec104/iec104zf.cxx \
    iec104/poll.cxx \
    iec104/protocol.cxx \
    iec104/buffer_t.cxx \
    iec104/scn_routeinf.cxx \
    iec104/scn_rtuinf.cxx \
    iec104/scn_rawdatadb.cxx \
    iec104/scn_commandmem.cxx \
    utils/command_util.cpp \
    utils/converter_util.cpp \
    protocol_session.cpp \
    iec104/scn_tasklist.cxx \
    task_thread.cpp

LIBS += -L$${LIB_PATH} \
        -lacqprotocolinterface \
        -lacqjson \
        -lacqlogger \
        -lacqprotocolpluginutils

QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-comment

# Headless tests of flow5-lib and flow5-io-lib

TEMPLATE = app
TARGET = tst_flow5lib

QT = core testlib

include(../common/common.pri)

SOURCES += tst_flow5lib.cpp

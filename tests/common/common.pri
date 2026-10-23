# Settings shared by the test programs: C++20, the three libraries of the main build, and the fixture

CONFIG += c++20 testcase no_testcase_installs
CONFIG -= app_bundle

DEFINES += OPENBLAS

# the libraries of the main build, in build/<config>/
FL5BUILD = $$clean_path($$OUT_PWD/../..)

INCLUDEPATH += $$PWD/../../XFoil-lib
INCLUDEPATH += $$PWD/../../flow5-lib
INCLUDEPATH += $$PWD/../../flow5-lib/api
INCLUDEPATH += $$PWD/../../flow5-io-lib
INCLUDEPATH += $$PWD/../../flow5-io-lib/api

LIBS += -L$$FL5BUILD/XFoil-lib -lXFoil
LIBS += -L$$FL5BUILD/flow5-lib -lflow5-lib
LIBS += -L$$FL5BUILD/flow5-io-lib -lflow5-io-lib
LIBS += -lopenblasp

# flow5-lib headers include OpenCascade headers
INCLUDEPATH += /usr/local/include/opencascade /usr/include/opencascade

HEADERS += $$PWD/fixture.h
SOURCES += $$PWD/fixture.cpp

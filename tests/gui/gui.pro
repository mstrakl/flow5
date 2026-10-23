# GUI tests, run with the Qt offscreen platform.
# They link the object files of the app built in build/<config>/flow5-app, so that the code tested
# is the code shipped; main.o is left out, the test has its own main().

TEMPLATE = app
TARGET = tst_gui

QT += opengl widgets xml openglwidgets testlib

include(../common/common.pri)

INCLUDEPATH += $$PWD/../../flow5-app

APPOBJECTS = $$files($$FL5BUILD/flow5-app/objects/*.o)
APPOBJECTS -= $$FL5BUILD/flow5-app/objects/main.o
isEmpty(APPOBJECTS): error("No object files in $$FL5BUILD/flow5-app/objects: build the app first")
LIBS += $$APPOBJECTS

# the app's other dependencies, as in flow5-app.pro
LIBS += -lgmsh -lGL
LIBS += -lTKBRep -lTKBO -lTKG3d -lTKGeomAlgo -lTKGeomBase -lTKLCAF -lTKMath -lTKMesh -lTKOffset -lTKPrim \
        -lTKDESTEP -lTKShHealing -lTKTopAlgo -lTKXSBase -lTKernel -lTKBool -lTKG2d -lTKCDF -lTKFillet
LIBS += -L/usr/local/lib -L/usr/lib64 -L/usr/local/lib64

SOURCES += tst_gui.cpp

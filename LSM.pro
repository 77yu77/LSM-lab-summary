TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        MemTable.cpp \
        MurmurHash3.cpp \
        correctness.cc \
        kvstore.cc \
        persistence.cc \
        testperformance.cc \
        utils.cpp

DISTFILES += \
    README.md

HEADERS += \
    MemTable.h \
    MurmurHash3.h \
    kvstore.h \
    kvstore_api.h \
    test.h \
    utils.h

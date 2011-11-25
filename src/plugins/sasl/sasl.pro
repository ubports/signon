TARGET = saslplugin

include( ../plugins.pri )

HEADERS += saslplugin.h \
        sasldata.h

SOURCES += saslplugin.cpp

LIBS += -lsasl2

headers.files = $$HEADERS
INSTALLS += headers

pkgconfig.files = signon-saslplugin.pc
INSTALLS += pkgconfig

include(doc/doc.pri)

TARGET = saslplugin

include( ../plugins.pri )

HEADERS += saslplugin.h \
        sasldata.h

SOURCES += saslplugin.cpp

PKGCONFIG += sasl2
LIBS += -lsasl2

headers.files = $$HEADERS
INSTALLS += headers

pkgconfig.files = signon-saslplugin.pc
INSTALLS += pkgconfig

include(doc/doc.pri)

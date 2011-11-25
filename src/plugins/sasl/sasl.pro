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

# override target path, it gets reset somewhere
target.path  = $${INSTALL_LIBDIR}/signon

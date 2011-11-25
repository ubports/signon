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

# override target paths, gets reset somewhere
headers.path = $${INSTALL_PREFIX}/include/signon-plugins
target.path  = $${INSTALL_LIBDIR}/signon

TARGET = ssotest2plugin

include( ../plugins.pri )

QT += gui

HEADERS += ssotest2plugin.h \
           ssotest2data.h

SOURCES += ssotest2plugin.cpp

headers.files = $$HEADERS
INSTALLS += headers

captcha_images.path = $${INSTALL_PREFIX}/lib/signon/captcha_images
captcha_images.files = Captcha1.jpg \
                       Captcha2.jpg \
                       Captcha3.jpg \
                       Captcha4.jpg

INSTALLS += captcha_images


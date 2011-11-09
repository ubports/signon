TEMPLATE = subdirs
SUBDIRS =

system(pkg-config --exists libcryptsetup) {
    SUBDIRS += cryptsetup
}


TEMPLATE = subdirs
SUBDIRS =

system(pkg-config --exists libcryptsetup) {
    SUBDIRS += cryptsetup
}

system(pkg-config --exists mssf-qt) {
    SUBDIRS += mssf-ac
}

system(pkg-config --exists smack-qt) {
    SUBDIRS += smack-ac
}

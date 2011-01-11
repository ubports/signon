TEMPLATE = subdirs
SUBDIRS =

system(pkg-config --exists CellularQt) {
    exists(/usr/include/devicelock/devicelock.h) {
	SUBDIRS += sim-dlc
    }
}




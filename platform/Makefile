


all: osx-all linux-all

osx-all: osx-i386
linux-all: linux-i386

linux-i386:
	cd linux ; \
	make TOPDIR=$(TOPDIR)

osx-i386:
	cd osx ; \
	make TOPDIR=$(TOPDIR)

clean:
	cd osx ; make clean
	cd linux ; make clean

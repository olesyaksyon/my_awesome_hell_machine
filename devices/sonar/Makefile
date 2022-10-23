obj-m := sonar.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

install: all
	cp -f sonar.ko /lib/modules/$(shell uname -r)/

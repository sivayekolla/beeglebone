# Specify the kernel module to be built
obj-m += task_dy.o    

# Build target: compiles the kernel module
all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

# Clean target: removes all generated files from build
clean:
	# Clean the module build files
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

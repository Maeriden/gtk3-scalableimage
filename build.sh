#! /bin/bash

if ! [ -e build ]; then
	mkdir build
fi

if ! [ -d build ]; then
	echo "ERROR: unable to create 'build' directory"
	exit 1
fi

gcc -c "src/gtkscalableimage.c" -o "build/gtkscalableimage.o" $(pkg-config --cflags --libs gtk+-3.0) -g
if [[ $? != 0 ]]; then
	echo "Build failed"
	exit 1
fi

echo "Build succeded"

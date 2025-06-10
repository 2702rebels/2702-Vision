MAKEFLAGS := --jobs=$(shell nproc)

all:
	make -C lib
	make -C Utilities/log2mjpeg
	make -C Utilities/logffmpeg
	make -C Utilities/vid-sample
	make -C tests
	@if command -v rpicam-vid &> /dev/null; then \
	    make -C Utilities/rpi-sample \
		make -C 2025/Object \
	else \
		make -C 2025/Object norpicam \
	fi

notest:
	make -C lib
	make -C 2025/Object
	make -C Utilities/log2mjpeg
	make -C Utilities/logffmpeg
	make -C Utilities/vid-sample
	@if command -v rpicam-vid &> /dev/null; then \
	    make -C Utilities/rpi-sample \
		make -C 2025/Object \
	else \
		make -C 2025/Object norpicam \
	fi

test:
	make -C lib
	make -C tests
	./tests/test

clean:
	make -C lib clean
	make -C 2025/Object clean
	make -C Utilities/log2mjpeg clean
	make -C Utilities/logffmpeg clean
	make -C Utilities/rpi-sample clean
	make -C Utilities/vid-sample clean
	make -C tests clean

cleandeps:
	rm -rf vcpkg
	rm -rf vcpkg_installed

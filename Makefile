MAKEFLAGS := --jobs=$(shell nproc)

all:
	make -C lib
	make -C Utilities/log2mjpeg
	make -C Utilities/logffmpeg
	make -C Utilities/vid-sample
	make -C tests
	@command -v rpicam-vid; \
	if [ $$? -eq 0 ]; then \
	    make -C Utilities/rpi-sample; \
		make -C 2025/Object; \
	else \
		make -C 2025/Object norpicam; \
	fi

notest:
	make -C lib
	make -C Utilities/log2mjpeg
	make -C Utilities/logffmpeg
	make -C Utilities/vid-sample
	@command -v rpicam-vid; \
	if [ $$? -eq 0 ]; then \
	    make -C Utilities/rpi-sample; \
		make -C 2025/Object; \
	else \
		make -C 2025/Object norpicam; \
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

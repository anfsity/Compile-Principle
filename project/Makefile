IMAGE = maxxing/compiler-dev
BUILD_DIR = cmake-build

UID := $(shell id -u)
GID := $(shell id -g)
PWD := $(shell pwd)

all: build

configure:
	cmake -S . -B $(BUILD_DIR)

build: configure
	cmake --build $(BUILD_DIR) -j12

clean:
	rm -rf $(BUILD_DIR)

shell:
	docker run -it --rm \
			-u $(UID):$(GID) \
			-v "$(PWD):$(PWD)" \
			-w "$(PWD)" \
			$(IMAGE) bash

run:
	./cmake-build/compiler -koopa ./tests/hello.c -o debug/hello.koopa
	./cmake-build/compiler -koopa ./tests/unary.c -o debug/unary.koopa
	./cmake-build/compiler -koopa ./tests/binary.c -o debug/binary.koopa
	./cmake-build/compiler -koopa ./tests/logic.c -o debug/logic.koopa

docker-build:
	docker run --rm \
		-u $(UID):$(GID) \
		-v "$(PWD):$(PWD)" \
		-w "$(PWD)" \
		$(IMAGE) \
		sh -c "cmake -S . -B $(BUILD_DIR) && cmake --build $(BUILD_DIR) -j12"
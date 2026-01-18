IMAGE = maxxing/compiler-dev
BUILD_DIR = cmake-build

UID := $(shell id -u)
GID := $(shell id -g)
PWD := $(shell pwd)

all: build

config:
	cmake -S . -B cmake-build -G Ninja -DCMAKE_MAKE_PROGRAM=$(PWD)/ninja

build: config
	cmake --build $(BUILD_DIR) -j12

clean:
	rm -rf $(BUILD_DIR)

shell:
	docker run -it --rm \
			-u $(UID):$(GID) \
			-v "$(PWD):$(PWD)" \
			-w "$(PWD)" \
			-e PATH="$(PWD):/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin" \
			$(IMAGE) bash

run:
	./cmake-build/compiler -koopa ./tests/hello.c -o ./debug/hello.koopa
	./cmake-build/compiler -riscv ./tests/hello.c -o ./debug/hello.asm

test:
	cd cmake-build && ctest && cd ..

docker-build:
	docker run --rm \
		-u $(UID):$(GID) \
		-v "$(PWD):$(PWD)" \
		-w "$(PWD)" \
		$(IMAGE) \
		sh -c "cmake -S . -B $(BUILD_DIR) && cmake --build $(BUILD_DIR) -j12"
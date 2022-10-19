CC = emcc
PLATFORM=PLATFORM_WEB
INCLUDE_PATHS=../raylib/src

BUILD_WEB_RESOURCES_PATH ?= resources

build:
	mkdir build
	$(CC) -o build/index.html main.c -Os -Wall -I $(INCLUDE_PATHS) -L $(INCLUDE_PATHS) -s USE_GLFW=3 -s ASYNCIFY --shell-file minshell.html --preload-file $(BUILD_WEB_RESOURCES_PATH) -D$(PLATFORM) -lraylib

clean:
	rm -rf build/

run:
	cd build/ && python -m http.server

LINKER := *.o -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lvulkan
FLAGS := -std=c++14 -Wall -Wextra -Wshadow -Wunreachable-code -Wsometimes-uninitialized -Wno-unused-function -pedantic -msse3
DEFINES := -DVK_USE_PLATFORM_XCB_KHR
CC := clang++-6.0
SANITIZERS := -g -fsanitize=address,undefined

SHADER := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S vert -e waterVS water.hlsl -o ../build/water_vs.spv)"; echo $$SHADER
SHADER := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S frag -e waterFS water.hlsl -o ../build/water_fs.spv)"; echo $$SHADER

all:
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c renderer_vulkan.cpp -o renderer_vulkan.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c window_xcb.cpp -o window_xcb.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c file.cpp -o file.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c texture.cpp -o texture.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c shader.cpp -o shader.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c vertexbuffer_vulkan.cpp -o vertexbuffer.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) main.cpp $(LINKER) -o ../build/naturedemo


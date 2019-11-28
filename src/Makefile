LINKER := *.o -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lvulkan
FLAGS := -std=c++14 -Wall -Wextra -Wshadow -Wunreachable-code -Wno-unused-function -pedantic -msse3
DEFINES := -DVK_USE_PLATFORM_XCB_KHR
CC := clang++-8
SANITIZERS := -g -fsanitize=address,undefined

SHADER1 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S vert -e mainVS water.hlsl -o ../build/water_vs.spv)"
SHADER2 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S frag -e mainFS water.hlsl -o ../build/water_fs.spv)"
SHADER3 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S vert -e mainVS sky.hlsl -o ../build/sky_vs.spv)"
SHADER4 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S frag -e mainFS sky.hlsl -o ../build/sky_fs.spv)"
SHADER5 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S vert -e mainVS ground.hlsl -o ../build/ground_vs.spv)"
SHADER6 := "$(shell $$VULKAN_SDK/bin/glslangValidator -D -V -S frag -e mainFS ground.hlsl -o ../build/ground_fs.spv)"

all:
	echo $(SHADER1)
	echo $(SHADER2)
	echo $(SHADER3)
	echo $(SHADER4)
	echo $(SHADER5)
	echo $(SHADER6)
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c renderer_vulkan.cpp -o renderer_vulkan.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c window_xcb.cpp -o window_xcb.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c file.cpp -o file.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c matrix.cpp -o matrix.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c mesh.cpp -o mesh.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c texture.cpp -o texture.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c shader.cpp -o shader.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c vertexbuffer_vulkan.cpp -o vertexbuffer.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) main.cpp $(LINKER) -o ../build/naturedemo


LINKER := *.o -ldl -lxcb -lxcb-ewmh -lxcb-keysyms -lxcb-icccm -lX11-xcb -lvulkan
FLAGS := -std=c++14 -Wall -Wextra -Wshadow -Wunreachable-code -Wsometimes-uninitialized -Wno-unused-function -pedantic -msse3
DEFINES := -DVK_USE_PLATFORM_XCB_KHR
CC := clang++-6.0
SANITIZERS := -g -fsanitize=address,undefined

all:
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c window_xcb.cpp -o window_xcb.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c file.cpp -o file.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) -c shader.cpp -o shader.o
	$(CC) $(FLAGS) $(DEFINES) $(SANITIZERS) main.cpp $(LINKER) -o ../build/naturedemo


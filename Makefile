INCLUDES = -I./lib/headers -I./src/util
FLAGS = -Wall -fPIC -g $(INCLUDES)
LIBFLAGS = -L./lib/binaries
LDFLAGS = -lGL -lglfw3
CFLAGS = -std=c99
CPPFLAGS = -std=c++0x
CC = g++

.PHONY: clean

main: obj/glad.o obj/render.o obj/main.o 
	$(CC) $(FLAGS) $(LIBFLAGS) -o $@ $^ $(LDFLAGS)

obj/main.o: main.c
	$(CC) $(FLAGS) $(INCLUDES) $(CFLAGS) -o $@ -c $^

obj/render.o: render.c obj/shader_constants.h obj/glad.o
	$(CC) $(FLAGS) $(INCLUDES) $(CFLAGS) -o $@ -c render.c

obj/shader_constants.h: shaders/vertex.glsl shaders/fragment.glsl
	bash -c "printf '#ifndef SHADER_CONSTANTS_H\n#define SHADER_CONSTANTS_H\n' > obj/shader_constants.h"
	bash -c "xxd -i shaders/vertex.glsl | tac | sed '3s/$$/, 0x00/' | tac >> obj/shader_constants.h"
	bash -c "xxd -i shaders/fragment.glsl | tac | sed '3s/$$/, 0x00/' | tac >> obj/shader_constants.h"
	bash -c "printf '#endif' >> obj/shader_constants.h"

obj/glad.o: obj/shader_constants.h lib/headers/glad.c
	$(CC) $(FLAGS) $(INCLUDES) -c $(CFLAGS) $^  -o $@

clean:
	rm -f obj/main.o obj/render.o obj/glad.o obj/main main obj/shader_constants.h

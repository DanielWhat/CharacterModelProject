#g++ -Wall -o "%e" "%f" -lGL -lGLU -lglut -lGLEW -lassimp -lIL -ILU
CC = g++
CFLAGS = -Wall
FRAMEWORKS = -lGL -lGLU -lglut -lGLEW -lassimp -lIL -ILU

all: army_pilot.out

army_pilot.o: army_pilot.cpp ./../initial_meshes.h ./../render_scene.h
	$(CC) -c $(FLAGS) $< -o $@
	
initial_meshes.o: ./../initial_meshes.cpp ./../initial_meshes.h
	$(CC) -c $(FLAGS) $< -o $@

render_scene.o: ./../render_scene.cpp ./../initial_meshes.h
	$(CC) -c $(FLAGS) $< -o $@

army_pilot.out: army_pilot.o ./../initial_meshes.o render_scene.o
	$(CC) $(CFLAGS) $^ -o $@ -lm $(FRAMEWORKS)

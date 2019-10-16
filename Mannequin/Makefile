#g++ -Wall -o "%e" "%f" -lGL -lGLU -lglut -lGLEW -lassimp -lIL -ILU
CC = g++
CFLAGS = -Wall
FRAMEWORKS = -lGL -lGLU -lglut -lGLEW -lassimp -lIL -ILU

all: mannequin.out

mannequin.o: mannequin.cpp ./../initial_meshes.h ./../render_scene.h ./../floor_plane.h
	$(CC) -c $(FLAGS) $< -o $@

initial_meshes.o: ./../initial_meshes.cpp ./../initial_meshes.h
	$(CC) -c $(FLAGS) $< -o $@

render_scene.o: ./../render_scene.cpp ./../render_scene.h
	$(CC) -c $(FLAGS) $< -o $@

floor_plane.o: ./../floor_plane.cpp ./../floor_plane.h
	$(CC) -c $(FLAGS) $< -o $@

mannequin.out: mannequin.o initial_meshes.o render_scene.o floor_plane.o
	$(CC) $(CFLAGS) $^ -o $@ -lm $(FRAMEWORKS)

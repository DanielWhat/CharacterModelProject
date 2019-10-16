CC = g++
CFLAGS = -Wall
FRAMEWORKS = -lGL -lGLU -lglut -lGLEW -lassimp -lIL -ILU

all: dwarf.out

dwarf.o: dwarf.cpp ./../floor_plane.h ./../render_scene.h ./../initial_meshes.h
	$(CC) -c $(FLAGS) $< -o $@

initial_meshes.o: ./../initial_meshes.cpp ./../initial_meshes.h
	$(CC) -c $(FLAGS) $< -o $@

render_scene.o: ./../render_scene.cpp ./../render_scene.h
	$(CC) -c $(FLAGS) $< -o $@

floor_plane.o: ./../floor_plane.cpp ./../floor_plane.h
	$(CC) -c $(FLAGS) $< -o $@

dwarf.out: dwarf.o floor_plane.o render_scene.o initial_meshes.o
	$(CC) $(CFLAGS) $^ -o $@ -lm $(FRAMEWORKS)

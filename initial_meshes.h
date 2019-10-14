#ifndef INITIAL_MESHES_H
#define INITIAL_MESHES_H

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>

struct InitialMesh
/* This is a data type that stores a meshes orginal vertex and normal positions */
{
	int mNumVertices;
	bool* seen_before;
	aiVector3D* mVertices;
	aiVector3D* mNormals;
};


InitialMesh* initialise_transform_vertices(const aiScene* the_scene);
/* Takes a scene and returns the initial mesh data in a list*/



#endif /* INITIAL_MESHES_H */

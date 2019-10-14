#include "initial_meshes.h"


InitialMesh* initialise_transform_vertices(const aiScene* the_scene)
/* Takes a scene and returns the initial mesh data in a list*/
{
	InitialMesh* meshes = new InitialMesh[the_scene->mNumMeshes]; //allocate the memory to store the number of meshes we need

	//iterate through each mesh in the scene
	for (int mesh_index = 0; mesh_index <  the_scene->mNumMeshes; mesh_index++) {
		aiMesh* mesh = the_scene->mMeshes[mesh_index];

		//for each mesh initialise an entry in the initial_meshes list
		meshes[mesh_index].mNumVertices = mesh->mNumVertices;
		meshes[mesh_index].mVertices = new aiVector3D[mesh->mNumVertices]; //allocate memory to store all the verticies and normals for this mesh
		meshes[mesh_index].mNormals = new aiVector3D[mesh->mNumVertices];
		meshes[mesh_index].seen_before = new bool[mesh->mNumVertices];

		//now actually assign the data
		for (int i = 0; i < mesh->mNumVertices; i++) {
			meshes[mesh_index].mVertices[i] = mesh->mVertices[i];
			meshes[mesh_index].mNormals[i] = mesh->mNormals[i];
			meshes[mesh_index].seen_before[i] = false;
		}
	}
	
	return meshes;
}


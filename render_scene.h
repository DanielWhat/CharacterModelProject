#ifndef RENDER_SCENE_H
#define RENDER_SCENE_H

#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <GL/freeglut.h>
#include <map>



void draw_mesh(const aiScene* the_scene, const aiNode* node, int node_index, std::map<int, int> texture_id_map, bool is_shadow);
/* Draws a mesh when given the scene object, the node object which has the mesh
 * you want to draw, the mesh index specifying the mesh you want to draw, and a
 * texture ID map mapping material indexes to texture IDs. If is_shadow is true,
 * then will draw a black mesh.
 *
 * NOTE: Humans should not be using this function, use render instead */


void render (const aiScene* the_scene, const aiNode* node, std::map<int, int> texIdMap, bool is_shadow=false);
/* Traverses the scene graph (which starts at Node), and draws each mesh.
 * Requires a texture ID map mapping each material index to the
 * corresponding texture ID. If no textures are available then simply
 * provide an empty map.
 *
 * NOTE: Uses legacy OpenGL functions such as PushMatrix() */


#endif /* RENDER_SCENE_H*/

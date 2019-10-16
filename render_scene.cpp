#include "render_scene.h"



void draw_mesh(const aiScene* the_scene, const aiNode* node, int node_index, std::map<int, int> texture_id_map, bool is_shadow)
/* Draws a mesh when given the scene object, the node object which has the mesh
 * you want to draw, the mesh index specifying the mesh you want to draw, and a
 * texture ID map mapping material indexes to texture IDs. If is_shadow is true,
 * then will draw a black mesh.
 *
 * NOTE: Humans should not be using this function, use render instead */
{
	float blue[] = {0, 0, 1, 1};
    aiMesh* mesh;
    aiFace* face;
    aiMaterial* material;
    aiColor4D diffuse;
    int mesh_index, material_index;

    mesh_index = node->mMeshes[node_index]; //get the mesh index for this mesh
    mesh = the_scene->mMeshes[mesh_index];  //get the actual meshes

    material_index = mesh->mMaterialIndex;  //get the material index assigned to the mesh
    material = the_scene->mMaterials[material_index];  //now get the actual material object using the material index

    //try to get the material, and put the material into diffuse
    if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse) && !is_shadow) {
        glColor4f(diffuse.r, diffuse.g, diffuse.b, 1);
    } else if (is_shadow) {
		glColor3f(0.2, 0.2, 0.2);
	} else {
        glColor4fv(blue); //default colour
    }

    //now actually draw the mesh's polygons
    for (int face_index = 0; face_index < mesh->mNumFaces; face_index++) {
        face = &mesh->mFaces[face_index];
        GLenum face_mode;

        //select a different face mode, depending on the number of indices the face has
        switch(face->mNumIndices) {
            case 1: face_mode = GL_POINTS; break;
            case 2: face_mode = GL_LINES; break;
            case 3: face_mode = GL_TRIANGLES; break;
            default: face_mode = GL_POLYGON; break;
        }

        if (mesh->HasTextureCoords(0)) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texture_id_map[material_index]); //the material index associated with a mesh relates to the
		}

        //draw the actual polygons that form the mesh
        glBegin(face_mode);
            for (int i = 0; i < face->mNumIndices; i++) {
                int vertex_index = face->mIndices[i];

                //if there is a colour assigned to this vertex use it
                if (mesh->HasVertexColors(0)) {
                    glColor4fv((GLfloat*) &mesh->mColors[0][vertex_index]);
                }

                if (mesh->HasTextureCoords(0)) {
					glTexCoord2f(mesh->mTextureCoords[0][vertex_index].x, mesh->mTextureCoords[0][vertex_index].y);
				}

                //the mesh has vertex normals then use them
                if (mesh->HasNormals()) {
                    glNormal3fv(&mesh->mNormals[vertex_index].x);
                }

                glVertex3fv(&mesh->mVertices[vertex_index].x); //draw the actual face vertex
            }
        glEnd();
    }
}






void render (const aiScene* the_scene, const aiNode* node, std::map<int, int> texIdMap, bool is_shadow)
/* Traverses the scene graph (which starts at Node), and draws each mesh.
 * Requires a texture ID map mapping each material index to the
 * corresponding texture ID. If no textures are available then simply
 * provide an empty map.
 *
 * NOTE: Uses legacy OpenGL functions such as PushMatrix() */
{
    aiMatrix4x4 matrix = node->mTransformation;

    aiTransposeMatrix4(&matrix); //assimp uses row-major ordering, whereas openGL uses column major ordering, so we need to convert the assimp matrix to column major
    glPushMatrix();
        glMultMatrixf((float*)&matrix); //multiply by the transformation matrix corresponding to this Node

        //draw the meshes assigned to this node (note: we are not drawing child nodes here, only meshes assigned to this node)
        for (int node_index = 0; node_index < node->mNumMeshes; node_index++) {
            draw_mesh(the_scene, node, node_index, texIdMap, is_shadow);
        }
        //recursively draw all of this node's children
        for (int i = 0; i < node->mNumChildren; i++) {
            render(the_scene, node->mChildren[i], texIdMap, is_shadow);
        }

    glPopMatrix();
}

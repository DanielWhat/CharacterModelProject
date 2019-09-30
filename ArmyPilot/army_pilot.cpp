#include <IL/il.h>
#include <GL/freeglut.h>
#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>
using namespace std;
#include "assimp_extras.h"

const aiScene* scene = NULL;
float blue[4] = {0, 0, 1, 1};
aiVector3D scene_min, scene_max, scene_center;


void render (const aiScene* the_scene, const aiNode* node)
/* Traverses the scene graph (which starts at Node), and draws each mesh.
 * NOTE: Uses legacy OpenGL functions such as PushMatrix() */
{
    aiMatrix4x4 matrix = node->mTransformation;
    aiMesh* mesh;
    aiFace* face;
    aiMaterial* material;
    aiColor4D diffuse;
    int mesh_index, material_index;

    aiTransposeMatrix4(&matrix); //assimp uses row-major ordering, whereas openGL uses column major ordering, so we need to convert the assimp matrix to column major
    glPushMatrix();
        glMultMatrixf((float*)&matrix); //multiply by the transformation matrix corresponding to this Node

        //draw the meshes assigned to this node (note: we are not drawing child nodes here, only meshes assigned to this node)
        for (int node_index = 0; node_index < node->mNumMeshes; node_index++) {
            mesh_index = node->mMeshes[node_index]; //get the mesh index for this mesh
            mesh = the_scene->mMeshes[mesh_index];  //get the actual meshes

            material_index = mesh->mMaterialIndex;  //get the material index assigned to the mesh
            material = the_scene->mMaterials[material_index];  //now get the actual material object using the material index

            //try to get the material, and put the material into diffuse
            if (AI_SUCCESS == aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &diffuse)) {
                glColor4f(diffuse.r, diffuse.g, diffuse.b, 1);
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

                //
                glBegin(face_mode);
                    for (int i = 0; i < face->mNumIndices; i++) {
                        int vertex_index = face->mIndices[i];

                        //if there is a colour assigned to this vertex use it
                        if (mesh->HasVertexColors(0)) {
                            glColor4fv((GLfloat*) &mesh->mColors[0][vertex_index]);
                        }

                        //the mesh has vertex normals then use them
                        if (mesh->HasNormals()) {
                            glNormal3fv(&mesh->mNormals[vertex_index].x);
                        }

                        glVertex3fv(&mesh->mVertices[vertex_index].x); //draw the ctual face vertex
                    }
                glEnd();
            }
        }
        //recursively draw all of this node's children
        for (int i = 0; i < node->mNumChildren; i++) {
            render(the_scene, node->mChildren[i]);
        }

    glPopMatrix();
}




void display()
{
    float light_position[4] = {0, 50, 50, 1};
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the depth and colour buffers

	glMatrixMode(GL_MODELVIEW); //tell openGL we are talking about the model-view matrix now
	glLoadIdentity();
	gluLookAt(0, 0, 3, 0, 0, -5, 0, 1, 0); //specify camera position
    
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

    glRotatef(-90, 0, 1, 0);
    glRotatef(90, 1, 0, 0);

    // scale the whole asset to fit into our view frustum
	float tmp = scene_max.x - scene_min.x;
	tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
	tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
	tmp = 1.f / tmp;
	glScalef(tmp, tmp, tmp);

    float xc = (scene_min.x + scene_max.x)*0.5;
	float yc = (scene_min.y + scene_max.y)*0.5;
	float zc = (scene_min.z + scene_max.z)*0.5;
	// center the model
	glTranslatef(-xc, -yc, -zc);

    render(scene, scene->mRootNode);

    //glutSolidTeapot(1);

    glutSwapBuffers();
}



void loadModel(const char* filename)
/* Loads the model/scene at the given filename into a GLOBAL VARIABLE called scene */
{
    scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
    if (scene == NULL) {
        exit(1);
    }
    get_bounding_box(scene, &scene_min, &scene_max);
}



void initialise()
{
    const float light_ambient[4] = {0.2, 0.2, 0.2, 1.0}; //ambient light's colour
    const float white[4] = {1, 1, 1, 1}; //light's colour

    glClearColor(1, 1, 1, 1); //se background colour to white
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_DEPTH_TEST); //"do depth comparisons and update the depth buffer"
    glEnable(GL_NORMALIZE); //normalise the unit vectors

    //set the light's colours
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
	glLightfv(GL_LIGHT0, GL_SPECULAR, white);

    glEnable(GL_COLOR_MATERIAL); //make the material colour the same as the object's glColor3f
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //ditto above
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white); //make the specular colour white for all objects
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 50);
    glColor4f(0, 0, 0, 1); //use blue as the default colour

    loadModel("ArmyPilot.x");

    //set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(35, 1, 1.0, 1000.0);
}



int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); //initialise the window with RGB colours, a double buffer for smoother animations, and a depth buffer
    glutInitWindowSize(600, 600);
    glutCreateWindow("Army Pilot");
    glutInitContextVersion(4, 2);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    initialise();
    glutDisplayFunc(display);
    glutMainLoop();
    aiReleaseImport(scene);
}

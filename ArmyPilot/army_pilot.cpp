#include <IL/il.h>
#include <map>
#include <GL/freeglut.h>
#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <iostream>
#include <string>
using namespace std;
#include "assimp_extras.h"

struct InitialMesh
/* This is a data type that stores a meshes, orginal vertex and normal positions */
{
	int mNumVertices;
	aiVector3D* mVertices;
	aiVector3D* mNormals;
};

InitialMesh* initial_meshes = NULL;

const aiScene* scene = NULL;
float blue[4] = {0, 0, 1, 1};
aiVector3D scene_min, scene_max, scene_center;

std::map<int, int>texIdMap;


//animation globals
int animation_duration;
int current_tick = 0;
float time_step = 30;



//-------------Loads texture files using DevIL library-------------------------------
void loadGLTextures(const aiScene* scene)
{
	/* initialization of DevIL */
	ilInit();
	if (scene->HasTextures())
	{
		std::cout << "Support for meshes with embedded textures is not implemented" << endl;
		return;
	}

	/* scan scene's materials for textures */
	/* Simplified version: Retrieves only the first texture with index 0 if present*/
    string head = "./head01.png";
    string body = "./body01.png";
    string m4 = "./m4tex.png";
    string paths[3] = {head, body, m4};

	for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
	{
		aiString path;  // filename

		if (scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
		{
			//we have to do this because the images are absolute paths for some reason
			strcpy(path.data, paths[m].c_str());

			glEnable(GL_TEXTURE_2D);
			ILuint imageId;
			GLuint texId;
			ilGenImages(1, &imageId);
			glGenTextures(1, &texId);
			texIdMap[m] = texId;   //store tex ID against material id in a hash map -- ok so m is nothing special, just a number. So all we do is assign a texture ID to this number m through a hash map, so that when we see meshes with material index "m" we can easily see what texture ID that relates to
			ilBindImage(imageId); /* Binding of DevIL image name */
			ilEnable(IL_ORIGIN_SET);
			ilOriginFunc(IL_ORIGIN_LOWER_LEFT);
			if (ilLoadImage((ILstring)path.data))   //if success
			{
				/* Convert image to RGBA */
				ilConvertImage(IL_RGBA, IL_UNSIGNED_BYTE);

				/* Create and load textures to OpenGL */
				glBindTexture(GL_TEXTURE_2D, texId);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ilGetInteger(IL_IMAGE_WIDTH),
					ilGetInteger(IL_IMAGE_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE,
					ilGetData());
				glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
				cout << "Texture:" << path.data << " successfully loaded." << endl;
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
				glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
				glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
			}
			else
			{
				cout << "Couldn't load Image: " << path.data << endl;
			}
		}
	}  //loop for material
}




void initialise_transform_vertices(const aiScene* the_scene)
/* Takes a scene and puts the initial meshes into a GLOBAL variable called
 * initial_meshes. <--- @ This is fine for now, but make it work without globals later @ */
{
	cout << "There are " << the_scene->mNumMeshes << " meshes in this object." << endl;

	initial_meshes = new InitialMesh[the_scene->mNumMeshes]; //allocate the memory to store the number of meshes we need

	//iterate through each mesh in the scene
	for (int mesh_index = 0; mesh_index <  the_scene->mNumMeshes; mesh_index++) {
		aiMesh* mesh = the_scene->mMeshes[mesh_index];

		//for each mesh initialise an entry in the initial_meshes list
		initial_meshes[mesh_index].mNumVertices = mesh->mNumVertices;
		initial_meshes[mesh_index].mVertices = new aiVector3D[mesh->mNumVertices]; //allocate memory to store all the verticies and normals for this mesh
		initial_meshes[mesh_index].mNormals = new aiVector3D[mesh->mNumVertices];

		//now actually assign the data
		for (int i = 0; i < mesh->mNumVertices; i++) {
			initial_meshes[mesh_index].mVertices[i] = mesh->mVertices[i];
			initial_meshes[mesh_index].mNormals[i] = mesh->mNormals[i];
		}
	}
}



void transform_verticies (const aiScene* the_scene)
{
	//This follows closely the methodology in slide 20 Lec. 9

	//for each mesh in the scene
	for (int mesh_index = 0; mesh_index < the_scene->mNumMeshes; mesh_index++) {
		aiMesh* mesh = the_scene->mMeshes[mesh_index];

		//for each bone of the mesh
		for (int bone_index = 0; bone_index < mesh->mNumBones; bone_index++) {
			aiBone* bone = mesh->mBones[bone_index];

			//ok so for what we're trying to do here is get the matrix, which will transform our verticies into the world namespace
			//first we need to get offset matricies, then we'll need to apply all the transformations going up the node hierarchy

			aiMatrix4x4 offset_matrix = bone->mOffsetMatrix; //get the offset matrix that transforms from the mesh space to the initial bind pose
															 //slides in Lec 8 pg 18 to 21 might help if you forget what offset matrix is.

			const aiNode* node = scene->mRootNode->FindNode(bone->mName); //find the node corresponding to this bone_index
			aiMatrix4x4 node_hierarchy_transformation = node->mTransformation; // get the node's transformation matrix


			//go up the node hierarchy
			while (node->mParent != 0) { //while node is not the root node
				node = node->mParent;
				node_hierarchy_transformation = node->mTransformation * node_hierarchy_transformation; //accumulate all the transformations together
			}

			aiMatrix4x4 final_transformation_matrix = node_hierarchy_transformation * offset_matrix; //Yay! Now we have the transformation matrix that we need to apply to all verticies of our bone.
			aiMatrix4x4 final_transformation_matrix_normals = final_transformation_matrix;
			final_transformation_matrix_normals.Transpose();
			final_transformation_matrix_normals.Inverse();

			//Now we transform all the verticies attached to the current bone with the above transfromation matrix
			//Remember that we transform from where the intial meshes were, so we gonna need to use our initial_meshes

			//weights are essentially a struct with the vertex_indicies and the transformation weight for that vertex
			for (int weight_index = 0; weight_index < bone->mNumWeights; weight_index++) {
				aiVertexWeight weight = bone->mWeights[weight_index];

				int vertex_index = weight.mVertexId;
				aiVector3D vertex = initial_meshes[mesh_index].mVertices[vertex_index];
				aiVector3D normal = initial_meshes[mesh_index].mNormals[vertex_index];

				mesh->mVertices[vertex_index] = final_transformation_matrix * vertex;
				mesh->mNormals[vertex_index] = final_transformation_matrix_normals * normal;
			}
		}
	}
}


//@copy this over properly
void update_node_matrices(int tick)
{
	int index;
	aiAnimation* anim = scene->mAnimations[0]; //get the first animation
	aiMatrix4x4 position_matrix, rotation_matrix, final_transformation_matrix;
	aiMatrix3x3 rotation_matrix3;
	aiNode* node;


	for (int i = 0; i < anim->mNumChannels; i++) {      //remember that we have a channel for each node (i.e forearm, hand, shoulder etc.)
		position_matrix = aiMatrix4x4(); //Identity matrix
		rotation_matrix = aiMatrix4x4();
		aiNodeAnim* channel = anim->mChannels[i];

		index = (channel->mNumPositionKeys > 1) ? tick : 0; //if we have an animation for the position for this channel, then update the index
															//note though that position is not actually animatable, it's just the offset matrix, but assimp gives it to us anyways.
		aiVector3D position = (channel->mPositionKeys[index]).mValue;
		position_matrix.Translation(position, position_matrix); //weird function, but this just says that position matrix should become a translation to "position"

		index = (channel->mNumRotationKeys > 1) ? tick : 0; //if we have an animation for the rotation for this channel, then update the index
		aiQuaternion rotation = (channel->mRotationKeys[index]).mValue;
		rotation_matrix3 = rotation.GetMatrix();
		rotation_matrix = aiMatrix4x4(rotation_matrix3);

		final_transformation_matrix = position_matrix * rotation_matrix;

		node = scene->mRootNode->FindNode(channel->mNodeName); //get the node/mesh corresponding to this channel
		node->mTransformation = final_transformation_matrix; //remember that position_matrix is the offset matrix, so we are actually only changing the rotation
	}
}




void draw_mesh(const aiScene* the_scene, const aiNode* node, int node_index)
/* Draws a mesh when given the scene object, the node object which has the mesh
 * you want to draw, and the mesh index specifying the mesh you want to draw.
 * NOTE: Humans should not be using this function, use render instead */
{
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

        if (mesh->HasTextureCoords(0)) {
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, texIdMap[material_index]); //the material index associated with a mesh relates to the
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

                glVertex3fv(&mesh->mVertices[vertex_index].x); //draw the ctual face vertex
            }
        glEnd();
    }
}






void render (const aiScene* the_scene, const aiNode* node)
/* Traverses the scene graph (which starts at Node), and draws each mesh.
 * NOTE: Uses legacy OpenGL functions such as PushMatrix() */
{
    aiMatrix4x4 matrix = node->mTransformation;

    aiTransposeMatrix4(&matrix); //assimp uses row-major ordering, whereas openGL uses column major ordering, so we need to convert the assimp matrix to column major
    glPushMatrix();
        glMultMatrixf((float*)&matrix); //multiply by the transformation matrix corresponding to this Node

        //draw the meshes assigned to this node (note: we are not drawing child nodes here, only meshes assigned to this node)
        for (int node_index = 0; node_index < node->mNumMeshes; node_index++) {
            draw_mesh(the_scene, node, node_index);
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

	transform_verticies(scene);
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
	printBoneInfo(scene);
    get_bounding_box(scene, &scene_min, &scene_max);
	animation_duration = scene->mAnimations[0]->mDuration;
}


void update_animation (int value)
{
	if (current_tick < animation_duration) {
		update_node_matrices(current_tick);
		glutTimerFunc(time_step, update_animation, 0);
		current_tick++;
	} else {
		current_tick = 0;
		update_node_matrices(current_tick);
		glutTimerFunc(time_step, update_animation, 0);
	}
	glutPostRedisplay();
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
    loadGLTextures(scene);
	initialise_transform_vertices(scene);

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
	glutTimerFunc(50, update_animation, 0);
    glutMainLoop();
    aiReleaseImport(scene);
}

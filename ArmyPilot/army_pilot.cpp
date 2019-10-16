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
#include "./../assimp_extras.h"
#include "./../initial_meshes.h"
#include "./../render_scene.h"
#include "./../floor_plane.h"


InitialMesh* initial_meshes = NULL;

const aiScene* scene = NULL;
float blue[4] = {0, 0, 1, 1};
aiVector3D scene_min, scene_max, scene_center;

std::map<int, int>texIdMap;


//animation globals
int animation_duration;
int current_tick = 0;
float time_step = 30;

float distance_x = 0;
float radius = 3;
float angle = 0;

//colours
const float white[4] = {1, 1, 1, 1};
const float black[4] = {0, 0, 0, 1};



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



void transform_vertices (const aiScene* the_scene)
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
				aiVertexWeight weight_obj = bone->mWeights[weight_index];

				int vertex_index = weight_obj.mVertexId;
				float weight = weight_obj.mWeight;
				aiVector3D vertex = initial_meshes[mesh_index].mVertices[vertex_index];
				aiVector3D normal = initial_meshes[mesh_index].mNormals[vertex_index];

				if (!initial_meshes[mesh_index].seen_before[vertex_index])  {
					mesh->mVertices[vertex_index] = (final_transformation_matrix * vertex) * weight;
					mesh->mNormals[vertex_index] = (final_transformation_matrix_normals * normal) * weight;
					initial_meshes[mesh_index].seen_before[vertex_index] = true;

				} else {
					mesh->mVertices[vertex_index] += (final_transformation_matrix * vertex) * weight;
					mesh->mNormals[vertex_index] += (final_transformation_matrix_normals * normal) * weight;
				}
			}
		}
	}

	//reset the flags for next time
	clear_flags(initial_meshes, the_scene->mNumMeshes);
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



void draw_army_pilot()
{
	glPushMatrix();
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

		transform_vertices(scene);
	    render(scene, scene->mRootNode, texIdMap);
	glPopMatrix();
}




void display()
{
	float character_mid_point_height = 0.36;
    float light_position[4] = {0, 5, 7, 1};
	float look_point[3] = {0, character_mid_point_height + (float)0.2, 0};
	float camera_point[3] = {0, 1, radius};

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the depth and colour buffers

	glMatrixMode(GL_MODELVIEW); //tell openGL we are talking about the model-view matrix now
	glLoadIdentity();
	gluLookAt(-sin(angle * (M_PI / 180))*camera_point[2] + distance_x, camera_point[1], cos(angle * (M_PI / 180)) * camera_point[2], look_point[0] + distance_x, look_point[1], look_point[2], 0, 1, 0); //specify camera position

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	glDisable(GL_TEXTURE_2D);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, black);
	draw_floor_plane(0.5, 150, 50);
	glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, white);
	glEnable(GL_TEXTURE_2D);

	//draw the floor plane
    glTranslatef(distance_x, character_mid_point_height, 0);
	glTranslatef(0.33, 0, 0);
	glRotatef(-14, 0, 0, 1);
    glRotatef(95, 1, 0, 0);

	//draw the model
	draw_army_pilot();

    glutSwapBuffers();
}



void loadModel(const char* filename)
/* Loads the model/scene at the given filename into a GLOBAL VARIABLE called scene */
{
    scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
    if (scene == NULL) {
        exit(1);
    }
	//printBoneInfo(scene);
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
	distance_x += 0.02;
	glutPostRedisplay();
}


void move_camera(int key, int x, int y)
{
	if (key == GLUT_KEY_UP) {
		radius -= 0.2;
	} else if (key == GLUT_KEY_DOWN) {
		radius += 0.2;
	} else if (key == GLUT_KEY_RIGHT) {
		angle += 2;
	} else if (key == GLUT_KEY_LEFT) {
		angle -= 2;
	}
	//glutPostRedisplay(); No need for this, will just update with the animation
}



void initialise()
{
    const float light_ambient[4] = {0.2, 0.2, 0.2, 1.0}; //ambient light's colour

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
	initial_meshes = initialise_transform_vertices(scene);

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
	glutSpecialFunc(move_camera);
    glutMainLoop();
    aiReleaseImport(scene);
}

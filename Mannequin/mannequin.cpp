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
const aiScene* animation = NULL;
float blue[4] = {0, 0, 1, 1};
aiVector3D scene_min, scene_max, scene_center;

std::map<int, int>texIdMap;

//colours
const float light_ambient[4] = {0.2, 0.2, 0.2, 1.0}; //ambient light's colour
const float white[4] = {1, 1, 1, 1}; //light's colour

//animation globals
int animation_duration;
float current_tick = 0;
float time_step = 13;
float duration_factor = 1;

//viewing stuff
float radius = 3;
float distance_x = 0;
float angle = 0;


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

	for (unsigned int m = 0; m < scene->mNumMaterials; ++m)
	{
		aiString path;  // filename

		if (scene->mMaterials[m]->GetTexture(aiTextureType_DIFFUSE, 0, &path) == AI_SUCCESS)
		{
			//we have to do this because the images are absolute paths for some reason
			//strcpy(path.data, paths[m].c_str());

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




void update_node_matrices(float tick, aiAnimation* anim)
{
	int index;
	aiMatrix4x4 position_matrix, position_matrix1, position_matrix2, rotation_matrix, final_transformation_matrix;
	aiMatrix3x3 rotation_matrix3;
	aiNode* node;


	for (int channel_index = 0; channel_index < anim->mNumChannels; channel_index++) {      //remember that we have a channel for each node (i.e forearm, hand, shoulder etc.)

		position_matrix = aiMatrix4x4(); //Identity matrix
		rotation_matrix = aiMatrix4x4();
		aiNodeAnim* channel = anim->mChannels[channel_index];

		index = 0;
		aiVector3D position = (channel->mPositionKeys[index]).mValue;
		position_matrix.Translation(position, position_matrix);


		index = 0;
		aiQuaternion rotation = (channel->mRotationKeys[index]).mValue;


		//make sure we actually have atleast two rotation keys
		//also make sure it is actually nessesary to interpolate
		if (channel->mNumRotationKeys > 1 && tick != 0) {
			for (int i = 1; i < channel->mNumRotationKeys; i++) {
				if (channel->mRotationKeys[i-1].mTime * duration_factor < tick && tick <= channel->mRotationKeys[i].mTime * duration_factor) {
					index = i;
				}
			}

			//get the two rotation quaternions we need to interpolate between
			aiQuaternion rotation1 = (channel->mRotationKeys[index-1]).mValue;
			float time1 = (channel->mRotationKeys[index-1]).mTime * duration_factor;

			aiQuaternion rotation2 = (channel->mRotationKeys[index]).mValue;
			float time2 = (channel->mRotationKeys[index]).mTime * duration_factor;

			float lambda = (tick - time1) / (time2 - time1); //takes takes value 0 if tick = time1 and value 1 if tick = time2
			rotation.Interpolate(rotation, rotation1, rotation2, lambda);
		}


		rotation_matrix3 = rotation.GetMatrix();
		rotation_matrix = aiMatrix4x4(rotation_matrix3);

		if (channel_index != 23) {
			final_transformation_matrix = position_matrix * rotation_matrix;
		} else {
			final_transformation_matrix = rotation_matrix;
		}

		node = scene->mRootNode->FindNode(channel->mNodeName);
		node->mTransformation = final_transformation_matrix;
	}
}


void draw_mannequin(bool is_shadow)
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
		render(scene, scene->mRootNode, texIdMap, is_shadow);
	glPopMatrix();
}




void display()
{
	float character_mid_point_height = 0.45;
    float light_position[4] = {0, 50, 50, 1};
	float look_point[3] = {0, character_mid_point_height + (float)0.2, 0};
	float camera_point[3] = {0, 1, radius};
	float shadow_matrix[16] = {light_position[1], 0,                  0,                 0,
							  -light_position[0], 0, -light_position[2],                 0,
						  	                   0, 0,  light_position[1],                 0,
										       0, 0,                  0, light_position[1]};


    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the depth and colour buffers

	glMatrixMode(GL_MODELVIEW); //tell openGL we are talking about the model-view matrix now
	glLoadIdentity();
	gluLookAt(-sin(angle * (M_PI / 180))*camera_point[2] + distance_x, camera_point[1], cos(angle * (M_PI / 180)) * camera_point[2], look_point[0] + distance_x, look_point[1], look_point[2], 0, 1, 0);

    glLightfv(GL_LIGHT0, GL_POSITION, light_position);

	//draw floor plane
	glPushMatrix();
		glTranslatef(0.5*100*0.5, 0, 0);
		draw_floor_plane(0.5, 100, 10);
	glPopMatrix();

	//draw character
	glPushMatrix();
		glTranslatef(distance_x, character_mid_point_height, 0);
		glTranslatef(-0.2, 0, 0);
		glRotatef(90, 0, 1, 0);
		draw_mannequin(false);
	glPopMatrix();

	//draw shadow
	glDisable(GL_LIGHTING);
	glPushMatrix();
		glTranslatef(distance_x, 0.01, 0);
		glTranslatef(-0.2, 0, 0);
		glMultMatrixf(shadow_matrix);
		glTranslatef(0, character_mid_point_height, 0);
		glRotatef(90, 0, 1, 0);
		draw_mannequin(true);
	glPopMatrix();
	glEnable(GL_LIGHTING);

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



void load_animation(const char* filename)
/* Loads the model/scene at the given filename into a GLOBAL VARIABLE called scene */
{
    animation = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
    if (scene == NULL) {
        exit(1);
    }
	//printAnimInfo(animation);
	animation_duration = animation->mAnimations[0]->mDuration;
}


void update_animation (int value)
{
	if (current_tick < animation_duration) {
		update_node_matrices(current_tick, animation->mAnimations[0]);
		glutTimerFunc(time_step, update_animation, 0);
		current_tick += 0.4;
	} else {
		current_tick = 0;
		update_node_matrices(current_tick, animation->mAnimations[0]);
		glutTimerFunc(time_step, update_animation, 0);
	}
	distance_x += 0.025;
	glutPostRedisplay();
}



void initialise()
{
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

    loadModel("mannequin.fbx");
	load_animation("run.fbx");
    //loadGLTextures(scene);
	initial_meshes = initialise_transform_vertices(scene);

    //set up the projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(35, 1, 1.0, 1000.0);
}


void move_camera(int key, int x, int y)
{
	if (key == GLUT_KEY_UP) {
		radius -= 0.2;
	} else if (key == GLUT_KEY_DOWN) {
		radius += 0.2;
	} else if (key == GLUT_KEY_RIGHT) {
		angle -= 2;
	} else if (key == GLUT_KEY_LEFT) {
		angle += 2;
	}
	//glutPostRedisplay(); No need for this, will just update with the animation
}


int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); //initialise the window with RGB colours, a double buffer for smoother animations, and a depth buffer
    glutInitWindowSize(600, 600);
    glutCreateWindow("Mannequin");
    glutInitContextVersion(4, 2);
    glutInitContextProfile(GLUT_CORE_PROFILE);

    initialise();
    glutDisplayFunc(display);
	glutTimerFunc(50, update_animation, 0);
	glutSpecialFunc(move_camera);
    glutMainLoop();
    aiReleaseImport(scene);
}

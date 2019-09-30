#include <IL/il.h>
#include <GL/freeglut.h>
#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

const aiScene* scene = NULL;
const float blue[4] = {0, 0, 1, 1};

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); //clear the depth and colour buffers

	glMatrixMode(GL_MODELVIEW); //tell openGL we are talking about the model-view matrix now
	glLoadIdentity();
	gluLookAt(0, 0, 3, 0, 0, -5, 0, 1, 0); //specify camera position
    glutSolidTeapot(1);

    glutSwapBuffers();
}



void loadModel(const char* filename, const aiScene* the_scene)
/* Loads the model/scene at the given filename into the variable the_scene */
{
    the_scene = aiImportFile(filename, aiProcessPreset_TargetRealtime_MaxQuality);
    if (the_scene == NULL) {
        exit(1);
    }
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
    glColor4fv(blue); //use blue as the default colour

    loadModel("ArmyPilot.x", scene);

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

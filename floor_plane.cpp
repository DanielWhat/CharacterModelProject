#include "floor_plane.h"

void draw_floor_plane(float square_size, int num_squares_x, int num_squares_z)
/* Takes a square size float, the number of squares in the x direction, and the
 * number of squares in the z direction. Draws a floor plane along the x,z axes
 *
 * NOTE: Uses legacy OpenGL */
{
	glPushMatrix();
		glTranslatef(-(num_squares_x*square_size)/2, 0, -(num_squares_z*square_size)/2);
		glBegin(GL_QUADS);
			for (int i = 0; i < num_squares_x; i++) {
				for (int j = 0; j < num_squares_z; j++) {
					if ((i + j) % 2 == 0) {
						glColor3f(0.9, 0, 0.9);
					} else {
						glColor3f(0, 0.9, 0.9);
					}
					glNormal3f(0, 1, 0);
					glVertex3f(square_size*i, 0, square_size*j);
					glVertex3f(square_size*(i+1), 0, square_size*j);
					glVertex3f(square_size*(i+1), 0, square_size*(j+1));
					glVertex3f(square_size*(i), 0, square_size*(j+1));
				}
			}
		glEnd();
	glPopMatrix();
}

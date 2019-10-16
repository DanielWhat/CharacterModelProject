#ifndef FLOOR_PLANE_H
#define FLOOR_PLANE_H

#include <GL/freeglut.h>

void draw_floor_plane(float square_size, int num_squares_x, int num_squares_z);
/* Takes a square size float, the number of squares in the x direction, and the
 * number of squares in the z direction. Draws a floor plane along the x,z axes
 *
 * NOTE: Uses legacy OpenGL */

#endif /* FLOOR_PLANE_H */

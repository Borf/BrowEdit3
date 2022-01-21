#include "Plane.h"


//glUseProgram(0);
//glColor4f(0, 1, 0, 1);
//glBegin(GL_LINES);
//glVertex3fv(glm::value_ptr(-mouseDragPlane.normal * mouseDragPlane.D));
//glVertex3fv(glm::value_ptr(-mouseDragPlane.normal * (mouseDragPlane.D + 100)));
//glEnd();
//glColor4f(1, 0, 0, 0.25f);

//glm::vec3 p1 = glm::normalize(glm::cross(mouseDragPlane.normal, glm::vec3(0, 0, 1)));
//if(glm::dot(mouseDragPlane.normal, glm::vec3(0, 0, 1)) > 0.99)
//	p1 = glm::normalize(glm::cross(mouseDragPlane.normal, glm::vec3(0, 1, 0)));
//glm::vec3 p2 = glm::normalize(glm::cross(p1, mouseDragPlane.normal));
//glBegin(GL_QUADS);
//glVertex3fv(glm::value_ptr(-(mouseDragPlane.normal * mouseDragPlane.D) + 1000.0f * p1 - 1000.0f * p2));
//glVertex3fv(glm::value_ptr(-(mouseDragPlane.normal * mouseDragPlane.D) + 1000.0f * p1 + 1000.0f * p2));
//glVertex3fv(glm::value_ptr(-(mouseDragPlane.normal * mouseDragPlane.D) - 1000.0f * p1 + 1000.0f * p2));
//glVertex3fv(glm::value_ptr(-(mouseDragPlane.normal * mouseDragPlane.D) - 1000.0f * p1 - 1000.0f * p2));
//glEnd();
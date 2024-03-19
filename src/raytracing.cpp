#include "raytracing.h"

#include <iostream>
#include <format>

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

/**
 *	Deconstructor.
 */
RTSettings::~RTSettings() {

}

/**
 *	Deconstructor.
 */
RTMaterial::~RTMaterial() {

}

/**
 *	Deconstructor.
 */
//RTSphere::~RTSphere() {
//	//delete &material; // TODO: Fix deletion
//}

/**
 *	Deconstructor.
 */
RTCamera::~RTCamera() {

}

 /**
  *	Calculates the Rotation-Translation-Scaling matrix for the camera.
  *	Automatically assigns the new value to Camera.rts
  */
void RTCamera::calculateRTS() {
	// Create translation matrix
	glm::mat4 translation = glm::translate(glm::mat4(1), pos);

	// Create rotation matrix
	glm::mat4	rotationX = glm::rotate(glm::mat4(1), ang.x, glm::vec3(1, 0, 0)),
				rotationY = glm::rotate(glm::mat4(1), ang.y, glm::vec3(0, 1, 0)),
				rotationZ = glm::rotate(glm::mat4(1), ang.z, glm::vec3(0, 0, 1));
	glm::mat4	rotation = rotationY * rotationX * rotationZ;

	// Create scaling matrix
	glm::mat4 scaling = glm::scale(glm::mat4(1), glm::vec3(1));

	// Combine into RTS matrix and calculate local space
	rts		= scaling * translation * rotation;
	left	= glm::normalize(rts[0]);
	up		= glm::normalize(rts[1]);
	front	= glm::normalize(rts[2]);
}
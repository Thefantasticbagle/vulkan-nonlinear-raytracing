#pragma once
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "glm/ext.hpp"

#include <array>

struct RTFrame {
	alignas(16) glm::vec3
		cameraPos;
	alignas(16) glm::mat4
		localToWorld;
	alignas(16) int
		frameNumber;
};

/**
 *  Struct containing parameters for UBO.
 *	Should match the one in the shaders.
 */
struct RTParams {
    // Camera
    glm::vec2   screenSize;
    float       fov,
                focusDistance;

    // Raytracing settings
    unsigned int    maxBounces,
                    raysPerFrag;
    float           divergeStrength,
                    blackholePower;

    // Other
    unsigned int    spheresCount,
                    blackholesCount;
};

/**
 *	Class for storing material settings.
 */
struct RTMaterial {
	alignas(16) glm::vec4
		color,
		emissionColor,
		specularColor;
	alignas(16) float
		smoothness;
};

/**
 *	Class for storing sphere information.
 */
class RTSphere {
public:
	alignas(16) float
		radius;
	alignas(16) glm::vec3
		center;
	alignas(16) RTMaterial
		material;
};

/**
 *	Class for storing black hole information.
 */
struct RTBlackhole {
	alignas(16) float
		radius;
	alignas(16) glm::vec3
		center;
};

/**
 *	Class for storing and calculating camera information.
 */
class RTCamera {
public:
	RTCamera(glm::vec3 position, glm::vec3 angle, glm::vec2 screenSize, float focusDistance, float fov, float zNear, float zFar)
		: pos(position), ang(angle), screenSize(screenSize), focusDistance(focusDistance), fov(fov), zNear(zNear), zFar(zFar) {
		calculateRTS();
	}

	/**
	 *	Calculates the Rotation-Translation-Scaling matrix for the camera.
	 */
	void calculateRTS() {
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

	// Calculated
	glm::mat4	rts;
	glm::vec3	left,
				up,
				front;

	// Assigned
	glm::vec3	pos,
				ang;
	glm::vec2	screenSize;
	float		focusDistance,
				fov,
				zNear,
				zFar;
	unsigned int frameNumber;
};
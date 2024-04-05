#ifndef GLSL_CPP_COMMON
#define GLSL_CPP_COMMON

// --- C++ macros
#ifdef __cplusplus
#define a16 alignas(16)
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

using vec2 = glm::vec2;
using vec3 = glm::vec3;
using vec4 = glm::vec4;
using mat4 = glm::mat4;
using uint = unsigned int;

#define START_BINDING(a) enum a {
#define END_BINDING() }

// --- GLSL macros
#else
#define a16
#define START_BINDING(a) const uint
#define END_BINDING()
#endif

// --- Bindings
START_BINDING( ComputeBindings )
	b_params		= 0,
	b_spheres		= 1,
	b_blackholes	= 2,
	b_image			= 3
END_BINDING();

// --- Structs
/**
 *	Struct containing information which should be updated every frame.
 */
struct RTFrame {
	a16 vec3 cameraPos;
	a16 mat4 localToWorld;
	a16 int frameNumber;
};

/**
 *  Struct containing parameters for UBO.
 *	Should match the one in the shaders.
 */
struct RTParams {
    // Camera
    vec2    screenSize;
    float   fov,
            focusDistance;

    // Raytracing settings
    uint    maxBounces,
            raysPerFrag;
    float   divergeStrength,
            blackholePower;

    // Other
    uint    spheresCount,
            blackholesCount;
};

/**
 *	Struct for storing material settings.
 */
struct RTMaterial {
	a16 vec4    color,
		        emissionColor,
		        specularColor;
	a16 float   smoothness;
};

/**
 *	Struct for storing sphere information.
 */
struct RTSphere {
	a16 float		radius;
	a16 vec3		center;
	a16 RTMaterial	material;
};

/**
 *	Struct for storing black hole information.
 */
struct RTBlackhole {
	a16 float	radius;
	a16 vec3	center;
};

#endif
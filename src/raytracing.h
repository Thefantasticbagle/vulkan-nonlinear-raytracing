#pragma once
#include <glm/glm.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <array>


/**
 *	Class for storing and sending raytracing settings.
 */
#pragma pack(push, 0)
class RTSettings {
public:
	~RTSettings();
	/*RTSettings(unsigned int maxBounces = 4, unsigned int raysPerFrag = 4, float divergeStrength = 0.03f, float blackholePower = 1.f)
		: maxBounces(maxBounces), raysPerFrag(raysPerFrag), divergeStrength(divergeStrength), blackholePower(blackholePower) {}*/

	unsigned int    maxBounces,		// 0B offset
					raysPerFrag;	// 4B offset
	float           divergeStrength,// 8B offset
					blackholePower; // 12B offset
	// 16B total size
};
#pragma pack(pop, 0)

/**
 *	Class for storing material settings.
 */
#pragma pack(push, 0)
class RTMaterial {
public:
	~RTMaterial();
	/*RTMaterial(glm::vec4 color = glm::vec4(1, 0, 0, 1), glm::vec4 emissionColor = glm::zero<glm::vec4>(), glm::vec4 specularColor = glm::zero<glm::vec4>(), float smoothness = 0.f)
		: color(color), emissionColor(emissionColor), specularColor(specularColor), smoothness(smoothness) {}*/

	glm::vec4	color,			// 0B offset
				emissionColor,	// 16B offset
				specularColor;	// 32B offset
	float		smoothness;		// 48B offset
	char		__padding[12];	// 52B offset
	// 64B total size
};
#pragma pack(pop)

/**
 *	Class for storing sphere information.
 */
//#pragma pack(push, 0)
//class RTSphere {
//public:
//	~RTSphere();
//	/*RTSphere(float radius = 1.f, glm::vec3 center = glm::zero<glm::vec3>(), RTMaterial material = RTMaterial())
//		: radius(radius), center(center), material(material) {}*/
//
//	//float		radius;			// 0B offset
//	//char		__padding0[12];	// 4B offset
//	//glm::vec3	center;			// 16B offset
//	//char		__padding1[4];	// 28B offset
//	//RTMaterial	material;		// 32B offset
//	// 96B total size
//
//	glm::vec4 radiusPos;
//
//	static VkVertexInputBindingDescription getBindingDescription() {
//		VkVertexInputBindingDescription bindingDescription{};
//		bindingDescription.binding = 0;
//		bindingDescription.stride = sizeof(RTSphere);
//		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
//
//		return bindingDescription;
//	}
//
//	static std::array<VkVertexInputAttributeDescription, 1> getAttributeDescriptions() {
//		std::array<VkVertexInputAttributeDescription, 1> attributeDescriptions{};
//
//		attributeDescriptions[0].binding = 0;
//		attributeDescriptions[0].location = 0;
//		attributeDescriptions[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
//		attributeDescriptions[0].offset = offsetof(RTSphere, radiusPos);
//
//		/*attributeDescriptions[0].binding = 0;
//		attributeDescriptions[0].location = 0;
//		attributeDescriptions[0].format = VK_FORMAT_R32_SFLOAT;
//		attributeDescriptions[0].offset = offsetof(RTSphere, radius);
//
//		attributeDescriptions[1].binding = 0;
//		attributeDescriptions[1].location = 1;
//		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
//		attributeDescriptions[1].offset = offsetof(RTSphere, center);*/
//
//		return attributeDescriptions;
//	}
//};
//#pragma pack(pop)

struct RTSphere {
	glm::vec2 position;
	glm::vec2 velocity;
	glm::vec4 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(RTSphere);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(RTSphere, position);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(RTSphere, color);

		return attributeDescriptions;
	}
};

class RTCamera {
public:
	~RTCamera();
	RTCamera(glm::vec3 position, glm::vec3 angle, glm::vec2 screenSize, float focusDistance, float fov, float zNear, float zFar)
		: pos(position), ang(angle), screenSize(screenSize), focusDistance(focusDistance), fov(fov), zNear(zNear), zFar(zFar) {
		calculateRTS();
	}

	void calculateRTS();

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
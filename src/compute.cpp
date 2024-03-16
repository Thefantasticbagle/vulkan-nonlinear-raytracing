#include "vulkanApplication.h"

/**
// *	Createws the compute pipeline.
// */
void VulkanApplication::createComputePipeline() {
	// Load compute shader
	auto compShaderCode = readFile("../resources/shaders/comp.spv");

	// Create shader module
	VkShaderModule  compShaderModule = createShaderModule(compShaderCode);

	// Assign pipeline stages
	VkPipelineShaderStageCreateInfo compShaderStageInfo{};
	compShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	compShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	compShaderStageInfo.module = compShaderModule;
	compShaderStageInfo.pName = "main"; //entrypoint
	compShaderStageInfo.pSpecializationInfo = nullptr; //can be used to set const values in shader before compilation!

	// Pipeline layout
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS)
		throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_PIPELINE::PIPELINE_LAYOUT_CREATION_FAILED");

	// Pipeline
    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.stage = compShaderStageInfo;

    //create compute pipeline
	if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS)
		throw std::runtime_error("ERR::VULKAN::CREATE_COMPUTE_PIPELINE::PIPELINE_CREATION_FAILED");

	vkDestroyShaderModule(device, compShaderModule, nullptr);
}
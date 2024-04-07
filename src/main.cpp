#define STB_IMAGE_IMPLEMENTATION

#include <stdexcept>
#include <iostream>

#include "vulkanApplication.h"

/**
 *	The main program.
 */
int main() {
    VulkanApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
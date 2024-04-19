# vulkan-compute
A non-linear ray tracing engine made using mainly Vulkan Compute and C++.

## Setup
### Download Vulkan SDK
Go to [LunarXchange](https://vulkan.lunarg.com) to download the version of Vulkan SDK appropriate for your system.

### Clone repository and fetch external dependencies
```sh
$ git clone https://github.com/Thefantasticbagle/vulkan-compute.git
$ cd vulkan-compute
$ git submodule init
$ git submodule update
```

### Build the project
For this step, cmake is required.
```sh
$ mkdir build
$ cd build
$ cmake -S ../ -B ./
```
Lastly, use Visual Studio to open `vulkan-compute.sln`, and build for Release!
(Building for Debug enables Validation layers and lowers performance)

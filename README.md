# Vulkan

This is the Vulkan repository for the AAU 9th semester pre-specialization project for the group dpw901e17.
This is one part of the repositories for this project.

The relevant code for this project is located in the Vulkan folder.

## Requirements

* 64-bit version of Windows
* Visual Studio 15 or later
* Graphics card compatible with Vulkan
* LunarG Vulkan SDK (must be located in "C:\VulkanSDK\1.0.65.0\")

## Run the tests

For running the test described in the article, build the project and run the "testrick.bat" batch file in the same folder as the executable.
The settings used were; 5x5x5 initial scene, set increments to 5, run for 30 seconds, and run 10 times.

To change between rendering the cube-model or the skull-model, change the defined macro in file "VulkanApplication.cpp" line 30 to TEST_USE_CUBE or TEST_USE_SKULL.

## Acknowledgements
https://vulkan-tutorial.com/ was used as a starting off point, when creating this repository. Thus there may be some similariites in program structure.

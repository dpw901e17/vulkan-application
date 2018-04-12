# Vulkan

This is the Vulkan repository for the application as described in "Comparing Direct3D 12 and Vulkan onPerformance and Programmability".
The relevant code is located in the Vulkan folder.
Associated repos are:
* https://github.com/dpw901e17/Direct3D12-Application
* https://github.com/dpw901e17/data

## Requirements

For running the program:
* 64-bit version of Windows 10
* Visual Studio 15 or later
* Graphics card compatible with Vulkan
* LunarG Vulkan SDK (must be located in "C:\VulkanSDK\1.0.65.0\")

For running the tests, we additionally require Open Hardware Monitor v. 0.7.1 beta to be installed at "%PROGAMFILES(x84)%\OpenHardwareMonitor".
It can be installed from http://openhardwaremonitor.org/.

## Run the tests
Tests can be executed with either a cube-model or a skull-model. You can define the one to use through the macro in file "VulkanApplication.cpp" line 30, which should be set to TEST_USE_CUBE or TEST_USE_SKULL.

The repository is set up for running two types of tests, which are executed through bat-scripts run in the same directory as the application executable. Folders containing generated data are named with a timestamp.
* "ThreadDataCollector.bat" is used for tests, where the number of threads for command submission is increased.
* "testrig.bat" is used for tests, where the number of objects to render is increased. When running the test, any of the 3 initials (b/c/m) can be used, as it is only used for labeling the files containing Open Hardware Monitor data. 

## Acknowledgements
https://vulkan-tutorial.com/ was used as a starting off point, when creating this repository. Thus there may be some similarities in program structure.

The skull model used for high-polygon testing was downloaded from https://free3d.com/3d-model/skull-human-anatomy-82445.html.

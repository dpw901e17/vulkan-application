@echo off
echo compiling!

C:/VulkanSDK/1.0.65.0/Bin32/glslangValidator.exe -V shader.vert
C:/VulkanSDK/1.0.65.0/Bin32/glslangValidator.exe -V shader.frag
C:/VulkanSDK/1.0.65.0/Bin32/glslangValidator.exe -V skull.frag -o skull.spv

xcopy /Y .\vert.spv ..\x64\Debug\shaders\vert.spv*
xcopy /Y .\frag.spv ..\x64\Debug\shaders\frag.spv*
xcopy /Y .\skull.spv ..\x64\Debug\shaders\skull.spv*
xcopy /Y .\vert.spv ..\x64\Release\shaders\vert.spv*
xcopy /Y .\frag.spv ..\x64\Release\shaders\frag.spv*
xcopy /Y .\skull.spv ..\x64\Release\shaders\skull.spv*
xcopy /Y .\texture.png ..\x64\Debug\textures\texture.png*
xcopy /Y .\texture.png ..\x64\Release\textures\texture.png*
xcopy /Y .\record.bat ..\x64\Debug\record.bat*
xcopy /Y .\record.bat ..\x64\Release\record.bat*
xcopy /Y .\AutoRecord.bat ..\x64\Debug\AutoRecord.bat*
xcopy /Y .\AutoRecord.bat ..\x64\Release\AutoRecord.bat*
xcopy /Y .\testrick.bat ..\x64\Debug\testrick.bat*
xcopy /Y .\testrick.bat ..\x64\Release\testrick.bat*
xcopy /Y .\ThreadDataCollector.bat ..\x64\Debug\ThreadDataCollector.bat*
xcopy /Y .\ThreadDataCollector.bat ..\x64\Release\ThreadDataCollector.bat*

xcopy /Y CollectWinPerfmonDataCfg.cfg ..\x64\Debug\CollectWinPerfmonDataCfg.cfg*
xcopy /Y CollectWinPerfmonDataCfg.cfg ..\x64\Release\CollectWinPerfmonDataCfg.cfg*

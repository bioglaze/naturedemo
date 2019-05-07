md build
%VULKAN_SDK%/bin/glslangValidator -D -V -S vert -e mainVS src/water.hlsl -o build/water_vs.spv
%VULKAN_SDK%/bin/glslangValidator -D -V -S frag -e mainFS src/water.hlsl -o build/water_fs.spv
%VULKAN_SDK%/bin/glslangValidator -D -V -S vert -e mainVS src/sky.hlsl -o build/sky_vs.spv
%VULKAN_SDK%/bin/glslangValidator -D -V -S frag -e mainFS src/sky.hlsl -o build/sky_fs.spv
pause


md build
%VULKAN_SDK%/bin/glslangValidator -D -V -S vert -e waterVS src/water.hlsl -o build/water_vs.spv
%VULKAN_SDK%/bin/glslangValidator -D -V -S frag -e waterFS src/water.hlsl -o build/water_fs.spv
pause


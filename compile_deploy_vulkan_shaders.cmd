md build
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainVS -all-resources-bound -T vs_6_0 src\water.hlsl -Fo build\water_vs.spv
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainFS -all-resources-bound -T ps_6_0 src\water.hlsl -Fo build\water_fs.spv
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainVS -all-resources-bound -T vs_6_0 src\sky.hlsl -Fo build\sky_vs.spv
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainFS -all-resources-bound -T ps_6_0 src\sky.hlsl -Fo build\sky_fs.spv
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainVS -all-resources-bound -T vs_6_0 src\ground.hlsl -Fo build\ground_vs.spv
%VULKAN_SDK%\bin\dxc -Ges -spirv -fspv-target-env=vulkan1.1 -E mainFS -all-resources-bound -T ps_6_0 src\ground.hlsl -Fo build\ground_fs.spv
pause


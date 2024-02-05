workspace "GameEngine"
    system "windows"
    architecture "x64"
    startproject "Engine"

    configurations
    {
        "Debug",
        "Profile",
        "Release"
    }


includeDirs = {}


include "3dparty/premake5.lua"


project "Engine"
    filter {}

    kind "WindowedApp"

    location "Engine"

    language "C++"
    cppdialect "C++20"

    staticruntime "On"
    systemversion "latest"

    filter "Debug"
        objdir("%{wks.location}/Bin/Debug/Intermediate/%{prj.name}")
    filter "Profile"
        objdir("%{wks.location}/Bin/Profile/Intermediate/%{prj.name}")
    filter "Release"
        objdir("%{wks.location}/Bin/Release/Intermediate/%{prj.name}")
    filter {}

    targetdir("%{wks.location}/Bin/")
    debugdir("")

    files
    {
        path.getabsolute("%{prj.location}/Code/**.h", ""),
        "%{prj.location}/Code/**.h",
        "%{prj.location}/Code/**.cpp",
        "%{wks.location}/3dparty/ImGuizmo/ImGuizmo.cpp"
    }

    -- Include directories
        -- Engine files
    includedirs "%{prj.location}/Code"
        -- Vulkan
    includedirs "$(VK_SDK_PATH)/Include"
    includedirs "$(VULKAN_SDK)/Include"
        -- ThirdParty
    thirdpartyDir = "%{wks.location}/3dparty/"
    includedirs(thirdpartyDir .. includeDirs["glm"])
    includedirs(thirdpartyDir .. includeDirs["ImGui"])
    includedirs(thirdpartyDir .. includeDirs["spdlog"])
    includedirs(thirdpartyDir .. includeDirs["tracy"])
    includedirs(thirdpartyDir .. includeDirs["VulkanMemoryAllocator"])
    includedirs(thirdpartyDir .. includeDirs["assimp"])
    includedirs(thirdpartyDir .. includeDirs["stb"])
    includedirs(thirdpartyDir .. includeDirs["entt"])
    includedirs(thirdpartyDir .. includeDirs["ImGuizmo"])
    includedirs(thirdpartyDir .. includeDirs["meshoptimizer"])

    dependson { "imgui", "meshoptimizer" }

    -- Linking
    filter "Debug"
        libdirs("%{wks.location}/Bin/Debug")
    filter "Profile"
        libdirs("%{wks.location}/Bin/Profile")
    filter "Release"
        libdirs("%{wks.location}/Bin/Release")
    filter {}

    libdirs { "$(VK_SDK_PATH)/Lib", "$(VULKAN_SDK)/Lib" }
    -- winapi
    links { "Shcore.lib" }
    -- subprojects
    links { "vulkan-1.lib", "imgui.lib", "meshoptimizer", "dxcompiler.lib" }
    filter "Debug or Profile"
        links "TracyClient.lib"
    filter {}

    filter "Debug"
        links(thirdpartyDir .. "assimp/Debug/assimp.lib")
        links(thirdpartyDir .. "assimp/Debug/zlib.lib")
    filter "Profile or Release"
        links(thirdpartyDir .. "assimp/Release/assimp.lib")
        links(thirdpartyDir .. "assimp/Release/zlib.lib")
    filter {}
    
    flags "MultiProcessorCompile"
    editandcontinue "Off"

    -- Defines
    defines { "SPDLOG_NO_EXCEPTIONS", "ENTT_NOEXCEPTION", "LOG_ENABLE", "GLM_FORCE_INTRINSICS", "GLM_FORCE_INLINE", "GLM_ENABLE_EXPERIMENTAL", "GLM_FORCE_DEPTH_ZERO_TO_ONE", "GLM_FORCE_RADIANS", "GLM_FORCE_DEFAULT_ALIGNED_GENTYPES", "NOMINMAX" }
    filter "Debug"
        defines { "DEBUG_BUILD", "VULKAN_VALIDATION_ENABLE" }
    filter "Debug or Profile"
        defines { "TRACY_ENABLE", "PROFILE_ENABLE", "GPU_PROFILE_ENABLE", "ASSERT_ENABLE" }
    filter "Release"
        defines { "RELEASE_BUILD" }
    
    -- Optimization and Debug
    filter "Debug"
        optimize "Off"
        symbols "Full"
    filter "Debug or Profile"
        dependson "TracyClient"
    filter "Profile or Release"
        optimize "Full"
        inlining "Auto"
        omitframepointer "On"
        symbols "On"
        flags { "LinkTimeOptimization" }
    filter "Release"
        rtti "Off"
        defines { "ENTT_DISABLE_ASSERT" }
    filter {}
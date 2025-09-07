cmake_minimum_required(VERSION 3.15)

project(player_register CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Enable filesystem linking for Windows
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
    # Note: WIN32_LEAN_AND_MEAN and NOMINMAX are already defined by Endstone
    # No need to define them here to avoid redefinition warnings
    # For MSVC, filesystem is part of the standard library, no separate lib needed
    set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES}")
endif()

# Link filesystem library for different platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
elseif(APPLE)
    # macOS doesn't need special linking for filesystem
    target_compile_options(${PROJECT_NAME} PRIVATE -std=c++20)
endif()

include(FetchContent)
FetchContent_Declare(
        endstone
        GIT_REPOSITORY https://github.com/EndstoneMC/endstone.git
        GIT_TAG v0.10 # TODO: CHANGE ME to the actual version you are using
)
FetchContent_MakeAvailable(endstone)

# Add all source files
set(SOURCES
    src/player_register.cpp
    src/player_manager.cpp
    src/account_manager.cpp
    src/database.cpp
    src/config.cpp
    src/sha256.cpp
    src/player_register_listener.cpp
)

endstone_add_plugin(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link filesystem library for different platforms
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
elseif(APPLE)
    # macOS doesn't need special linking for filesystem
    target_compile_options(${PROJECT_NAME} PRIVATE -std=c++20)
endif()

# Let endstone_add_plugin handle all necessary linking
# No need to specify additional libraries manually

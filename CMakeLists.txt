cmake_minimum_required(VERSION 3.15)

project(player_register CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    # Enable filesystem linking for Windows
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_WIN32_WINNT=0x0601")
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
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
)

endstone_add_plugin(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Add nlohmann_json include directory
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Link with required libraries
# nlohmann_json is header-only, no linking needed

# Windows-specific filesystem linking
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE stdc++fs)
endif()

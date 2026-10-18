# Xbox input uses XInput supplied with Windows 8 and later. The generic MinGW
# xinput library imports XINPUT1_3, which requires the legacy DirectX package.
add_library(xbox_input STATIC xinput_device.c)
target_include_directories(xbox_input PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

if(WIN32)
    # Build the pinned controller backend into DogGame; no SDL DLL/runtime install.
    include(FetchContent)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST OFF CACHE BOOL "" FORCE)
    set(SDL_TESTS OFF CACHE BOOL "" FORCE)
    set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
    set(SDL_VIDEO OFF CACHE BOOL "" FORCE)
    set(SDL_RENDER OFF CACHE BOOL "" FORCE)
    set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
    set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(doggame_sdl
        URL https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-2.32.10.tar.gz
        URL_HASH SHA256=5f5993c530f084535c65a6879e9b26ad441169b3e25d789d83287040a9ca5165
    )
    FetchContent_MakeAvailable(doggame_sdl)
    target_sources(xbox_input PRIVATE host_gamepad.c)
    target_link_libraries(xbox_input PUBLIC SDL2::SDL2-static)
    if(MINGW)
        target_link_libraries(xbox_input PUBLIC platform xinput1_4)
    else()
        target_link_libraries(xbox_input PUBLIC platform xinput)
    endif()
else()
    find_package(SDL2 REQUIRED)
    target_link_libraries(xbox_input PUBLIC platform SDL2::SDL2)
endif()

if(MSVC)
    target_compile_options(xbox_input PRIVATE /W4 /O2)
else()
    target_compile_options(xbox_input PRIVATE -Wall -Wextra -O2)
endif()

option(DOGGAME_BUILD_CONTROLLER_TESTS "Build the virtual controller boundary test" OFF)
if(WIN32 AND DOGGAME_BUILD_CONTROLLER_TESTS)
    add_executable(doggame_controller_test "${CMAKE_SOURCE_DIR}/tests/host_gamepad_test.c")
    target_link_libraries(doggame_controller_test PRIVATE xbox_input)
    if(MINGW)
        target_link_options(doggame_controller_test PRIVATE -static)
    endif()
endif()

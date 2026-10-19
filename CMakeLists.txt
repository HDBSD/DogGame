cmake_minimum_required(VERSION 3.20)

# DogGame - Windows host for the recompiled Xbox title.

project(DogGame C CXX RC)
set(CMAKE_CXX_STANDARD 11)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
option(PREVIEW_BUILD_LOCKING "Require Auth0 Discord whitelist authorization at startup" OFF)
option(PREVIEW_VULKAN_FPSGRAPH "Default to Vulkan and the debug FPS graph in a locked preview" OFF)
if(PREVIEW_VULKAN_FPSGRAPH AND NOT PREVIEW_BUILD_LOCKING)
    message(FATAL_ERROR "PREVIEW_VULKAN_FPSGRAPH requires PREVIEW_BUILD_LOCKING=ON")
endif()
if(PREVIEW_VULKAN_FPSGRAPH)
    set_property(SOURCE src/debug_options.c APPEND PROPERTY
        COMPILE_DEFINITIONS PREVIEW_VULKAN_FPSGRAPH=1)
endif()

# Common definitions
add_compile_definitions(
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

# ── xboxrecomp toolkit ──────────────────────────────────────────
#
# Option A: xboxrecomp as a subdirectory (simplest)
#   Place xboxrecomp/ next to your game project and use a relative path.
#
# Option B: find_package (if you install xboxrecomp system-wide)
#   find_package(xboxrecomp REQUIRED)
#
# Adjust this path to point to your xboxrecomp clone:
set(XBOXRECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../xboxrecomp" CACHE PATH
    "Path to the xboxrecomp toolkit root directory")

# Add xboxrecomp as a subdirectory -- this builds all 6 runtime libraries
# and provides the 'xboxrecomp' umbrella target to link against.
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(DOGGAME_UPSTREAM_DIR "${XBOXRECOMP_DIR}")
set(DOGGAME_RUNTIME_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build/runtime/xboxrecomp")
execute_process(
    COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_runtime.py"
        --upstream "${DOGGAME_UPSTREAM_DIR}" --output "${DOGGAME_RUNTIME_DIR}"
    RESULT_VARIABLE DOGGAME_RUNTIME_RESULT
)
if(NOT DOGGAME_RUNTIME_RESULT EQUAL 0)
    message(FATAL_ERROR "Could not prepare the pinned DogGame runtime")
endif()
file(GLOB_RECURSE DOGGAME_RUNTIME_OVERRIDES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/runtime_overrides/*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    ${DOGGAME_RUNTIME_OVERRIDES}
    "${CMAKE_CURRENT_SOURCE_DIR}/config/xboxrecomp.lock.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/prepare_runtime.py")
# The cache continues to identify pristine upstream; consumers use the assembled copy.
set(XBOXRECOMP_DIR "${DOGGAME_RUNTIME_DIR}")
add_subdirectory("${XBOXRECOMP_DIR}" "${CMAKE_BINARY_DIR}/xboxrecomp")

# ── Recompiled game executable ──────────────────────────────────

# Collect generated recompiled code.
# The recomp pipeline outputs files into src/recomp/gen/.
# With --split 1000, you get gen_0000.c, gen_0001.c, etc.
# CONFIGURE_DEPENDS so re-running the recompiler picks up added/removed files
# without a manual re-configure. A plain GLOB is evaluated once at configure
# time, so a newly generated file (an extra chunk, recomp_stubs_unresolved.c)
# is silently left out of the build and shows up as unresolved externals.
file(GLOB RECOMP_GEN_SOURCES CONFIGURE_DEPENDS "src/recomp/gen/*.c")

# Fresh generation emits the recompiler's supported sub_ADDRESS_gen wrappers.
# Older generated trees still contain the original definitions: rename those
# at compile time, without editing generated files. Locate by entry point;
# excluding maintained routines changes chunk boundaries in a clean setup.
foreach(address IN ITEMS 00152FEC 001000B0)
    set(delegate_count 0)
    foreach(source IN LISTS RECOMP_GEN_SOURCES)
        file(STRINGS "${source}" definition LIMIT_COUNT 1
            REGEX "^void sub_${address}(_gen)?\\(void\\)[ \t]*$")
        if(definition)
            math(EXPR delegate_count "${delegate_count} + 1")
            if(definition MATCHES "^void sub_${address}\\(")
                set_property(SOURCE "${source}" APPEND PROPERTY
                    COMPILE_DEFINITIONS sub_${address}=sub_${address}_gen)
            endif()
        endif()
    endforeach()
    if(NOT delegate_count EQUAL 1)
        message(FATAL_ERROR "Expected one generated delegate for sub_${address}; regenerate with scripts/generate_game.py")
    endif()
endforeach()

# Core game sources
set(GAME_SOURCES
    src/api/title_guest_movie_frame.c
    src/overrides/movie_frame.c
    src/main.c
    src/recomp_manual.c
    src/title_overrides.c
    src/debug_options.c
    src/title_debug.c
    src/title_debug_geometry.c
    src/title_debug_overrides.c
    src/title_display.c
    src/api/title_guest_menu.c
    src/api/title_guest_ui.c
    src/api/title_guest_text.c
    src/api/title_guest_diagnostics.c
    src/api/title_guest_debug.c
    src/api/title_guest_routines.c
    src/api/title_guest_symbols.c
    src/api/title_guest_calls.c
    src/api/title_guest_player.c
    src/title_player_flight.c
    src/api/title_guest_geometry.c
    src/api/title_guest_state.c
    src/api/title_host_runtime.c
    src/title_prompts.c
    src/api/title_saves.c
    # Add game-specific sources here, e.g.:
    # src/asset_loader.c
    # src/custom_renderer.c
)

# The recomp pipeline also generates a dispatch header and function declarations.
# These are included by the generated .c files:
#   src/recomp/gen/recomp_funcs.h    - extern declarations for all recompiled functions
#   src/recomp/recomp_dispatch.h     - binary search dispatch table
#   src/recomp/recomp_types.h        - register model, ICALL macros, MEM helpers

add_executable(${PROJECT_NAME} WIN32
    Resources/DogGame.rc
    ${GAME_SOURCES}
    ${RECOMP_GEN_SOURCES}
)
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME DogGame)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/SDL-LICENSE.txt"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/SDL-LICENSE.txt")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/runtime_overrides/src/kernel/vulkan_headers/LICENSE.txt"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/Vulkan-Headers-LICENSE.txt")
if(PREVIEW_BUILD_LOCKING)
    target_sources(${PROJECT_NAME} PRIVATE src/preview_auth.c src/api/preview_auth_identity.c)
    target_compile_definitions(${PROJECT_NAME} PRIVATE PREVIEW_BUILD_LOCKING=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE winhttp ws2_32 bcrypt)
endif()

# Link against the xboxrecomp umbrella target (all 6 runtime libraries)
target_link_libraries(${PROJECT_NAME} PRIVATE xboxrecomp)
if(MINGW)
    # Keep the installed game independent of the temporary compiler's C++ DLLs.
    target_link_options(${PROJECT_NAME} PRIVATE -static)
endif()

# Include paths for generated recomp headers
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/api
    ${CMAKE_CURRENT_SOURCE_DIR}/src/recomp
    ${XBOXRECOMP_DIR}/include
)

# Windows SDK libraries needed at link time
if(MINGW)
    set(DOGGAME_XINPUT_LIBRARY xinput1_4)
else()
    # The Microsoft SDK names its modern XInput import library xinput.lib.
    set(DOGGAME_XINPUT_LIBRARY xinput)
endif()
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    dxguid
    ${DOGGAME_XINPUT_LIBRARY}
    winmm
    # Lets the fault handler resolve the host RIP back to the generated
    # function's symbol, i.e. to the guest function that faulted.
    dbghelp
    shell32
)

# MSVC-specific: large address aware, increase section limit for big generated files
if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        /LARGEADDRESSAWARE
        /DEBUG
    )
    # The generated code files can be very large. Increase object file section limit.
    target_compile_options(${PROJECT_NAME} PRIVATE /bigobj /Zi /MP4)
endif()

# ── Game-specific additions ─────────────────────────────────────
#
# Add any game-specific libraries, loaders, or custom renderers below.
# Examples:
#
# # Custom texture loader for your game's format
# target_sources(${PROJECT_NAME} PRIVATE src/texture_loader.c)
#
# # If your game uses RenderWare, you may need a RW bridge layer
# target_sources(${PROJECT_NAME} PRIVATE src/rw_bridge.c)
#
# # Additional compile definitions for debugging
# target_compile_definitions(${PROJECT_NAME} PRIVATE
#     ENABLE_ICALL_TRACE=1
#     VERBOSE_KERNEL_LOG=1
# )

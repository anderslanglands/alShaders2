cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project(alShaders2)

set(ALS_MAJOR_VERSION 2)
set(ALS_MINOR_VERSION 0)
set(ALS_PATCH_VERSION 0b2)
set(ALS_VERSION "${ALS_MAJOR_VERSION}.${ALS_MINOR_VERSION}.${ALS_PATCH_VERSION}")

set(CMAKE_VERBOSE_MAKEFILE FALSE)
set(CMAKE_SKIP_RPATH TRUE)

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

set(CMAKE_CXX_STANDARD 11)

# Statically link stdc++ to avoid GLILBC version clashes
set(CMAKE_SHARED_LINKER_FLAGS "-static-libstdc++")

# Compiler flags
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    # Disable some of the bullshit warnings MSVC wants to barf
    add_definitions( "-W3 -D_CRT_SECURE_NO_WARNINGS -wd4005 -wd4996 -wd4305 -wd4244 -nologo" )
else()
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fvisibility=hidden -DNDEBUG")
    # set(CMAKE_CXX_FLAGS_DEBUG "-fsanitize=address -fno-omit-frame-pointer")
endif()

# check if we have a local cmake include file and include it if we do
# this is useful for setting our arnold location as an alternative to
# environment variables
if(EXISTS ${CMAKE_SOURCE_DIR}/local.cmake)
    message(STATUS "Using local.cmake")
    include(${CMAKE_SOURCE_DIR}/local.cmake)
endif()

# Find Arnold SDK
find_package(Arnold REQUIRED)
include_directories(${ARNOLD_INCLUDE_DIR})
link_directories(${ARNOLD_LIBRARY_DIR})

if (NOT DEFINED INSTALL_DIR)
    if (DEFINED INSTALL_ROOT)
        set(INSTALL_DIR "${INSTALL_ROOT}/${ALS_VERSION}/ai${ARNOLD_VERSION}")
        message("INSTALL_ROOT defined. Adding versions automatically:\n\t${INSTALL_DIR}")
    else()
        set(INSTALL_DIR "${CMAKE_BINARY_DIR}/dist/${ALS_VERSION}/ai${ARNOLD_VERSION}")
        message("INSTALL_DIR not defined. Defaulting to:\n\t${INSTALL_DIR}")
    endif()
else()
    message("Installing to:\n\t${INSTALL_DIR}")
endif()

# Set up installation paths
set(DSO_INSTALL_DIR ${INSTALL_DIR}/bin)
set(MTD_INSTALL_DIR ${INSTALL_DIR}/bin)
set(AE_INSTALL_DIR ${INSTALL_DIR}/ae)
set(AEXML_INSTALL_DIR ${INSTALL_DIR}/aexml)
set(NEXML_INSTALL_DIR ${INSTALL_DIR}/aexml)
set(SPDL_INSTALL_DIR ${INSTALL_DIR}/spdl)
set(ARGS_INSTALL_DIR ${INSTALL_DIR}/Args)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/docs)

message("Shader binaries will be installed to:\n\t${DSO_INSTALL_DIR}")
message("Shader MTD will be installed to: \n\t${MTD_INSTALL_DIR}")
message("AETemplates will be installed to: \n\t${AE_INSTALL_DIR}")
message("SPDL will be installed to: \n\t${SPDL_INSTALL_DIR}")
message("Katana Args will be installed to: \n\t${ARGS_INSTALL_DIR}")

# Include common files
include_directories(common)
include_directories(${CMAKE_SOURCE_DIR})

# Set the list of subdirectories to recurse into to find stuff to build
set(SUBDIRECTORIES 
    cryptomatte
    common
)

# define shader build function
function(build_shader)
    foreach(SHADER ${ARGV})
        set(SRC ${SHADER}.cpp ../common/alUtil.cpp ../common/Color.cpp)
        set(UI ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}.ui)
        set(MTD ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.mtd)
        set(AE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}Template.py)
        set(AEXML ${CMAKE_CURRENT_BINARY_DIR}/AE${SHADER}Template.xml)
        set(NEXML ${CMAKE_CURRENT_BINARY_DIR}/NE${SHADER}Template.xml)
        set(SPDL ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.spdl)
        set(KARGS ${CMAKE_CURRENT_BINARY_DIR}/${SHADER}.args)
        set(C4DRES ${CMAKE_CURRENT_BINARY_DIR}/C4DtoA)
        set(HTML ${CMAKE_SOURCE_DIR}/docs/${SHADER}.html)

        add_library(${SHADER} SHARED ${SRC})

        target_link_libraries(${SHADER} ai)
        set_target_properties(${SHADER} PROPERTIES PREFIX "")

        add_custom_command(OUTPUT ${MTD} COMMAND python ARGS ${CMAKE_SOURCE_DIR}/uigen.py ${UI} ${MTD} ${AE} ${AEXML} ${NEXML} ${SPDL} ${KARGS} ${CMAKE_CURRENT_BINARY_DIR} ${HTML} DEPENDS ${UI})
        add_custom_target(${SHADER}UI ALL DEPENDS ${MTD})

        install(TARGETS ${SHADER} DESTINATION ${DSO_INSTALL_DIR})
        install(FILES ${MTD} DESTINATION ${MTD_INSTALL_DIR})
        install(FILES ${AE} DESTINATION ${AE_INSTALL_DIR})
        install(FILES ${AEXML} DESTINATION ${AEXML_INSTALL_DIR})
        install(FILES ${NEXML} DESTINATION ${NEXML_INSTALL_DIR})
        install(FILES ${SPDL} DESTINATION ${SPDL_INSTALL_DIR})
        install(FILES ${KARGS} DESTINATION ${ARGS_INSTALL_DIR})
        install(FILES ${KARGS} DESTINATION ${ARGS_INSTALL_DIR})
        install(DIRECTORY ${C4DRES} DESTINATION ${INSTALL_DIR})
    endforeach()
endfunction(build_shader)

# loop over subdirectories
foreach(SUBDIR ${SUBDIRECTORIES})
    add_subdirectory(${SUBDIR})
endforeach()

# configure packaging script
configure_file(
    "${PROJECT_SOURCE_DIR}/package.in.py"
    "${PROJECT_SOURCE_DIR}/package.py"
)

# add top-level files
INSTALL(FILES README INSTALL DESTINATION ${INSTALL_DIR})

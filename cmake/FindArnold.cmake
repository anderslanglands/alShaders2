# FindArnold.cmake
SET(Arnold_FOUND FALSE)

# If there's no arnold version explicitly defined
if (NOT DEFINED ARNOLD_VERSION)
	# Try to grab it from the env
	if (DEFINED ENV{ARNOLD_VERSION})
		set(ARNOLD_VERSION $ENV{ARNOLD_VERSION})
	endif()
endif()

# if local.cmake hasn't defined it already...
if (NOT DEFINED ARNOLD_ROOT)
	# if the environment variable isn't set...
	if (NOT DEFINED ENV{ARNOLD_ROOT})
		message(FATAL_ERROR "Could not find Arnold SDK. Please set ARNOLD_ROOT to point to the root directory of the Arnold SDK. This is the directory containing bin, include etc. Either add a set() statement to a local.cmake file in the source directory, or define it as an environment variable prior to building.")
	endif()
	set(ARNOLD_ROOT $ENV{ARNOLD_ROOT})
endif()
message(STATUS "ARNOLD_ROOT is ${ARNOLD_ROOT}")

# Verify that ARNOLD_ROOT points to a valid directory
if (NOT IS_DIRECTORY "${ARNOLD_ROOT}")
	message(FATAL_ERROR "ARNOLD_ROOT does not point to a valid directory. Please set it to point to the root directory of the Arnold SDK. This is the directory containing bin, include etc.")
endif()

# Find the Arnold include directory
find_path(ARNOLD_INCLUDE_DIR
	NAMES ai.h
	PATHS ${ARNOLD_ROOT}/include
)
message(STATUS "ARNOLD_INCLUDE_DIR is ${ARNOLD_INCLUDE_DIR}")

# Find the Arnold library directory
# We link against the lib stub on Windows, because Windows is special.
set(ARNOLD_LIBDIR "bin")
if (${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(ARNOLD_LIBDIR "lib")
	find_path(ARNOLD_LIBRARY_DIR
		NAMES ai.lib
		PATHS ${ARNOLD_ROOT}/${ARNOLD_LIBDIR}
	)
else()
	find_path(ARNOLD_LIBRARY_DIR
		NAMES ${CMAKE_SHARED_LIBRARY_PREFIX}ai${CMAKE_SHARED_LIBRARY_SUFFIX}
		PATHS ${ARNOLD_ROOT}/${ARNOLD_LIBDIR}
	)
endif()


message(STATUS "ARNOLD_LIBRARY_DIR is ${ARNOLD_LIBRARY_DIR}")

if (ARNOLD_INCLUDE_DIR AND ARNOLD_LIBRARY_DIR)
	set(Arnold_FOUND TRUE)
endif()
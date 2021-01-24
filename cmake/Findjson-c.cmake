set(BUILD_SHARED_LIBS OFF CACHE BOOL "Default to building shared libraries" FORCE)
add_subdirectory(${cdiscord_SOURCE_DIR}/lib/json-c)

set(JSON-C_LIBRARY json-c)
set(JSON-C_INCLUDE_DIR ${cdiscord_SOURCE_DIR}/lib/json-c)
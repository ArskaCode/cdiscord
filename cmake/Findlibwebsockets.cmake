set(LWS_WITHOUT_EXTENSIONS OFF CACHE BOOL "Don't compile with extensions" FORCE)
set(LWS_WITH_SHARED OFF CACHE BOOL "Build the shared version of the library" FORCE)
set(LWS_STATIC_PIC ON CACHE BOOL "Build the static version of the library with position-independent code" FORCE)
add_subdirectory(${cdiscord_SOURCE_DIR}/lib/libwebsockets)

set(LIBWEBSOCKETS_LIBRARY websockets)
set(LIBWEBSOCKETS_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/lib/libwebsockets/include)

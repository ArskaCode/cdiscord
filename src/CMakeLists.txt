
add_library(cdiscord SHARED
        cdiscord.c
        gateway.c gateway.h

        ${cdiscord_SOURCE_DIR}/include/cdiscord.h internal.h dc_api.c dc_api.h)

target_include_directories(cdiscord PUBLIC ${cdiscord_SOURCE_DIR}/include)

target_include_directories(cdiscord PRIVATE ${JSON-C_INCLUDE_DIR} ${LIBWEBSOCKETS_INCLUDE_DIR})

target_link_libraries(cdiscord PRIVATE ${JSON-C_LIBRARY} ${LIBWEBSOCKETS_LIBRARY} curl bsd)
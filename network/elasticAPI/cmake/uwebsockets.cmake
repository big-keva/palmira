set(PROJECT_NAME usockets)

set(USOCKETS_SOURCES
    ${USOCKETS_DIR}/src/bsd.c
    ${USOCKETS_DIR}/src/context.c
    ${USOCKETS_DIR}/src/loop.c
    ${USOCKETS_DIR}/src/socket.c
    ${USOCKETS_DIR}/src/eventing/epoll_kqueue.c
)

if (ELASTIC_ENABLE_SSL)
    list(APPEND USOCKETS_SOURCES
        ${USOCKETS_DIR}/src/crypto/openssl.c
        ${USOCKETS_DIR}/src/crypto/sni_tree.cpp
    )
endif (ELASTIC_ENABLE_SSL)

add_library(${PROJECT_NAME} STATIC ${USOCKETS_SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${PROJECT_NAME} PUBLIC ${USOCKETS_DIR}/src)

if (ELASTIC_ENABLE_SSL)
    target_compile_definitions(${PROJECT_NAME} PUBLIC LIBUS_USE_OPENSSL)

    target_link_libraries(${PROJECT_NAME} PUBLIC OpenSSL::SSL OpenSSL::Crypto)
else ()
    target_compile_definitions(${PROJECT_NAME} PUBLIC LIBUS_NO_SSL)
endif (ELASTIC_ENABLE_SSL)

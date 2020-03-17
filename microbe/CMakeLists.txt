file(GLOB_RECURSE microbe_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.h"
)

add_executable(microbe ${microbe_SRCS})

target_precompile_headers(microbe
    PUBLIC
        pch.hpp
)

target_link_libraries(microbe
    KF5::KDELibs4Support
    KF5::CoreAddons
    Qt5::Core
    pthread
)

install(TARGETS microbe ${INSTALL_TARGETS_DEFAULT_ARGS})

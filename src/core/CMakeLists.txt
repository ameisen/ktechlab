SET(core_STAT_SRCS

)

list(APPEND core_KCFG_SRCS
    ktlconfig.kcfgc
)

kconfig_add_kcfg_files(core_STAT_SRCS ${core_KCFG_SRCS})

add_library(core STATIC ${core_STAT_SRCS})

target_precompile_headers(core
    PUBLIC
        pch.hpp
)

########### install files ###############

install(FILES ktechlab.kcfg DESTINATION ${KCFG_INSTALL_DIR})

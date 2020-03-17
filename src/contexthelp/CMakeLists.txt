file(GLOB contexthelp_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "help-*"
    "*.png"
)

install(FILES
	${contexthelp_SRCS}
	DESTINATION ${DATA_INSTALL_DIR}/ktechlab/contexthelp)

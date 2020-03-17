include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}  # needed for subdirs
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_BINARY_DIR}/core  # for the kcfg file
    ${CMAKE_CURRENT_SOURCE_DIR}/drawparts
    ${CMAKE_CURRENT_SOURCE_DIR}/electronics
    ${CMAKE_CURRENT_SOURCE_DIR}/electronics/components
    ${CMAKE_CURRENT_SOURCE_DIR}/electronics/simulation
    ${CMAKE_CURRENT_SOURCE_DIR}/flowparts
    ${CMAKE_CURRENT_SOURCE_DIR}/gui
    ${CMAKE_CURRENT_BINARY_DIR}/gui  # for ui-generated files
    ${CMAKE_CURRENT_SOURCE_DIR}/gui/itemeditor
    ${CMAKE_CURRENT_SOURCE_DIR}/languages
    ${CMAKE_CURRENT_SOURCE_DIR}/mechanics
    ${CMAKE_CURRENT_SOURCE_DIR}/micro
    ${CMAKE_CURRENT_SOURCE_DIR}/ktlqt3support
    ${KDE4_INCLUDES}
    ${QT_INCLUDES}
)

if(GPSim_FOUND)
    include_directories(${GPSim_INCLUDE_DIRS})
    set(CMAKE_CXX_FLAGS ${KDE4_ENABLE_EXCEPTIONS})
endif()

add_subdirectory(core)
add_subdirectory(gui)
add_subdirectory(flowparts)
add_subdirectory(micro)
add_subdirectory(mechanics)
add_subdirectory(electronics)
add_subdirectory(languages)
add_subdirectory(drawparts)
add_subdirectory(contexthelp)
add_subdirectory(math)
add_subdirectory(ktlqt3support)

########### next target ###############

list(APPEND core_KCFG_SRCS
    core/ktlconfig.kcfgc
)
kconfig_add_kcfg_files(core_STAT_SRCS ${core_KCFG_SRCS})

file(GLOB_RECURSE gui_UI RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.ui"
)

ki18n_wrap_ui(gui_STAT_SRCS ${gui_UI})

file(GLOB_RECURSE ktechlab_SRCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.cpp"
    "*.hpp"
    "*.h"
)

list(APPEND ktechlab_SRCS
    ${core_STAT_SRCS}
    ${gui_STAT_SRCS}
)

list(REMOVE_ITEM ktechlab_SRCS
    core/main.cpp
    electronics/models/utils/spice-to-nice.cpp
)

list(APPEND ktechlab_DCOP_SKEL_SRCS
    docmanageriface.h
    viewiface.h
    documentiface.h
)

add_executable(ktechlab
    core/main.cpp
    ${ktechlab_SRCS}
)

target_precompile_headers(ktechlab
    PUBLIC
        pch.hpp
)

target_link_libraries( ktechlab
	KF5::TextEditor
	KF5::KHtml
	KF5::Parts
    KF5::KDELibs4Support
    KF5::WidgetsAddons
	${KDE4_KHTML_LIBRARY} # khtml
	${GPSIM_LIBRARY}
	${QT_QTXML_LIBRARY}
	${QT_QTGUI_LIBRARY} # QtGui
	${QT_QTCORE_LIBRARY} # QtCore
	KF5::ConfigCore
	KF5::ConfigGui
	KF5::CoreAddons
	KF5::IconThemes
	KF5::KIOCore
	KF5::KHtml
	KF5::Parts
	KF5::XmlGui
	KF5::TextEditor
    KF5::WidgetsAddons
    KF5::KDELibs4Support
)

if(GPSim_FOUND)
    target_link_libraries(ktechlab ${GPSim_LIBRARIES})
endif()

install(TARGETS ktechlab ${INSTALL_TARGETS_DEFAULT_ARGS})

message(STATUS "include_dir begin")
get_property(dirs TARGET KF5::TextEditor PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "KF5::TextEditor include_dir='${dir}'")
endforeach()
message(STATUS "include_dir end")

# for helping testing

if (BUILD_TESTS)
    add_library(test_ktechlab STATIC ${ktechlab_SRCS})

    target_link_libraries(test_ktechlab
        KF5::TextEditor
        KF5::KHtml
        KF5::Parts
        KF5::KDELibs4Support
        KF5::WidgetsAddons
    )
endif()

########### install files ###############

install(FILES
    microbe.xml
    DESTINATION ${DATA_INSTALL_DIR}/katepart5/syntax
)

file(GLOB ktechlab_RCS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "*.rc"
)

install(FILES
    ${ktechlab_RCS}
    DESTINATION ${KXMLGUI_INSTALL_DIR}/ktechlab
)
install(FILES
    org.kde.ktechlab.desktop
    DESTINATION ${XDG_APPS_INSTALL_DIR}
)
install(FILES
    ktechlab_mime.xml
    DESTINATION ${XDG_MIME_INSTALL_DIR}
)
install(FILES
    org.kde.ktechlab.appdata.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)

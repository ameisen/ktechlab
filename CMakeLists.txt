set(KTECHLAB_DEVELOPER_BUILD ON)

project(ktechlab)
set(VERSION 0.60.0)

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(BUILD_EXAMPLES OFF)
set(BUILD_DOCS OFF)
set(BUILD_TESTS OFF)

find_package(ECM 5.44.0 CONFIG REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_KDE_MODULE_DIR})

list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules)

include(ECMInstallIcons)

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings)
include(FeatureSummary)

find_package(Qt5 CONFIG REQUIRED COMPONENTS
    Core
    DBus
    Widgets
    Test # TODO this should be optional
)

find_package(KF5TextEditor)

find_package(KF5 REQUIRED COMPONENTS
    Config
    CoreAddons
    DocTools
    IconThemes
    I18n
    KDELibs4Support
    KHtml
    KIO
    Parts
    TextEditor
    TextWidgets
    Solid
    WidgetsAddons
    XmlGui
)

find_package(GPSim REQUIRED)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_compile_options(
    -Wno-deprecated-declarations
    -Wall
    -Wextra
)

list(APPEND build_flags
    -fPIC
)

if (KTECHLAB_DEVELOPER_BUILD)
    list(APPEND build_flags
        -march=native
        -fsanitize=address
        -fno-omit-frame-pointer
    )
endif (KTECHLAB_DEVELOPER_BUILD)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND build_flags -Og)
else()
    list(APPEND build_flags -O3)
endif ()

list(APPEND build_flags ${Qt5Core_EXECUTABLE_COMPILE_FLAGS})

add_compile_options(
    ${build_flags}
)

add_link_options(
    ${build_flags}
)

add_definitions(
    ${GLIB2_DEFINITIONS}
    ${KDE4_DEFINITIONS}
)

message("Qt5Core_EXECUTABLE_COMPILE_FLAGS=>>>${Qt5Core_EXECUTABLE_COMPILE_FLAGS}<<<" )

# KDE disables STL compatibility by default; re-enabling it is supported
# see http://kde.6490.n7.nabble.com/QT-NO-STL-in-KDE4-td1123882i20.html
# and http://www.cmake.org/Wiki/HowToBuildKDE4Software#Some_useful_functions
remove_definitions(-DQT_NO_STL)

# TODO switch to NO_GPSIM to HAVE_GPSIM
if (NOT GPSim_FOUND)
    set(NO_GPSIM TRUE)
endif ()

message("Qt5Core_INCLUDE_DIRS=>>>${Qt5Core_INCLUDE_DIRS}<<<" )
message("Qt5Widgets_INCLUDE_DIRS=>>>${Qt5Widgets_INCLUDE_DIRS}<<<" )
message("KF5TextEditor_INCLUDE_DIRS=>>>${KF5TextEditor_INCLUDE_DIRS}<<<" )

include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}/src/core
    ${KDE4_INCLUDES}
    ${TextEditor_INCLUDE_DIRS}  # TODO figure out this one
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5Widgets_INCLUDE_DIRS}
    ${Qt5Test_INCLUDE_DIRS}
)

set(SRC_EXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples/)
set(SRC_TESTS_DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/tests/data/)

CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/config.h.cmake
	${CMAKE_BINARY_DIR}/config.h
)

message(STATUS "include_dir begin")
get_property(dirs DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY INCLUDE_DIRECTORIES)
foreach(dir ${dirs})
  message(STATUS "include_dir='${dir}'")
endforeach()
message(STATUS "include_dir end")

add_subdirectory( microbe )
add_subdirectory( src )
if (BUILD_EXAMPLES)
    add_subdirectory( examples )
endif ()
add_subdirectory( icons )
add_subdirectory( icons-kde3 )
if (BUILD_DOCS)
    add_subdirectory( doc )
endif ()
if (BUILD_TESTS)
    add_subdirectory( tests )
endif ()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

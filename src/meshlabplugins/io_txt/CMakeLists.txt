# Copyright 2019-2020, Collabora, Ltd.
# SPDX-License-Identifier: BSL-1.0


set(SOURCES io_txt.cpp)

set(HEADERS io_txt.h)

add_library(io_txt MODULE ${SOURCES} ${HEADERS})

target_include_directories(io_txt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(io_txt PUBLIC meshlab-common)

set_property(TARGET io_txt PROPERTY FOLDER Plugins)

set_property(TARGET io_txt PROPERTY RUNTIME_OUTPUT_DIRECTORY
                                    ${MESHLAB_PLUGIN_OUTPUT_DIR})

set_property(TARGET io_txt PROPERTY LIBRARY_OUTPUT_DIRECTORY
                                    ${MESHLAB_PLUGIN_OUTPUT_DIR})

install(
    TARGETS io_txt
    DESTINATION ${MESHLAB_PLUGIN_INSTALL_DIR}
    COMPONENT Plugins)

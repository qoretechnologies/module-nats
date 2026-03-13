cmake_minimum_required(VERSION 3.11.0)

project(qore-nats-module)

set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)

set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
    cmake_policy(SET CMP0074 NEW)

    if (${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.18.0")
        cmake_policy(SET CMP0053 NEW)
    endif()
endif()

cmake_policy(SET CMP0042 NEW)

if (UNIX AND NOT APPLE)
    set(LINUX TRUE)
endif()

find_package(Qore 2.0 REQUIRED)

# Find nats.c library — if not installed, fetch and build from source
find_path(NATS_INCLUDE_DIR nats/nats.h)
find_library(NATS_LIBRARY NAMES nats)
if(NOT NATS_INCLUDE_DIR OR NOT NATS_LIBRARY)
    message(STATUS "nats.c not found locally — fetching v3.10.0 from GitHub")
    include(FetchContent)
    set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)
    set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    # Build static only to avoid target name conflict with our "nats" module
    set(NATS_BUILD_LIB_SHARED OFF CACHE BOOL "" FORCE)
    set(NATS_BUILD_LIB_STATIC ON CACHE BOOL "" FORCE)
    # Static lib must be built with -fPIC since it's linked into our shared module
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    FetchContent_Declare(
        nats_c
        GIT_REPOSITORY https://github.com/nats-io/nats.c.git
        GIT_TAG        v3.10.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(nats_c)
    # nats.c source puts headers in src/nats.h, but installed layout is nats/nats.h.
    # Create a symlink so #include <nats/nats.h> works from the source tree.
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/nats_include")
    file(CREATE_LINK "${nats_c_SOURCE_DIR}/src" "${CMAKE_CURRENT_BINARY_DIR}/nats_include/nats" SYMBOLIC)
    set(NATS_INCLUDE_DIR "${CMAKE_CURRENT_BINARY_DIR}/nats_include")
    set(NATS_LIBRARY nats_static)
else()
    message(STATUS "Found nats.c: ${NATS_LIBRARY}")
    message(STATUS "nats.c include dir: ${NATS_INCLUDE_DIR}")
endif()

# Check for C++17
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++17" COMPILER_SUPPORTS_CXX17)
if(COMPILER_SUPPORTS_CXX17)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++17 support. Please use a different C++ compiler.")
endif()

set(QPP_SRC
    src/QC_NatsConnection.qpp
    src/QC_NatsMsg.qpp
    src/QC_NatsSubscription.qpp
    src/QC_JetStreamContext.qpp
    src/QC_NatsKeyValueStore.qpp
)

set(CPP_SRC
    src/nats-module.cpp
    src/NatsHelper.cpp
    src/QoreNatsConnection.cpp
    src/QoreNatsSubscription.cpp
    src/QoreNatsJetStream.cpp
    src/QoreNatsKVStore.cpp
)

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SRC ${QPP_SRC})

foreach (it ${QPP_SOURCES})
    GET_FILENAME_COMPONENT(_outfile ${it} NAME_WE)
    set(QPP_DOX ${QPP_DOX} ${CMAKE_CURRENT_BINARY_DIR}/${_outfile}.dox.h)
endforeach()

set(module_name "nats")

set(QMOD
    qlib/NatsUtil
    qlib/NatsDataProvider
    qlib/NatsJetStreamDataProvider
)

set(QORE_DOX_TMPL_SRC
    docs/mainpage.dox.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES})

include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${NATS_INCLUDE_DIR})
target_include_directories(${module_name} PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

add_custom_target(QORE_INC_FILES DEPENDS ${QORE_INC_SRC})
add_dependencies(${module_name} QORE_INC_FILES)

target_link_libraries(${module_name} ${NATS_LIBRARY} ${QORE_LIBRARY})

set(MODULE_DOX_INPUT ${CMAKE_CURRENT_BINARY_DIR}/mainpage.dox ${QPP_DOX})
string(REPLACE ";" " " MODULE_DOX_INPUT "${MODULE_DOX_INPUT}")

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

set(MODULE_DOX_INPUT ${CMAKE_BINARY_DIR})
qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

qore_user_modules("${QMOD}")
qore_external_user_module("qlib/NatsUtil" "")
qore_external_user_module("qlib/NatsDataProvider" "")
qore_external_user_module("qlib/NatsJetStreamDataProvider" "")

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SRC})
else()
    install(FILES ${QPP_META_SRC}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

qore_config_info()

if (DOXYGEN_FOUND)
    qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
    add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
    add_dependencies(docs-module QORE_MOD_DOX_FILES)

    # Two-phase doc build: the initial pass (docs-module) generates nats.tag with empty
    # TAGFILES. User module docs (docs-NatsUtil, docs-NatsDataProvider) are then built using
    # nats.tag, generating their own tag files. The final pass (docs-module-final) rebuilds
    # the binary module docs with user module tag files in TAGFILES, enabling @ref
    # cross-references from the mainpage into user module symbols.

    # Suppress unresolved cross-reference warnings in the initial pass
    file(APPEND ${CMAKE_BINARY_DIR}/Doxyfile
        "\n# Suppress warnings for initial pass (no user module TAGFILES available)\nWARN_IF_DOC_ERROR = NO\n")

    # Create final-pass Doxyfile from the binary module's Doxyfile with user module tag files
    set(_tagfiles "${CMAKE_BINARY_DIR}/NatsUtil.tag=../NatsUtil/html "
        "${CMAKE_BINARY_DIR}/NatsDataProvider.tag=../NatsDataProvider/html")
    set(_doc_deps docs-NatsUtil docs-NatsDataProvider docs-NatsJetStreamDataProvider)
    string(APPEND _tagfiles
        " ${CMAKE_BINARY_DIR}/NatsJetStreamDataProvider.tag=../NatsJetStreamDataProvider/html")

    file(COPY_FILE ${CMAKE_BINARY_DIR}/Doxyfile ${CMAKE_BINARY_DIR}/Doxyfile.final)
    file(APPEND ${CMAKE_BINARY_DIR}/Doxyfile.final
        "\n# Final pass: enable user module cross-references and re-enable doc error warnings\n"
        "TAGFILES = ${_tagfiles}\n"
        "WARN_IF_DOC_ERROR = YES\n")

    add_custom_target(docs-module-final
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile.final
        COMMAND ${QORE_DOCS_ENV} ${QORE_QDX_COMMAND} --post
            ${CMAKE_BINARY_DIR}/docs/${module_name}/html
            ${CMAKE_BINARY_DIR}/docs/${module_name}/html/search
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen (final pass with user module cross-references)"
        VERBATIM
    )
    add_dependencies(docs-module-final ${_doc_deps})
    add_dependencies(docs docs-module-final)
endif()

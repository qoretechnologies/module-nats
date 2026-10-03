cmake_minimum_required(VERSION 3.21...3.31)

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

list(APPEND QORE_DOXYGEN_MODULES DataProvider)
find_package(Qore 2.0 REQUIRED)

# Find nats.c library — if not installed, fetch and build from source
find_path(NATS_INCLUDE_DIR nats/nats.h)
find_library(NATS_LIBRARY NAMES nats)
if(NOT NATS_INCLUDE_DIR OR NOT NATS_LIBRARY)
    message(STATUS "nats.c not found locally — fetching v3.12.0 from GitHub")
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
        GIT_TAG        v3.12.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(nats_c)
    include(cmake/PrepareNatsSources.cmake)
    qore_prepare_nats_sources("${nats_c_SOURCE_DIR}")

    # v3.12.0 already guards the microservice error counter against NULL errors.
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
    src/QC_NatsKeyValueWatcher.qpp
    src/QC_NatsKeyValueStore.qpp
    src/QC_NatsMicroService.qpp
    src/QC_NatsMicroGroup.qpp
)

set(CPP_SRC
    src/nats-module.cpp
    src/NatsHelper.cpp
    src/QoreNatsConnection.cpp
    src/QoreNatsSubscription.cpp
    src/QoreNatsJetStream.cpp
    src/QoreNatsKVStore.cpp
    src/QoreNatsKVWatcher.cpp
    src/QoreNatsMicroService.cpp
    src/QoreNatsMicroGroup.cpp
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
    docs/guide-natsarch.dox.tmpl
    docs/guide-natsauth.dox.tmpl
    docs/guide-natsbuilding.dox.tmpl
    docs/guide-natsconstants.dox.tmpl
    docs/guide-natscorequickstart.dox.tmpl
    docs/guide-natsdp.dox.tmpl
    docs/guide-natshashdecls.dox.tmpl
    docs/guide-natsjetdp.dox.tmpl
    docs/guide-natsjetstream.dox.tmpl
    docs/guide-natskv.dox.tmpl
    docs/guide-natsrelnotes.dox.tmpl
    docs/guide-natsreqs.dox.tmpl
    docs/guide-natstls.dox.tmpl
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

list(APPEND MODULE_DOX_INPUT
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsarch.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsauth.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsbuilding.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsconstants.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natscorequickstart.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsdp.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natshashdecls.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsjetdp.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsjetstream.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natskv.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsrelnotes.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natsreqs.dox
    ${CMAKE_CURRENT_BINARY_DIR}/guide-natstls.dox
)
set(QORE_DOXYGEN_IMAGES
    "${CMAKE_SOURCE_DIR}/qlib/NatsDataProvider/nats-logo.svg"
)
qore_external_binary_module(${module_name} ${PROJECT_VERSION})

qore_dist(${PROJECT_VERSION})

qore_user_modules("${QMOD}")
qore_external_user_module("qlib/NatsUtil" "")
qore_external_user_module("qlib/NatsDataProvider" "NatsUtil")
qore_external_user_module("qlib/NatsJetStreamDataProvider" "NatsUtil")

# Reconcile the installed i18n catalog root against the fragments these modules
# currently own.  Installing catalogs is otherwise additive: a fragment installed by
# an earlier generation of these sources survives every later install, is still
# discovered, and makes catalog composition fail as soon as the current generation
# describes the same app differently.
#
# This must come after every call above that installs catalogs, both because those
# calls record the fragment set and because install() rules run in declaration order,
# which is what publishes the manifest after the fragments it describes.  Removals are
# scoped to this project, so fragments Qore and other products install into the same
# shared catalog root are untouched.  Guarded because the helper arrived in Qore 2.3.
if (COMMAND qore_finalize_catalog_install)
    qore_finalize_catalog_install()
endif()

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

    qore_binary_module_two_phase_docs(${module_name} "NatsDataProvider;NatsJetStreamDataProvider;NatsUtil")
endif()

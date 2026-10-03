# Copyright (C) 2026 Qore Technologies, s.r.o.
# SPDX-License-Identifier: MIT
# Preserve const through read-only searches with glibc's type-generic string
# functions. URL/header tokenization operates on owned, mutable buffers.
# Keep the pinned upstream checkout intact and compile corrected private copies.
function(qore_prepare_nats_sources source_dir)
    set(files msg.c url.c util.c glib/glib_last_error.c)
    set(hashes
        4b9048fefb1358976d1ab4ac556713999cd4818bb2728da9215bb93bfee6e2a2
        f76aeb81995684cf6332070d09bad1562f8d4f6dd26d27f8a50603555cbcd6eb
        65bcdc9e4d19ca1fb7ee6b83dc3a9fb3aac84868a4441164eef21f74e1c96086
        8b5829462bb217f4e24dc50fcc55d70aa57ed9843865ad63ffceaa3087306ea0)
    get_target_property(sources nats_static SOURCES)
    foreach(file hash IN ZIP_LISTS files hashes)
        set(original "${source_dir}/src/${file}")
        file(SHA256 "${original}" actual_hash)
        if(NOT actual_hash STREQUAL hash)
            message(FATAL_ERROR "nats.c v3.12.0 source changed: ${file}; review the const-correctness fixes")
        endif()
        list(FIND sources "${original}" index)
        if(index EQUAL -1)
            message(FATAL_ERROR "nats_static does not compile expected source: ${original}")
        endif()
        file(READ "${original}" content)
        if(file STREQUAL "msg.c")
            string(REPLACE "strchr((const char*) ptr, (int) ':')" "strchr(ptr, (int) ':')" content "${content}")
        elseif(file STREQUAL "url.c")
            # Both pointers address the mutable allocation returned by nats_Trim.
            string(REPLACE "strrchr(host, ']')" "strrchr((char*) host, ']')" content "${content}")
            string(REPLACE "strchr(port, '/')" "strchr((char*) port, '/')" content "${content}")
        elseif(file STREQUAL "util.c")
            string(REPLACE "char        *tok        = NULL;" "const char  *tok        = NULL;" content "${content}")
            string(REPLACE "char *tmp;" "const char *tmp;" content "${content}")
        elseif(file STREQUAL "glib/glib_last_error.c")
            string(REPLACE "static char*\n_getErrorShortFileName" "static const char*\n_getErrorShortFileName" content "${content}")
            string(REPLACE "char *file = strstr(fileName, \"src\");" "const char *file = strstr(fileName, \"src\");" content "${content}")
            string(REPLACE "file = (char*) fileName;" "file = fileName;" content "${content}")
        endif()
        set(output "${CMAKE_CURRENT_BINARY_DIR}/nats-sources/${file}")
        file(GENERATE OUTPUT "${output}" CONTENT "${content}")
        set_source_files_properties("${output}" TARGET_DIRECTORY nats_static PROPERTIES GENERATED TRUE)
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${original}")
        list(REMOVE_AT sources ${index})
        list(APPEND sources "${output}")
    endforeach()
    set_property(TARGET nats_static PROPERTY SOURCES "${sources}")
endfunction()

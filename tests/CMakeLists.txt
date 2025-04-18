find_program(BASH_PROGRAM bash)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/tests)
set(CTEST_BINARY_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
set(nix_gnu_cflags ${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-error -Wno-unused-parameter -Wno-unused-result -Wno-discarded-qualifiers)
set(nix_clang_cflags ${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-unused-parameter -Wno-unused-result -Wno-incompatible-pointer-types-discards-qualifiers)
set(win_msvc_cflags ${CMAKE_C_FLAGS} /Wall)

file(GLOB source_files "test_*.c")
file(GLOB rt_files "rt_*.sh")
set(ext_pattern "(^.*/|\\.[^.]*$)")

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/data
        DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if (BASH_PROGRAM AND TESTS_RT)
    foreach(rt_file ${rt_files})
        file(REAL_PATH ${rt_file} rt_name)
        string(REGEX REPLACE ${ext_pattern} "" rt_name ${rt_file})
        add_test (${rt_name} ${BASH_PROGRAM} ${rt_file})
    endforeach()
endif()

foreach(source_file ${source_files})
    string(REGEX REPLACE ${ext_pattern} "" test_executable ${source_file})
    add_executable(${test_executable} ${source_file})
    if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
        target_compile_options(${test_executable} PRIVATE ${nix_cflags} ${nix_gnu_cflags})
    elseif (CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(${test_executable} PRIVATE ${nix_cflags} ${nix_clang_cflags})
    elseif (CMAKE_C_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(${test_executable} PRIVATE ${win_cflags} ${win_msvc_cflags})
    endif()
    target_include_directories(${test_executable} PRIVATE
            ${core_INCLUDE}
            ${delivery_INCLUDE}
            ${CMAKE_CURRENT_SOURCE_DIR}/include
    )
    target_link_libraries(${test_executable} PRIVATE
            stasis_delivery
    )
    add_test(${test_executable} ${test_executable})
    set_tests_properties(${test_executable}
            PROPERTIES
            TIMEOUT 240)
    set_tests_properties(${test_executable}
            PROPERTIES
            SKIP_RETURN_CODE 127)
    set_property(TEST ${test_executable}
            PROPERTY ENVIRONMENT "STASIS_SYSCONFDIR=${CMAKE_SOURCE_DIR}")
endforeach()

add_library(linux_wire_test_mocks
    mock_linux_wire.cpp
)

target_include_directories(linux_wire_test_mocks
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_features(linux_wire_test_mocks PUBLIC cxx_std_17)

add_executable(linux_wire_tests
    test_wire.cpp
    ../src/Wire.cpp
)

target_link_libraries(linux_wire_tests PRIVATE linux_wire_test_mocks)

target_include_directories(linux_wire_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
    ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME linux_wire_tests COMMAND linux_wire_tests)

add_executable(linux_wire_c_tests
    test_linux_wire_c.c
)

target_link_libraries(linux_wire_c_tests PRIVATE linux_wire)

add_test(NAME linux_wire_c_tests COMMAND linux_wire_c_tests)

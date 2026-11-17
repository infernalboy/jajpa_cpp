cmake_minimum_required(VERSION 3.5)

project(jsonparse)
set(TARGET jsonparse_test)


include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)



file(GLOB SRC_DIR
     "*.h"
     "*.cpp"
)
add_executable(
    ${TARGET}
    ${SRC_DIR}
    )

if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
    set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -O3" )
endif()

set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -Wall -Wextra -Wpedantic -Wcast-align -Wcast-qual" )
set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -Wconversion -Wctor-dtor-privacy -Wenum-compare -Wfloat-equal" )
set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual" )
set(COMPILE_OPTIONS "${COMPILE_OPTIONS} -Wredundant-decls -Wsign-conversion -Wsign-promo" )


message("COMPILE_OPTIONS : \n   ${COMPILE_OPTIONS}")

#target_compile_options(${TARGET} COMPILE_OPTIONS)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMPILE_OPTIONS} ")

set_target_properties(${TARGET} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS YES
)
target_compile_features(${TARGET} PUBLIC cxx_std_17)

#set_target_properties(${TARGET} PROPERTIES COMPILE_FLAGS "-stdlib=libc++ -pthread")
#       target_link_libraries(${TARGET} c++experimental pthread)

enable_testing()

target_link_libraries(
  jsonparse_test
  GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(jsonparse_test)

#to activate big json tests get tis files
#https://awesomeopensource.com/project/jdorfman/awesome-json-datasets
configure_file("test_data/big_test.json" "${CMAKE_CURRENT_BINARY_DIR}/big_test.json" COPYONLY)
#https://github.com/seductiveapps/largeJSON/blob/master/100mb.json
configure_file("test_data/100mb_test.json" "${CMAKE_CURRENT_BINARY_DIR}/100mb_test.json" COPYONLY)

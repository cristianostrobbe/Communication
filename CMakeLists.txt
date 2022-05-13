cmake_minimum_required(VERSION 3.15)

project(Communication VERSION 1.0.0)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -W -g -pthread -march=native")

option(BUILD_EXAMPLES "Build examples" ON)

find_package(nlohmann_json REQUIRED)

find_library(ZeroMQ_LIBRARY
   NAMES zmq
   PATHS ${PC_ZeroMQ_LIBRARY_DIRS}
)

find_package(cppzmq)
if(cppzmq_FOUND)
    message(STATUS "ZeroMQ Include Dir: " ${ZeroMQ_INCLUDE_DIR} )
    message(STATUS "C++ZMQ Include Dir: " ${cppzmq_INCLUDE_DIR} )
    message(STATUS "ZeroMQ Library:     " ${ZeroMQ_LIBRARY} )
    include_directories(${PROJECT_NAME} PUBLIC ${ZeroMQ_INCLUDE_DIR})
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

add_library(${PROJECT_NAME} SHARED src/Communication.cc )

include_directories(${PROJECT_SOURCE_DIR}/inc)

target_include_directories(${PROJECT_NAME} PUBLIC
   ${PROJECT_SOURCE_DIR}/inc 
)

target_link_libraries(${PROJECT_NAME} ${ZeroMQ_LIBRARY})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_SOURCE_DIR}/inc)

if(BUILD_EXAMPLES)
   add_executable(advanced examples/advanced_example.cc)
   target_link_libraries(advanced ${PROJECT_NAME} nlohmann_json)

   add_executable(simple examples/simple_example.cc)
   target_link_libraries(simple ${PROJECT_NAME} nlohmann_json)   
endif()

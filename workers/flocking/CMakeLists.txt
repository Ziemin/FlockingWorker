cmake_minimum_required (VERSION 3.0)
project (FlockingWorker)

include_directories("${PROJECT_SOURCE_DIR}/WorkerSdk/include")
include_directories("${PROJECT_SOURCE_DIR}/generated")

if(MSVC)
  # Static runtime on windows to avoid requiring the MSVCRT*.dll.
  foreach(FLAG_VAR
    CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
    CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
    if(${FLAG_VAR} MATCHES "/MD")
      string(REGEX REPLACE "/MD" "/MT" ${FLAG_VAR} "${${FLAG_VAR}}")
    endif()
  endforeach()
elseif(NOT APPLE)
  # Static GCC C++ and C standard libraries.
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

  # 32/64-bit switch on linux. Should be before adding the external subdirectory, since their
  # setting must match.
  if(LINUX_32BIT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  endif()
  if(LINUX_64BIT)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  endif()
endif()

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")
endif()

find_library(WORKER_SDK WorkerSdk "${PROJECT_SOURCE_DIR}/WorkerSdk/lib")
find_library(LIB_PROTO NAMES libprotobuf protobuf PATHS "${PROJECT_SOURCE_DIR}/WorkerSdk/lib")
find_library(LIB_CRYPTO ssl "${PROJECT_SOURCE_DIR}/WorkerSdk/lib")
find_library(LIB_SSL crypto libprotobuf protobuf PATHS "${PROJECT_SOURCE_DIR}/WorkerSdk/lib")
find_library(LIB_RAKNET RakNetLibStatic "${PROJECT_SOURCE_DIR}/WorkerSdk/lib")

FILE(GLOB_RECURSE SOURCES "${PROJECT_SOURCE_DIR}/generated/*.cc")
FILE(GLOB_RECURSE SOURCES2 "${PROJECT_SOURCE_DIR}/flocking/*.cpp")

# Build the worker
add_executable(FlockingWorker ${SOURCES} ${SOURCES2})
target_link_libraries(FlockingWorker ${WORKER_SDK} ${LIB_PROTO} ${LIB_CRYPTO} ${LIB_SSL} ${LIB_RAKNET})
#target_include_directories(FlockingWorker SYSTEM PRIVATE "${PROJECT_SOURCE_DIR}/WorkerSdk/include/google/protobuf")

# Create the Worker@OS.zip file
set(WORKER_ASSEMBLY_DIR "${PROJECT_SOURCE_DIR}/../../build/assembly/worker")
file(MAKE_DIRECTORY ${WORKER_ASSEMBLY_DIR})

if(${CMAKE_SYSTEM_NAME} STREQUAL "Darwin")
	set(WORKER_NAME "FlockingWorker@Mac")
	set(TARGET_NAME "${WORKER_NAME}")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "Windows")
	set(WORKER_NAME "FlockingWorker@${CMAKE_SYSTEM_NAME}")
	set(TARGET_NAME "${WORKER_NAME}.exe")
else()
	set(WORKER_NAME "FlockingWorker@${CMAKE_SYSTEM_NAME}")
	set(TARGET_NAME "${WORKER_NAME}")
endif()

add_custom_target(create_zip ALL
	WORKING_DIRECTORY "${WORKER_ASSEMBLY_DIR}"
	COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:FlockingWorker>" "."
	COMMAND ${CMAKE_COMMAND} -E rename "${WORKER_ASSEMBLY_DIR}/$<TARGET_FILE_NAME:FlockingWorker>" "${WORKER_ASSEMBLY_DIR}/${TARGET_NAME}"
	COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${WORKER_NAME}.zip" --format=zip "${TARGET_NAME}"
	COMMAND ${CMAKE_COMMAND} -E remove "${TARGET_NAME}"
	DEPENDS FlockingWorker)
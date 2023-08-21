cmake_minimum_required(VERSION 2.6.0)

project(RunTests)

# Supported options:
# -DFLEDGE_INCLUDE
# -DFLEDGE_LIB
# -DFLEDGE_SRC
# -DFLEDGE_INSTALL
#
# If no -D options are given and FLEDGE_ROOT environment variable is set
# then Fledge libraries and header files are pulled from FLEDGE_ROOT path.

set(CMAKE_CXX_FLAGS "-std=c++11 -O3")

# Generation version header file
set_source_files_properties(version.h PROPERTIES GENERATED TRUE)
add_custom_command(
  OUTPUT version.h
  DEPENDS ${CMAKE_SOURCE_DIR}/../VERSION
  COMMAND ${CMAKE_SOURCE_DIR}/../mkversion ${CMAKE_SOURCE_DIR}/..
  COMMENT "Generating version header"
  VERBATIM
)
include_directories(${CMAKE_BINARY_DIR})

# Add here all needed Fledge libraries as list
set(NEEDED_FLEDGE_LIBS common-lib services-common-lib filters-common-lib)

set(BOOST_COMPONENTS system thread)

find_package(Boost 1.53.0 COMPONENTS ${BOOST_COMPONENTS} REQUIRED)
include_directories(SYSTEM ${Boost_INCLUDE_DIR})

# Find source files
file(GLOB SOURCES ../*.cpp)
file(GLOB unittests "*.cpp")

# Find Fledge includes and libs, by including FindFledge.cmak file
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/..)
find_package(Fledge)
# If errors: make clean and remove Makefile
if (NOT FLEDGE_FOUND)
	if (EXISTS "${CMAKE_BINARY_DIR}/Makefile")
		execute_process(COMMAND make clean WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
		file(REMOVE "${CMAKE_BINARY_DIR}/Makefile")
	endif()
	# Stop the build process
	message(FATAL_ERROR "Fledge plugin '${PROJECT_NAME}' build error.")
endif()
# On success, FLEDGE_INCLUDE_DIRS and FLEDGE_LIB_DIRS variables are set 

# Locate GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Add ../include
include_directories(../include)
# Add Fledge include dir(s)
include_directories(${FLEDGE_INCLUDE_DIRS})

# Add other include paths
if (FLEDGE_SRC)
	message(STATUS "Using third-party includes " ${FLEDGE_SRC}/C/thirdparty)
	include_directories(${FLEDGE_SRC}/C/thirdparty/rapidjson/include)
	include_directories(${FLEDGE_SRC}/C/thirdparty/Simple-Web-Server)
endif()

# Add Fledge lib path
link_directories(${FLEDGE_LIB_DIRS})

# Link runTests with what we want to test and the GTest and pthread library
add_executable(RunTests ${unittests} ${SOURCES} version.h)

# Add additional libraries

# Add additional link directories

set(FLEDGE_INSTALL "" CACHE INTERNAL "")
# Install library
if (FLEDGE_INSTALL)
	message(STATUS "Installing ${PROJECT_NAME} in ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME}")
	install(TARGETS ${PROJECT_NAME} DESTINATION ${FLEDGE_INSTALL}/plugins/${PLUGIN_TYPE}/${PROJECT_NAME})
endif()


target_link_libraries(RunTests ${GTEST_LIBRARIES} pthread)
target_link_libraries(RunTests ${NEEDED_FLEDGE_LIBS})
target_link_libraries(RunTests  ${Boost_LIBRARIES})
target_link_libraries(RunTests -lpthread -ldl)

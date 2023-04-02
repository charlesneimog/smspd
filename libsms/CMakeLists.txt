cmake_minimum_required(VERSION 3.0)
project(sms)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source files to the project
file(GLOB_RECURSE SRC_FILES src/*.c)

set(sms_include src/sms.h)
list(APPEND include_files ${sms_include})

# Add the library to the project
add_library(sms SHARED ${SRC_FILES})

link_directories(/usr/lib /opt/local/lib /usr/local/lib)
include_directories(/usr/include /opt/local/include /usr/local/include src)

# add dynamic library dependencies:  libsndfile libgsl
target_link_libraries(sms sndfile gsl gslcblas)

# if MinGW, build a dll not a .so



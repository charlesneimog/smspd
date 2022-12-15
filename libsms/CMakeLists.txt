# Create a Cmake for the project called sms. sms is a Dynamic Library (shared object) and is built from the source files in the src directory. The resulting library is called libsms.so and is placed in the bin directory.
cmake_minimum_required(VERSION 3.0)
project(sms)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the source files to the project
file(GLOB_RECURSE SRC_FILES src/*.c)

set(sms_include src/sms.h)
list(APPEND include_files ${sms_include})

# Add the include directories to the project
include_directories(src)


# Add the library to the project
add_library(sms SHARED ${SRC_FILES})

# add dynamic library dependencies:  libsndfile libgsl
target_link_libraries(sms sndfile gsl gslcblas)

# Add the install targets
install(TARGETS sms DESTINATION bin)
install(DIRECTORY src DESTINATION include)

# Add sources to the project




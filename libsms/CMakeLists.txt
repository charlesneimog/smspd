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


if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    link_directories(/usr/lib /opt/local/lib /usr/local/lib /usr/local/opt/gsl/lib /usr/local/opt/libsndfile/lib)
    include_directories(/usr/include /opt/local/include /usr/local/include src)
    target_link_libraries(sms /usr/local/opt/libsndfile/lib/libsndfile.dylib /usr/local/opt/gsl/lib/libgsl.dylib /usr/local/opt/gsl/lib/libgslcblas.dylib)
else()
    target_link_libraries(sms sndfile gsl gslcblas)
endif()

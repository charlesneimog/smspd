# Define your standard CMake header (for example):
cmake_minimum_required(VERSION 3.5)
set(PDCMAKE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/pd.cmake CACHE PATH "Path to pd.cmake")
include(${PDCMAKE_DIR}/pd.cmake)
project(pd-sms)

#╭──────────────────────────────────────╮
#│              Static GSL              │
#╰──────────────────────────────────────╯

add_subdirectory(Libraries/gsl)
set_target_properties(gsl PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(gsl PROPERTIES POSITION_INDEPENDENT_CODE ON)

#╭──────────────────────────────────────╮
#│             Static FFTW3             │
#╰──────────────────────────────────────╯

cmake_policy(SET CMP0135 NEW)
option(BUILD_SHARED_LIBS OFF)
option(BUILD_TESTS OFF)
option(DISABLE_FORTRAN ON)

# check if fftw-3.3.10.tar.gz has been downloaded
set(FFTW3_FILE ${CMAKE_BINARY_DIR}/fftw-3.3.10.tar.gz)
if (NOT EXISTS ${FFTW3_FILE})
    message(STATUS "Downloading FFTW3")
    file(DOWNLOAD https://www.fftw.org/fftw-3.3.10.tar.gz ${FFTW3_FILE})
endif()

file(ARCHIVE_EXTRACT
    INPUT ${CMAKE_BINARY_DIR}/fftw-3.3.10.tar.gz
    DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/Libraries/
)

add_subdirectory(Libraries/fftw-3.3.10)
set_target_properties(fftw3 PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(fftw3 PROPERTIES POSITION_INDEPENDENT_CODE ON)

#╭──────────────────────────────────────╮
#│              Static Sms              │
#╰──────────────────────────────────────╯
project(sms)

add_definitions(-DHAVE_FFTW3_H)
add_definitions(-DMEXP=19937)

file(GLOB SOURCES "Libraries/libsms/src/*.c")
add_library(libsms STATIC ${SOURCES})
target_link_libraries(libsms m fftw3 gsl gslcblas)
set_target_properties(libsms PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set_target_properties(libsms PROPERTIES POSITION_INDEPENDENT_CODE ON)

#╭──────────────────────────────────────╮
#│             Pd External              │
#╰──────────────────────────────────────╯
# Get all cpp files inside ${CMAKE_SOURCE_DIR}/Sources/ and add them to the project
file(GLOB EXTERNAL_SOURCES "${CMAKE_SOURCE_DIR}/Sources/*.c")
add_pd_external(sms ${EXTERNAL_SOURCES})

if (WIN32)
    target_link_options(sms PUBLIC "-Wl,-allow-multiple-definition")
endif()

target_link_libraries(sms PUBLIC libsms gsl fftw3 sndfile m)

target_include_directories(sms PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/fftw-3.3.10/api")
target_include_directories(sms PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/Libraries/libsms/src/")

set_target_properties(sms PROPERTIES CXX_STANDARD 03)
target_compile_options(sms PUBLIC
                "-O3" 
                "-ffast-math" 
                "-funroll-loops" 
                "-fomit-frame-pointer" 
                "-mfpmath=sse" 
                "-msse" 
                "-msse2" 
                "-msse3" 
                "-DPD"
)

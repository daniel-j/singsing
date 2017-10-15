# - Try to find Aubio
# Once done this will define
#
#  AUBIO_FOUND - system has Aubio
#  AUBIO_INCLUDE_DIRS - the Aubio include directory
#  AUBIO_LIBRARIES - Link these to use Aubio
#


find_path(AUBIO_INCLUDE_DIR
    NAMES
    aubio/aubio.h
    PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
    /sw/include
)

find_library(AUBIO_LIBRARY
    NAMES
    aubio
    PATHS
    /usr/lib
    /usr/lib/i386-linux-gnu
    /usr/local/lib
    /opt/local/lib
    /sw/lib
)

set(AUBIO_INCLUDE_DIRS
    ${AUBIO_INCLUDE_DIR}
)
set(AUBIO_LIBRARIES
    ${AUBIO_LIBRARIES}
    ${AUBIO_LIBRARY}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Aubio DEFAULT_MSG AUBIO_LIBRARIES AUBIO_INCLUDE_DIRS)

# show the AUBIO_INCLUDE_DIRS and AUBIO_LIBRARIES variables only in the advanced view
mark_as_advanced(AUBIO_INCLUDE_DIRS AUBIO_LIBRARIES)

# - Try to find Aubio
# Once done this will define
#
#  AUBIO_FOUND - system has Aubio
#  AUBIO_INCLUDE_DIR - the Aubio include directory
#  AUBIO_LIBRARIES - Link these to use Aubio
#

find_path(AUBIO_INCLUDE_DIR NAMES aubio/aubio.h)

find_library(AUBIO_LIBRARY NAMES aubio)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Aubio DEFAULT_MSG AUBIO_LIBRARY AUBIO_INCLUDE_DIR)

mark_as_advanced(AUBIO_INCLUDE_DIR AUBIO_LIBRARY)

# - Try to find Freetype GL
# Once done this will define
#
#  FREETYPEGL_FOUND - system has Freetype GL
#  FREETYPEGL_INCLUDE_DIR - the Freetype GL include directory
#  FREETYPEGL_LIBRARY - Link these to use Freetype GL
#

find_path(FREETYPEGL_INCLUDE_DIR NAMES freetype-gl/freetype-gl.h)

find_library(FREETYPEGL_LIBRARY NAMES freetype-gl)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreetypeGL DEFAULT_MSG FREETYPEGL_LIBRARY FREETYPEGL_INCLUDE_DIR)

mark_as_advanced(FREETYPEGL_INCLUDE_DIR FREETYPEGL_LIBRARY)

# - Try to find Aubio
# Once done this will define
#
#  PROJECTM_FOUND
#  PROJECTM_INCLUDE_DIR
#  PROJECTM_LIBRARY
#

find_path(PROJECTM_INCLUDE_DIR NAMES libprojectM/projectM.hpp)

find_library(PROJECTM_LIBRARY NAMES projectM)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ProjectM DEFAULT_MSG PROJECTM_LIBRARY PROJECTM_INCLUDE_DIR)

mark_as_advanced(PROJECTM_INCLUDE_DIR PROJECTM_LIBRARY)

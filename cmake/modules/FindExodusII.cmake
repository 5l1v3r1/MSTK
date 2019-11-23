# Copyright: 2019- Triad National Security, LLC
#
# ExodusII Find Module for MSTK
#
# ExodusII needs METIS; This module will try to find METIS as well and add it
# as a dependency
#
# Usage: To search a particular path you can specify the path in
# CMAKE_PREFIX_PATH, in the CMake variable ExodusII_DIR or environment
# variable ExodusII_ROOT
#
# Following variables are set:
# ExodusII_FOUND          (BOOL)   Flag indicating if ExodusII was found
# ExodusII_INCLUDE_DIRS   (PATH)   Path to ExodusII include files
# ExodusII_LIBRARY        (FILE)   ExodusII library (libzoltan.a, libzoltan.so)
# ExodusII_LIBRARIES      (LIST)   List of ExodusII targets (MSTK::ExodusII)
#
#
# Additional variables
# ExodusII_VERSION          (STRING)     ExodusII Version string
#
# #############################################################################

set(exodus_inc_names "exodusII.h")
set(exodus_inc_suffixes "include" "cbind/include")
set(exodus_lib_names "exodus" "exoIIv2c")
list(APPEND exodus_lib_suffixes "lib" "Lib")

# First use pkg-config to parse an installed .pc file to find the
# library although we cannot rely on it

find_package(PkgConfig)
pkg_check_modules(PC_ExodusII Quiet parmetis)


# Search for include files

find_path(ExodusII_INCLUDE_DIR
  NAMES exodusII.h
  HINTS ${PC_ExodusII_INCLUDE_DIRS}
  )

if (NOT ExodusII_INCLUDE_DIR)
  if (ExodusII_FIND_REQUIRED)
    message(FATAL "Cannot locate exodusII.h")
  else (ExodusII_FIND_REQUIRED)
    if (NOT ExodusII_FIND_QUIET)
      message(WARNING "Cannot locate exodusII.h")
    endif ()
  endif ()
endif ()

set(ExodusII_INCLUDE_DIRS "${ExodusII_INCLUDE_DIR}")


# Search for libraries

find_library(ExodusII_LIBRARY
  NAMES "exodus" "exoIIv2c" 
  HINTS ${PC_ExodusII_LIBRARY_DIRS})

if (NOT ExodusII_LIBRARY)
  if (ExodusII_FIND_REQUIRED)
    message(FATAL "Can not locate ExodusII library")
  else (ExodusII_FIND_REQUIRED)
    if (NOT ExodusII_FIND_QUIET)
      message(WARNING "Cannot locate ExodusII library")
    endif ()
  endif ()
endif ()

# Set library version

set(ExodusII_VERSION ${PC_ExodusII_VERSION})  # No guarantee
if (NOT ExodusII_VERSION AND ExodusII_INCLUDE_DIR)
  set(exodus_h "${ExodusII_INCLUDE_DIR}/exodusII.h")
  file(STRINGS "${exodus_h}" exodus_version_string REGEX "^#define EX_API_VERS")
  string(REGEX REPLACE "^#define EX_API_VERS ([0-9]+\\.[0-9]+).*$" "\\1" exodus_version "${exodus_version_string}")

  set(ExodusII_VERSION "${exodus_version}")
endif ()


# Finish setting standard variables if everything is found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ExodusII
  FOUND_VAR ExodusII_FOUND
  REQUIRED_VARS ExodusII_LIBRARY ExodusII_INCLUDE_DIR)


# Create ExodusII target

if (ExodusII_FOUND AND NOT TARGET MSTK::ExodusII)
  set(ExodusII_LIBRARIES MSTK::ExodusII)
  add_library(${ExodusII_LIBRARIES} UNKNOWN IMPORTED)
  set_target_properties(${ExodusII_LIBRARIES} PROPERTIES
    IMPORTED_LOCATION "${ExodusII_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_ExodusII_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${ExodusII_INCLUDE_DIR}")
endif ()


# ExodusII depends on netCDF. Attempt to find it

find_package(netCDF CONFIG)
if (NOT netCDF_FOUND)
  find_package(netCDF REQUIRED MODULE)
endif ()

# Add MSTK::netCDF as a dependency of ExodusII
target_link_libraries(${ExodusII_LIBRARIES} INTERFACE ${netCDF_LIBRARIES})
target_include_directories(${ExodusII_LIBRARIES} INTERFACE ${netCDF_INCLUDE_DIRS})


# Hide these variables from the cache
mark_as_advanced(
  ExodusII_INCLUDE_DIR
  ExodusII_INCLUDE_DIRS
  ExodusII_LIBRARY
  ExodusII_LIBRARIES
  ExodusII_VERSION
  )


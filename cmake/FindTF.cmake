include(FindPackageHandleStandardArgs)

#/opt/venv/lib/python3.13/site-packages/tensorflow

# determine requested version (find_package(TF <ver>) => TF_FIND_VERSION)
if(DEFINED TF_FIND_VERSION)
  set(_requested_version "${TF_FIND_VERSION}")
elseif(DEFINED TF_VERSION)
  # allow caller to set -DTF_VERSION=... as alternative
  set(_requested_version "${TF_VERSION}")
else()
  set(_requested_version "")
endif()

# construct base dir (supports empty version)
set(_tf_base_dir "/opt/soft/tensorflow-${_requested_version}")

if(EXISTS "${_tf_base_dir}")
  message(STATUS "TensorFlow found: ${_tf_base_dir}")
  set(TF_INCLUDE_DIR "${_tf_base_dir}/include")
  set(TF_LIBRARY_DIR "${_tf_base_dir}/lib")
  set(TF_LIBRARIES "${TF_LIBRARY_DIR}/libtensorflow.so")
else()
  message(STATUS "TensorFlow not found at ${_tf_base_dir}")
endif()

# Use the standard helper to set TF_FOUND and other variables
find_package_handle_standard_args(TF
  REQUIRED_VARS TF_INCLUDE_DIR TF_LIBRARIES
  VERSION_VAR TF_VERSION)


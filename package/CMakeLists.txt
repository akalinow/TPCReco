set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_PACKAGE_FILE_NAME
    ${CPACK_PACKAGE_NAME}-v${CPACK_PACKAGE_VERSION}-${CMAKE_SYSTEM_NAME})

set(CPACK_PACKAGE_CONTACT "mfila@fuw.edu.pl")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
    "${CMAKE_PROJECT_NAME} - event reconstruction and analysis for Warsaw TPC")
include(CPack)

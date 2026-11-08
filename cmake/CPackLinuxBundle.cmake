# Per-generator CPack settings of the Linux packages which bundle Qt.
# The .deb keeps its private Qt out of the directories of the distribution; the
# archive stays relocatable.
if(CPACK_GENERATOR STREQUAL "DEB")
    set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/easymorse")
endif()

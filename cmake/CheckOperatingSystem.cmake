###############################################################################
### Check for Linux
IF ( UNIX AND NOT APPLE )
  IF ( ${CMAKE_SYSTEM_NAME} MATCHES "Linux" )
    SET ( CF_OS_LINUX 1 )
  ELSE()
    SET ( CF_OS_UNRECOGNIZED_REASON "Unrecognized UNIX type : coolfluid has only been tested for Linux or MacOSX type UNIX'es")
  ENDIF()
ENDIF()

###############################################################################
### Check for Apple MacOSX
IF ( APPLE )
  IF ( UNIX AND ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    SET ( CF_OS_MACOSX 1 )
  ELSE()
    SET ( CF_OS_UNRECOGNIZED_REASON "Unrecognized APPLE type : coolfluid has only been tested  only with Apple MacOSX ( Darwin ) systems.")
  ENDIF()
ENDIF()

###############################################################################
### Check for Windows
IF ( WIN32 )
  IF ( MSVC )
    SET ( CF_OS_WINDOWS 1 )
  ELSE()
    SET ( CF_OS_UNRECOGNIZED_REASON "Unrecognized WINDOWS type : coolfluid has only been tested with Win32 and MSVC compiler.")
  ENDIF()
ENDIF()

###############################################################################
### FINAL MESSAGE
IF ( CF_OS_UNRECOGNIZED_REASON )
  SET ( CF_OS_UNRECOGNIZED 1 )
  IF ( NOT CF_SKIP_OS_TEST )
    SET ( FULL_MSG "${CF_OS_UNRECOGNIZED_REASON} Set CMake variable CF_SKIP_OS_TEST to avoid this error" )
    MESSAGE ( FATAL_ERROR ${FULL_MSG} )
  ELSE()
    MESSAGE ( STATUS ${CF_OS_UNRECOGNIZED_REASON} )
    MESSAGE ( STATUS "Nevertheless we try to continue ..." )
  ENDIF()
ENDIF()

###############################################################################


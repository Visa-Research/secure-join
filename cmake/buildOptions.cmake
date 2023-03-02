
macro(EVAL var)
     if(${ARGN})
         set(${var} ON)
     else()
         set(${var} OFF)
     endif()
endmacro()

option(FETCH_AUTO      "automaticly download and build dependancies" OFF)
option(SECUREJOIN_ENABLE_ASAN      "Enable Asan" OFF)
option(SECUREJOIN_ENABLE_BOOST     "Enable boost networking" OFF)
option(SECUREJOIN_ENABLE_PAILLIER     "Enable boost paillier" OFF)
option(SECUREJOIN_ENABLE_FAKE_GEN     "Enable boost paillier" OFF)
option(NO_CMAKE_SYSTEM_PATH           "use system paths" OFF)

#option(FETCH_LIBOTE		"download and build libOTe" OFF))
EVAL(FETCH_LIBOTE_AUTO 
	(DEFINED FETCH_LIBOTE AND FETCH_LIBOTE) OR
	((NOT DEFINED FETCH_LIBOTE) AND (FETCH_AUTO)))


message(STATUS "secure-join options\n=======================================================")

message(STATUS "Option: FETCH_AUTO                   = ${FETCH_AUTO}")
message(STATUS "Option: FETCH_LIBOTE                 = ${FETCH_LIBOTE}\n")
message(STATUS "Option: NO_CMAKE_SYSTEM_PATH         = ${NO_CMAKE_SYSTEM_PATH}\n")

message(STATUS "Option: SECUREJOIN_ENABLE_ASAN       = ${SECUREJOIN_ENABLE_ASAN}")
message(STATUS "Option: SECUREJOIN_ENABLE_BOOST      = ${SECUREJOIN_ENABLE_BOOST}")
message(STATUS "Option: SECUREJOIN_ENABLE_PAILLIER   = ${SECUREJOIN_ENABLE_PAILLIER}")


message(STATUS "Option: SECUREJOIN_ENABLE_FAKE_GEN   = ${SECUREJOIN_ENABLE_FAKE_GEN}")



set(SECUREJOIN_CPP_VER 17)
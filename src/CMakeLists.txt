CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

PROJECT(myplace C)

IF("${CMAKE_BUILD_TYPE}" STREQUAL "")
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF("${CMAKE_BUILD_TYPE}" STREQUAL "")
MESSAGE("Build type: ${CMAKE_BUILD_TYPE}")

# Set ready to build
SET(EXT_LIBS_DIRS "")
SET(EXT_LIBS_DEFS "")
SET(EXT_LIBS_LDFLAGS "")

# Set external libraries
SET(EXT_LIBS MYPLACE_EXT_LIBS)

FOREACH(flag ${EXT_LIBS})
    SET(EXT_LIBS_DIRS       ${EXT_LIBS_DIRS}    ${${flag}_INCLUDE_DIRS})
    SET(EXT_LIBS_DEFS       ${EXT_LIBS_DEFS}    ${${flag}_CFLAGS_OTHER})
    SET(EXT_LIBS_LDFLAGS    ${EXT_LIBS_LDFLAGS} ${${flag}_LDFLAGS})
ENDFOREACH(flag)

# Set ready to build
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${EXT_LIBS_DIRS})

ADD_DEFINITIONS(
    ${EXT_LIBS_DEFS}
)

SET(SRCS
    myplace.c
    myplace-placelist.c
    myplace-detailinfo.c
    myplace-delete.c
    myplace-elementary.c
    myplace-mapview.c
)

ADD_DEFINITIONS("-DPREFIX=\"${PREFIX}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DDATADIR=\"${DATADIR}\"")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} "-Wl,--as-needed -pie" "-ldl" ${EXT_LIBS_LDFLAGS})

message(${EXT_LIBS_LDFLAGS})

#install binary & edj files
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})

# install manifest file & icon
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PKG_NAME}.xml DESTINATION /usr/share/packages)

# install application HOME directory
INSTALL(DIRECTORY DESTINATION ${DATADIR})

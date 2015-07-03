CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(myplace C CXX)

ADD_CUSTOM_TARGET(map_view.edj
	COMMAND edje_cc -no-save -id ${CMAKE_CURRENT_SOURCE_DIR}/../data/images
		${CMAKE_CURRENT_SOURCE_DIR}/map_view.edc ${CMAKE_CURRENT_SOURCE_DIR}/map_view.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/map_view.edc
)
ADD_CUSTOM_TARGET(detail_view.edj
    COMMAND edje_cc -no-save -id ${CMAKE_CURRENT_SOURCE_DIR}/../data/images
        ${CMAKE_CURRENT_SOURCE_DIR}/detail_view.edc ${CMAKE_CURRENT_SOURCE_DIR}/detail_view.edj
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/detail_view.edc
)

ADD_DEPENDENCIES(${PROJECT_NAME} map_view.edj)
ADD_DEPENDENCIES(${PROJECT_NAME} detail_view.edj)


#install binary & edj files
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/map_view.edj DESTINATION ${EDJ_DIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/detail_view.edj DESTINATION ${EDJ_DIR})

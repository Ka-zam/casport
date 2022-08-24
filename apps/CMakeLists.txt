set(CMAKE_VERBOSE_MAKEFILE "OFF")

#######################################################################################################
#
#  Add test program
#
#######################################################################################################
add_executable(tcas tcas.cc)

target_include_directories(tcas PUBLIC
 ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(tcas PUBLIC
 casport
)

#######################################################################################################
#
#  Install location
#
#######################################################################################################
set(APPS tcas)
install(TARGETS ${APPS} DESTINATION bin)

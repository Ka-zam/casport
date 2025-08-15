set(CASPORT_HEADER_LIST "${PROJECT_SOURCE_DIR}/include/casport.h")

# All users of this library will need at least C++14
#target_compile_features(librf PUBLIC cxx_std_14)

# IDEs should put the headers in a nice place
source_group(TREE "${PROJECT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${CASPORT_HEADER_LIST})

# Example applications

add_executable(example_filter example_filter.cc)
target_include_directories(example_filter PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(example_filter PUBLIC
    cascadix
)

# Install
install(TARGETS example_filter DESTINATION bin)
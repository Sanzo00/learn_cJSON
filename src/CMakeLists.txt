aux_source_directory(. DIR_LIB_SRCS)

add_library (CJSON ${DIR_LIB_SRCS})

target_link_libraries(CJSON m)

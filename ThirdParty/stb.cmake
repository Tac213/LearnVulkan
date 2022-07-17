file(GLOB stb_sources CONFIGURE_DEPENDS "${THIRD_PARTY_FOLDER}/stb")
add_library(stb INTERFACE ${stb_sources})
target_include_directories(stb INTERFACE ${THIRD_PARTY_FOLDER}/stb)

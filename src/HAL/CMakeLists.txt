FILE(GLOB HAL_sources *.c)
message(STATUS HAL : ${HAL_sources})
target_sources(app PRIVATE ${HAL_sources})

add_subdirectory(navigation)

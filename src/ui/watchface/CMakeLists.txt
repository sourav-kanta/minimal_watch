FILE(GLOB watchface_source *.c)
message(STATUS watchface_source : ${watchface_source})
target_sources(app PRIVATE ${watchface_source})

add_subdirectory(wf1)
add_subdirectory(wf2)

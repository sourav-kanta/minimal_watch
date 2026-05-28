FILE(GLOB wf2_source *.c)
message(STATUS wf2_source : ${wf2_source})
target_sources(app PRIVATE ${wf2_source})

add_subdirectory(assets)

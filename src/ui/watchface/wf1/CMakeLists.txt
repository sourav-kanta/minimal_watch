FILE(GLOB wf1_source *.c)
message(STATUS wf1_source : ${wf1_source})
target_sources(app PRIVATE ${wf1_source})

add_subdirectory(assets)

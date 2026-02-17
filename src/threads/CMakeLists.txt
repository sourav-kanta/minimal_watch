FILE(GLOB managers_sources *.c)
message(STATUS managers : ${managers_sources})
target_sources(app PRIVATE ${managers_sources})


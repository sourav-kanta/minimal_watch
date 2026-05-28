FILE(GLOB nav_sources *.c)
message(STATUS nav : ${nav_sources})
target_sources(app PRIVATE ${nav_sources})


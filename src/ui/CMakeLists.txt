FILE(GLOB ui_sources *.c)
message(STATUS ui_sources : ${ui_sources})
target_sources(app PRIVATE ${ui_sources})

add_subdirectory(watchface)
add_subdirectory(appmgr_ui)
add_subdirectory(notification_ui)

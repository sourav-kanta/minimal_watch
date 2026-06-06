FILE(GLOB app_mgr_ui_sources *.c)
message(STATUS app_mgr_ui_sources : ${app_mgr_ui_sources})
target_sources(app PRIVATE ${app_mgr_ui_sources})


FILE(GLOB notif_mgr_ui_sources *.c)
message(STATUS notif_mgr_ui_sources : ${notif_mgr_ui_sources})
target_sources(app PRIVATE ${notif_mgr_ui_sources})


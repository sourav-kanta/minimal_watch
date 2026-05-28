FILE(GLOB BLE_sources *.c)
message(STATUS BLE : ${BLE_sources})
target_sources(app PRIVATE ${BLE_sources})


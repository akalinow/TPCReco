add_unit_test(EventTPC_tst EventSources)
add_unit_test(grawToEventTPC_tst EventSources)

install(DIRECTORY testData DESTINATION ${CMAKE_INSTALL_PREFIX})

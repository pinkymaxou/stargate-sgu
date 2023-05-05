idf_component_register(SRCS "SGUBRProtocol.c" "SGUBRComm.c" "SGUHelper.c"
    INCLUDE_DIRS .
    REQUIRES lwip log)
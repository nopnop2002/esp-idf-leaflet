set(srcs "main.c" "web_server.c" "web_client.c")

if (CONFIG_MODE_DEMO)
    list(APPEND srcs "demo.c")
elseif (CONFIG_MODE_GPS)
    list(APPEND srcs "uart.c")
elseif (CONFIG_MODE_TRIP)
    list(APPEND srcs "trip.c")
endif()

idf_component_register(SRCS "${srcs}"
    INCLUDE_DIRS "."
    EMBED_FILES "../html/error.html"
                "../html/favicon.ico"
                "../html/main.js"
                "../html/root.html"
                "../html/main.css")

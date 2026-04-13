# get IDF version for comparison
set(idf_version "${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}")

# set conversion sources
set(srcs
    sources/http_server.c
)

set(include_dirs 
    ./include
)

list(APPEND srcs
    sources/http_server.c
)

idf_component_register(
  SRCS ${srcs}
  INCLUDE_DIRS ${include_dirs}
  REQUIRES driver esp_wifi nvs_flash esp_http_server esp-tls camera
)

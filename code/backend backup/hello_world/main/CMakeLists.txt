idf_component_register(
    SRCS "hello_world_main.c"
    INCLUDE_DIRS "."

    # Public requirements (other components may include their headers)
    REQUIRES
        esp_http_client
        nvs_flash
        esp_wifi
        json
        esp_netif
        driver

    # Private requirements (only this component needs the headers)
    PRIV_REQUIRES
        mbedtls           # ← gives you esp_crt_bundle.h
)

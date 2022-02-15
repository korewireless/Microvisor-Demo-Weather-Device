cmake_minimum_required(VERSION 3.14)

set(BUILD_NUMBER "8")
add_compile_definitions(BUILD_NUM=${BUILD_NUMBER})
add_compile_definitions(APP_NAME="Microvisor Weather Device Demo")

add_executable(mv-weather-device-demo
    main.c
    cJSON.c
    ht16k33.c
    http.c
    i2c.c
    logging.c
    openweather.c
    stm32u5xx_hal_timebase_tim_template.c
)

target_link_libraries(mv-weather-device-demo LINK_PUBLIC
    ST_Code
    twilio-microvisor-hal-stm32u5
    FreeRTOS)

# Optional informational and additional format generation
# including Bundle output
add_custom_command(OUTPUT EXTRAS
    DEPENDS mv-weather-device-demo
    COMMAND cp "mv-weather-device-demo" "mv-weather-device-demo.elf"
    COMMAND ${CMAKE_SIZE} --format=berkeley "mv-weather-device-demo"
    COMMAND ${CMAKE_OBJDUMP} -h -S "mv-weather-device-demo.elf" > "mv-weather-device-demo.list"
    COMMAND ${CMAKE_OBJCOPY} -O binary "mv-weather-device-demo.elf" "mv-weather-device-demo.bin"
    COMMAND python3 ../../twilio-microvisor-tools/bundler-py/bundler.py mv-weather-device-demo.elf mv-weather-device-demo.zip
    COMMAND rm -f ../../null.d
    COMMAND echo "LATEST BUILD: ${BUILD_NUMBER}"
)

add_custom_target(extras ALL
    ${CMAKE_COMMAND} -E echo "Outputting additional artefacts"
    DEPENDS EXTRAS
)

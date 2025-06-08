file(GLOB SOURCES *.c *.cpp)

set(requires "common" "arduino-esp32")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . ${HEADERS}
                    REQUIRES ${requires}
)
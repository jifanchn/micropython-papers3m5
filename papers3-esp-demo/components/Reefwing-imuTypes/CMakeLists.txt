file(GLOB SOURCES src/*.c src/*.cpp)

set(requires "arduino-esp32")

idf_component_register(SRCS ${SOURCES}
                    INCLUDE_DIRS . src/
                    REQUIRES ${requires}
)
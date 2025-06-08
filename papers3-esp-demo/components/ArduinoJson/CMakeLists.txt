set(requires "arduino-esp32")

idf_component_register(INCLUDE_DIRS . src ${HEADERS}
                    REQUIRES ${requires}
)

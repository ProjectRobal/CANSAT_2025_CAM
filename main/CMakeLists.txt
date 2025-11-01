file(GLOB C_FILES "src/*.c")

idf_component_register(SRCS ${C_FILES} "cansat_cam.c"
                    INCLUDE_DIRS "include")

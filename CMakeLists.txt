cmake_minimum_required(VERSION 3.18)
project(lclServer C)

set(CMAKE_C_STANDARD 11)

add_executable(lclServer main.c

        Simulacia.c
        Simulacia.h
        Mravec.h
        Mravec.c
        Pole.h
        Pole.c
        Plocha.h
        Plocha.c)

target_link_libraries(lclServer pthread)




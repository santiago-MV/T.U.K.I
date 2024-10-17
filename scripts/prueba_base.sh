#!/bin/bash

# Ejecutar el primer comando
cd consola
./consola '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/consola.config' '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/BASE_1.pseudo' &

# Esperar 1 segundo
sleep 1

# Ejecutar el segundo comando
./consola '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/consola.config' '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/BASE_2.pseudo' &

# Esperar 1 segundo
sleep 1

# Ejecutar el tercer comando
./consola '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/consola.config' '/home/utnso/tp-2023-1c-The-New-Pain/consola/config/BASE_2.pseudo' &

# Volver al directorio anterior
cd ..

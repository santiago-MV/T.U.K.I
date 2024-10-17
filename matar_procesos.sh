#!/bin/bash

# Cerrar el proceso con el comando ./memoria
pkill -15 -f "./memoria"

# Cerrar el proceso con el comando ./cpu
pkill -15 -f "./cpu"

# Cerrar el proceso con el comando ./cpu
pkill -15 -f "./fileSystem"

# Cerrar el proceso con el comando ./kernel
pkill -15 -f "./kernel"
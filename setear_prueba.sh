#!/bin/bash

echo "Seleccione el tipo de prueba a realizar:"
echo "1. Base"
echo "2. Deadlock"
echo "3. Memoria"
echo "4. File System"
echo "5. Errores"

read -p "Ingrese el numero de la prueba: " prueba

case $prueba in
    1)
        echo "Prueba Base"
        cp "./configs/memoria/memoria_v1.config" "./memoria/config/memoria.config"
        cp "./configs/cpu/cpu_v1.config" "./cpu/config/cpu.config"
        echo "1. FIFO"
        echo "2. HRRN"
        read -p "Ingrese el numero del algoritmo a usar: " algoritmo
        case $algoritmo in
            1)
                cp "./configs/kernel/kernel_v1.config" "./kernel/config/kernel.config"
                ;;
            2)  
                cp "./configs/kernel/kernel_v5.config" "./kernel/config/kernel.config"
                ;;
        esac
        ;;
    2)
        echo "Prueba Deadlock"
        cp "./configs/kernel/kernel_v2.config" "./kernel/config/kernel.config"
        cp "./configs/memoria/memoria_v1.config" "./memoria/config/memoria.config"
        cp "./configs/cpu/cpu_v1.config" "./cpu/config/cpu.config"
        ;;
    3)
        echo "Prueba Memoria"
        cp "./configs/kernel/kernel_v3.config" "./kernel/config/kernel.config"
        cp "./configs/cpu/cpu_v2.config" "./cpu/config/cpu.config"

        echo "1. FIRST"
        echo "2. BEST"
        echo "3. WORST"
        read -p "Ingrese el numero del algoritmo a usar: " algMem
        case $algMem in
            1)
                cp "./configs/memoria/memoria_v5.config" "./memoria/config/memoria.config"
                ;;
            2)  
                cp "./configs/memoria/memoria_v2.config" "./memoria/config/memoria.config"
                ;;
            3)  
                cp "./configs/memoria/memoria_v4.config" "./memoria/config/memoria.config"
                ;;    
        esac
        ;;
    4)
        echo "Prueba File System"
        cp "./configs/kernel/kernel_v4.config" "./kernel/config/kernel.config"
        cp "./configs/memoria/memoria_v3.config" "./memoria/config/memoria.config"
        cp "./configs/cpu/cpu_v2.config" "./cpu/config/cpu.config"
        ;;
    5)
        echo "Prueba Errores"
        cp "./configs/kernel/kernel_v1.config" "./kernel/config/kernel.config"
        cp "./configs/memoria/memoria_v2.config" "./memoria/config/memoria.config"
        cp "./configs/cpu/cpu_v1.config" "./cpu/config/cpu.config"
        ;;
    *)
        echo "Proba de nuevo wacho"
        ;;
esac

exit 0
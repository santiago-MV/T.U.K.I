#ifndef KERNEL_H_
#define KERNEL_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>
#include"planificacion.h"
#include"utils.h"

extern char* ip;
extern char* puerto_escucha;
extern char* ip_cpu;
extern char* puerto_cpu;
extern char* ip_filesystem;
extern char* puerto_filesystem;
extern char* ip_memoria;
extern char* puerto_memoria;

extern double estimacion_inicial;
extern double hrrn_alfa;
extern char* algoritmo_planificacion;
extern int gradoMaxMultiprog;
extern int conexion_memoria;




#endif
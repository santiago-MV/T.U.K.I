#ifndef MEMORIA_H_
#define MEMORIA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>
#include "utils.h"
#include <pthread.h>
char* ip;
char* puerto;
int tam_memoria;
int tam_segmento_0;
int cant_segmentos;
int retardo_memoria;
int retardo_compactacion;
char* algorimo_asignacion;

void* espacio_usuario;

extern t_list* lista_segmentos_actuales;
t_list* lista_tablas;
t_list* huecos_libres;
#endif /* SERVER_H_ */
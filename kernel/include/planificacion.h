#ifndef PLANIFICACION_H_
#define PLANIFICACION_H_

#include <stdio.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "kernel.h"
#include "utils.h"
#include<commons/temporal.h>
#include<commons/collections/queue.h>
#include <commons/collections/list.h>

void iniciar_listas_y_semaforos();
t_pcb* crear_pcb(t_list* instructions, int conexion_consola);
void actualizar_rafaga_ejecucion_del_pcb(t_pcb* pcbReceptor, t_pcb* pcbACopiar);
void actualizar_proxima_rafaga_estimada_del_pcb(t_pcb* pcb1);
void actualizar_conexion_consola(t_pcb* pcbActualizado, t_pcb* pcbEnEjecucion);
void actualizar_archivos(t_pcb* pcbActualizado,t_pcb*);
void actualizar_archivos_seguir_exec(t_pcb* pcbActualizado);
int verificar_finalizacion(int pid);

void estimacion_next_burst(t_pcb* pcb);
void agregar_pcb_cola_nuevos(t_pcb* pcb);
t_pcb* sacar_pcb_de_cola_nuevo();

void agregar_pcb_en_cola_listos(t_pcb* pcb);
t_pcb* sacar_pcb_cola_listos();
void mostrar_cola_ready();
void meter_pcb_en_ejecucion(t_pcb* pcb);
t_pcb* sacar_pcb_de_ejecucion();
void liberar_variables();

void agregar_pcb_en_cola_terminados(int*);
void empezar_espera_ready(t_pcb* pcb);
double calcularRR(t_pcb* pcb);
bool mayor_RR(t_pcb* proceso_a, t_pcb* proceso_b);
void ordenar_cola_listos();
void pcb_iniciar_rafaga_ejecucion(t_pcb* pcb);
void actualizar_pcb_ejecucion(t_pcb* pcb);

extern sem_t semMultiprogramacion;
extern sem_t semProcesoNuevo;
extern sem_t semProcesoListo;
#endif
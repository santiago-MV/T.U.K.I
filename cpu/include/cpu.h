#ifndef CPU_H_
#define CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<readline/readline.h>
#include<pthread.h>

#include "utils.h"

void handle_clients(void);
void handle_conection(int*);
void handle_memoria(void);
void enviar_paquete_memoria(t_paquete* paquete);
//int mmu(t_dir_logica_proc* direc_logica, t_segmento* info_segmento);
void execute_in_kernel(t_pcb* pcb, op_code op_code,int socket_cliente);
void execute_instrucciones(t_pcb* pcb, int socket_cliente);
void execute_exit(t_pcb* pcb, int socket_cliente);
void execute_bloq(t_pcb* pcb, int socket_cliente);
void execute_set(t_pcb* pcb, instruction_set* instruccion);
int execute_movout(t_pcb* pcb, instruction_set* instruccion, int socket_kernel);
int execute_movin(t_pcb* pcb, instruction_set* instruccion, int socket);
char* get_reg_value(t_pcb* pcb, char* reg_name);
void guardar_en_reg(t_pcb* pcb, char* contenido, char* reg_name);
void terminar_programa(t_log*, t_config*);
void terminar_conexion(int);

#endif
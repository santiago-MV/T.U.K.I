#ifndef UTILS_H_
#define UTILS_H_

#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<pthread.h>
#include<unistd.h>
#include<netdb.h>
#include<commons/log.h>
#include<commons/config.h>
#include<commons/collections/list.h>
#include<string.h>
#include<assert.h>

typedef enum
{
      HANDSHAKE,
      PAQUETE_INSTRUCCIONES,
      PAQUETE_PCB,
      FIN_PROCESO,
      CREACION_INICIAL_PROCESO,
      CREAR_SEGMENTO,
      BORRAR_SEGMENT,
      OUT_OF_MEMORY,
      SEGMENTO_CREADO,
      COMPACTAR,
      CREAR_ARCHIVO,
      INICIALIZAR_ESTRUCTURAS,
      TABLA_PROCESO,
      ESCRIBIR_ARCHIVO,
      LEER_ARCHIVO,
      TRUNCAR_ARCHIVO,
      SEG_FAULT,
      PEDIDO_LECTURA_CPU,
      PEDIDO_ESCRITURA_CPU,
      PEDIDO_LECTURA_FS,
      PEDIDO_ESCRITURA_FS,
      VALOR_LEIDO,
      OK,
      DIREC_FISICA,
      F_SEEK,
      BUSCAR_ARCHIVO,
      RESPONSE
}op_code;
typedef struct
{
	int size;
	void* stream;
} t_buffer;
typedef struct
{
	op_code codigo_operacion;
	t_buffer* buffer;
} t_paquete;
typedef struct
{
    int id;
    u_int32_t base;
    u_int32_t limite;
} t_segmento;
typedef struct
{
    int pid;
    t_segmento** segmentos;
    int cant_segmentos_actuales;
} t_tabla_proceso;

extern t_log* logger;
extern t_config* config;
extern t_list* lista_tablas;
extern char* ip;
extern char* puerto;
extern int tam_memoria;
extern int tam_segmento_0;
extern int cant_segmentos;
extern int retardo_memoria;
extern int retardo_compactacion;
extern char* algorimo_asignacion;
extern void* memoria;
extern t_list* lista_segmentos_actuales;
extern t_list* lista_tablas;
extern t_list* huecos_libres;

t_log* iniciar_logger(char*,char*);
t_config* iniciar_config(char*);
void iniciar_estructuras_memoria();
void inicializar_estructuras_pcb(int pid, int conexion_kernel);
void* recibir_buffer(int*, int);
int crear_servidor(char*, char*);
int esperar_cliente(int);
void recibir_mensaje(int);
int recibir_operacion(int);
int recibir_eliminar_segmento(int conexion, int* sid);
void compactar_memoria(int conexion_kernel);
int recibir_pedido_lectura(int conexion, int* tamanio_a_leer, int* pid);
char* recibir_pedido_escritura(int conexion, int* dir_fisica, int* pid);
void escribir_en_direccion_fisica(char* valor_a_escribir, int dir_fisica);
void enviar_ok(int socket);
int recibir_entero(int );
int recibir_fin_proceso(int conexion);
void fin_proceso(int pid);
t_tabla_proceso* obtener_tabla_con_pid(int pid, t_list* lista_tablas);
void crear_segmento(int pid, int sid,int tam, int cant_segmentos);
int recibir_crear_segmento(int conexion, int* tam, int* sid);
void enviar_valor_leido(int socket, char* valor_leido);
t_paquete* crear_paquete_con_codigo_op(int);
void agregar_a_paquete(t_paquete *paquete, void *valor, int tamanio);
void enviar_paquete(t_paquete* paquete, int socket_cliente);
void eliminar_paquete(t_paquete* paquete);
void remover_hueco_libre(t_segmento* hueco_libre, int indice);
void print_memoria();
void crear_buffer(t_paquete*);
t_segmento* buscar_hueco_libre(uint32_t tamanio_segmento);
void agregar_entero_a_paquete(t_paquete* paquete,int x);
void eliminar_segmento(int pid, int sid, bool mostrar);
int obtener_tamanio_segmento(t_segmento* segmento);
int recibir_pedido_lectura(int conexion, int* tamanio_a_leer, int* pid);
void leer_direccion_fisica(char* valor_leido,int dir_fisica, int tamanio_a_leer);
#endif /* UTILS_H_ */
#ifndef FILESYSTEM_H_
#define FILESYSTEM_H_



#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<commons/log.h>
#include<commons/collections/list.h>
#include<assert.h>
#include"utils.h"
#include<commons/bitarray.h>
#include<pthread.h>
#include<sys/mman.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<commons/string.h>
#include<fcntl.h>
#include<math.h>
#include<commons/collections/dictionary.h>


int cant_bloques;
int tamanio_bloque;
t_bitarray* bitmap;
void* addr;
void* buffer_bitmap;
char* ip_memoria;
char* puerto_memoria;
int delay;
char* path_FCB;


// ---- utils.h de server ----

t_list* recibir_paquete(int);
void open_archivo(int* ,char* );
void verificar_existencia_archivo(char* );
void read_archivo(char* ,int ,int ,int ,int );
void write_archivo(char* ,int ,int ,int ,int);
void reduce_block(int ,t_config*);
void expand_block(int ,t_config*,int);
void truncate_archivo(char* ,int);
int cpb_get();
void init_bloque_punteros(int ,int ,int[],char*);
void init_bitmap();
void levantar_bitmap();
void init_superBloque();
void init_archivo_bloques();
void init_fileSystem();
int bloque_archivo_get(int ,int ,int* ,int );
void create_fcb(char*);
#endif /* UTILS_H_ */
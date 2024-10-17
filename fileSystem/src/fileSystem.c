#include "fileSystem.h"


t_config* config;
t_log* logger;


// ===============   Handles  ===============


void handle_conection(int* socket_cliente){
	int size;
	int cod_op;
	int tam_nombre;
	char* nombre_archivo;
	int PID;
	int puntero;
	int cantidad_bytes;
	int direccion_fisica;
	int offset;
	
	while(1){
		offset = 0;
		cod_op = recibir_operacion(*socket_cliente);
		void* buffer = recibir_buffer(&size,*socket_cliente);
			switch (cod_op){
				case HANDSHAKE:

					recibir_mensaje(*socket_cliente);
					free(buffer);
					break;

				case ABRIR_ARCHIVO:
					
					memcpy(&tam_nombre,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					nombre_archivo = malloc(tam_nombre+1);
					memcpy(nombre_archivo,(buffer+offset),tam_nombre);
					nombre_archivo[tam_nombre] = '\0';
					
				
					log_info(logger, "Archivo Abierto: %s",nombre_archivo);
					
					
					open_archivo(socket_cliente,nombre_archivo);

					free(nombre_archivo);
					free(buffer);
					break;

				case CREAR_ARCHIVO:

					memcpy(&tam_nombre,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					nombre_archivo = malloc(tam_nombre+1);
					memcpy(nombre_archivo,(buffer+offset),tam_nombre);
					nombre_archivo[tam_nombre] = '\0';

					verificar_existencia_archivo(nombre_archivo);
					
					t_paquete* paquete = crear_paquete_con_codigo_op(OK);
					enviar_paquete(paquete,*socket_cliente);
					eliminar_paquete(paquete);
					
					
					log_info(logger, "Crear Archivo: %s",nombre_archivo);
					

					free(nombre_archivo);
					free(buffer);
					break;

				case TRUNCAR_ARCHIVO:

					memcpy(&tam_nombre,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					nombre_archivo = malloc(tam_nombre+1);
					memcpy(nombre_archivo,(buffer+offset),tam_nombre);
					nombre_archivo[tam_nombre] = '\0';
					offset += tam_nombre;
					memcpy(&cantidad_bytes,(buffer+offset),sizeof(int));

					truncate_archivo(nombre_archivo,cantidad_bytes);

					
					log_info(logger, "Truncar Archivo: %s - Tamanio: %d",nombre_archivo,cantidad_bytes);
					
					t_paquete* paquetin = crear_paquete_con_codigo_op(OK);
					enviar_paquete(paquetin,*socket_cliente);
					eliminar_paquete(paquetin);
					
					free(nombre_archivo);
					free(buffer);
					break;

				case LEER_ARCHIVO:
					
					memcpy(&tam_nombre,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					nombre_archivo = malloc(tam_nombre+1);
					memcpy(nombre_archivo,(buffer+offset),tam_nombre);
					nombre_archivo[tam_nombre] = '\0';
					offset += tam_nombre;
					memcpy(&puntero,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&cantidad_bytes,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&direccion_fisica,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&PID,(buffer+offset),sizeof(int));

					
					log_info(logger, "Leer Archivo: %s - Puntero: %d  - Memoria: %d - Tamaño: %d",nombre_archivo,puntero,direccion_fisica,cantidad_bytes);
					

					read_archivo(nombre_archivo,puntero,cantidad_bytes,direccion_fisica,PID);

					t_paquete* paqeuton = crear_paquete_con_codigo_op(OK);
					enviar_paquete(paqeuton,*socket_cliente);
					eliminar_paquete(paqeuton);

					free(nombre_archivo);
					free(buffer);
					break;
				case ESCRIBIR_ARCHIVO:

					memcpy(&tam_nombre,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					nombre_archivo = malloc(tam_nombre+1);
					memcpy(nombre_archivo,(buffer+offset),tam_nombre);
					offset += tam_nombre;
					nombre_archivo[tam_nombre] = '\0';
					memcpy(&puntero,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&cantidad_bytes,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&direccion_fisica,(buffer+offset),sizeof(int));
					offset += sizeof(int);
					memcpy(&PID,(buffer+offset),sizeof(int));

					
					log_info(logger, "Escribir Archivo: %s - Puntero: %d  - Memoria: %d - Tamaño: %d",nombre_archivo,puntero,direccion_fisica,cantidad_bytes);
					
					
					write_archivo(nombre_archivo,puntero,cantidad_bytes,direccion_fisica,PID);
					t_paquete* ok = crear_paquete_con_codigo_op(OK);
					enviar_paquete(ok,*socket_cliente);
					eliminar_paquete(ok);
					
					free(nombre_archivo);
					free(buffer);
					break;
				case -1:
					log_info(logger, "El modulo se desconecto.");
					free(buffer);
					return;
				default:
					log_info(logger,"Operacion desconocida. No quieras meter la pata");
					free(buffer);
					break;
			}
	}
}

void handle_clients(void)
{
	char* puerto_escucha;
	char* ip_fs;
	puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
	ip_fs = config_get_string_value(config,"IP_FILESYSTEM");

	int server_fs = crear_servidor(ip_fs,puerto_escucha);

	if(server_fs == -1) {
		perror("Error al iniciar el servidor");
		log_warning(logger, "fileSystem.c_handle_clients --> Error al iniciar el servidor");
		exit(-2);
	}

	while(1){
		
		log_info(logger, "Servidor listo para recibir al cliente...\n");

		int socket_cliente = esperar_cliente(server_fs);

		int *socket_ptr = malloc(sizeof(int));
		*socket_ptr = socket_cliente;
		pthread_t hilo;
		pthread_create(&hilo, NULL, (void*) handle_conection, (void*) socket_ptr);
	}

}

// fileSystem cliente de memoria -- con handle_memoria me conecto a memoria



// =============== Funciones ===============

void open_archivo(int* socket_cliente,char* nombre_archivo){
	char* string;
	string = string_from_format("%s/%s", path_FCB,nombre_archivo);
	FILE* archivo = fopen(string,"r");
	free(string);

	if(archivo == NULL){
		t_paquete* paquete = crear_paquete_con_codigo_op(NO_EXISTE_ARCHIVO);
		enviar_paquete(paquete,*socket_cliente);
		eliminar_paquete(paquete);
		return;
	}

	//esta bien usar el enviar_mensaje? :D
	t_paquete* paquete = crear_paquete_con_codigo_op(OK);
	enviar_paquete(paquete,*socket_cliente);
	eliminar_paquete(paquete);
	fclose(archivo);
}

void verificar_existencia_archivo(char* nombre_archivo){

	char* string;
	string = string_from_format("%s/%s", path_FCB,nombre_archivo);
	FILE* archivo = fopen(string,"r");
	free(string);
	if(archivo == NULL){
		create_fcb(nombre_archivo);
		return;
	}

	fclose(archivo);
}


void read_archivo(char* nombre_archivo,int puntero,int cantidad,int direccion_fisica,int PID){
	
	void* buffer = malloc(cantidad);
	int cpb;
	cpb = cpb_get();
	int punteros[cpb];
	int offset = 0;
	int indice;
	int byte_por_leer = cantidad;
	char* stringAux;

	stringAux = string_from_format("%s/%s", path_FCB,nombre_archivo);
	t_config* archivoFCB = iniciar_config(stringAux);
	free(stringAux);

	int puntero_directo = config_get_int_value(archivoFCB,"PUNTERO_DIRECTO");
	int puntero_indirecto = config_get_int_value(archivoFCB,"PUNTERO_INDIRECTO");
	
	memcpy(punteros,(addr + (puntero_indirecto*tamanio_bloque)),tamanio_bloque);
	sleep(delay/1000);
	
	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_indirecto),puntero_indirecto);
	
	//Busco que bloque contiene la posicion del puntero y calculo el desplazamiento dentro del mismo
	int bloque = puntero/tamanio_bloque;
	int desplazamiento = puntero - (tamanio_bloque*bloque);

	indice = bloque-1;
	//Leo todo y lo cargo en el buffer
	while (1){
		if (bloque == 0){
			indice = 0;
			offset += (tamanio_bloque-desplazamiento);

			if(byte_por_leer < tamanio_bloque){
				memcpy(buffer,(addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),byte_por_leer);
				sleep(delay/1000);
				
				log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_directo),puntero_directo);
				
				break;
			}

			memcpy(buffer,(addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),offset);
			sleep(delay/1000);
			
			log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_directo),puntero_directo);
			
			byte_por_leer -= offset;
			desplazamiento = 0;
			indice++;
		}
		if (byte_por_leer > tamanio_bloque){

			bloque = punteros[indice];

			memcpy((buffer + offset),(addr + ((bloque*tamanio_bloque)+desplazamiento)),tamanio_bloque);
			sleep(delay/1000);
			
			log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,bloque),bloque);
			
			indice++;
			offset += tamanio_bloque;
			byte_por_leer -= tamanio_bloque;
			continue;
		}
		bloque = punteros[indice];

		memcpy((buffer + offset),(addr+ ((bloque*tamanio_bloque)+desplazamiento)),byte_por_leer);
		sleep(delay/1000);
		
		log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,bloque),bloque);
		
		break;
	}
	
	//Envio la direccion fisica y los datos leidos a memoria

	t_paquete* paquete_memoria = crear_paquete_con_codigo_op(PEDIDO_ESCRITURA_FS);

	agregar_entero_a_paquete(paquete_memoria,PID);
	agregar_entero_a_paquete(paquete_memoria,direccion_fisica);
	agregar_a_paquete(paquete_memoria,buffer,cantidad);

	int conexion_memoria = crear_conexion(ip_memoria,puerto_memoria);

	enviar_paquete(paquete_memoria,conexion_memoria);
	eliminar_paquete(paquete_memoria);

	recibir_operacion(conexion_memoria);

	close(conexion_memoria);
	config_destroy(archivoFCB);
	free(buffer);
}

void write_archivo(char* nombre_archivo,int puntero,int cantidad,int direccion_fisica,int PID){

	void* buffer = malloc(cantidad);
	int cpb;
	cpb = cpb_get();
	int punteros[cpb];
	int offset = 0;
	int size;
	int byte_por_escribir = cantidad;
	char* stringAux;
	stringAux = string_from_format("%s/%s", path_FCB,nombre_archivo);
	t_config* archivoFCB = iniciar_config(stringAux);
	free(stringAux);
	int puntero_directo = config_get_int_value(archivoFCB,"PUNTERO_DIRECTO");
	int puntero_indirecto = config_get_int_value(archivoFCB,"PUNTERO_INDIRECTO");

	memcpy(punteros,(addr + (puntero_indirecto*tamanio_bloque)),tamanio_bloque);
	sleep(delay/1000);
	
	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_indirecto),puntero_indirecto);
	
	//Envio a memoria la direccion fisica y tamanio del datos a leer, y los recibo

	t_paquete* paquete_memoria = crear_paquete_con_codigo_op(PEDIDO_LECTURA_FS);

	agregar_entero_a_paquete(paquete_memoria,PID);
	agregar_entero_a_paquete(paquete_memoria,direccion_fisica);
	agregar_entero_a_paquete(paquete_memoria,cantidad);
	
	int conexion_memoria = crear_conexion(ip_memoria,puerto_memoria);

	enviar_paquete(paquete_memoria,conexion_memoria);
	eliminar_paquete(paquete_memoria);
	recibir_operacion(conexion_memoria);
	void* buff = recibir_buffer(&size,conexion_memoria);
	memcpy(buffer,buff+sizeof(int),cantidad);
	
	close(conexion_memoria);

	//Busco que bloque contiene la posicion del puntero y busco el desplazamiento dentro del mismo
	int bloque = floor(puntero/tamanio_bloque);
	int desplazamiento = puntero - (tamanio_bloque*bloque);
	int indice = bloque-1;
	
	//Escribo los datos a memoria
	while (1){
		if (bloque == 0){
			indice = 0;
			offset += (tamanio_bloque-desplazamiento);

			if(byte_por_escribir<tamanio_bloque){

				memcpy((addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),buffer,byte_por_escribir);
				msync((addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),byte_por_escribir,MS_INVALIDATE);
				sleep(delay/1000);
				
				log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_directo),puntero_directo);	
				
				break;
			}

			memcpy((addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),buffer,offset);
			msync((addr + ((puntero_directo*tamanio_bloque)+desplazamiento)),offset,MS_INVALIDATE);
			sleep(delay/1000);
			
			log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,puntero_directo),puntero_directo);
			
			
			
			byte_por_escribir -= offset;
			desplazamiento = 0;
			
		}
		if (byte_por_escribir > tamanio_bloque){

			bloque = punteros[indice];
			
			memcpy((addr + ((bloque*tamanio_bloque)+desplazamiento)),(buffer+ offset),tamanio_bloque);
			msync((addr + ((bloque*tamanio_bloque)+desplazamiento)),tamanio_bloque,MS_INVALIDATE);
			sleep(delay/1000);
			
			log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,bloque),bloque);
			
			
			indice++;
			offset += tamanio_bloque;
			byte_por_escribir -= tamanio_bloque;
			continue;
		}
		bloque = punteros[indice];
		
		memcpy((addr+((bloque*tamanio_bloque)+desplazamiento)),(buffer + offset),byte_por_escribir);
		msync((addr + ((bloque*tamanio_bloque)+desplazamiento)),byte_por_escribir,MS_INVALIDATE);
		sleep(delay/1000);
		
		log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,punteros,bloque),bloque);
		
		
		
		break;
	}


	config_destroy(archivoFCB);
	free(buffer);
	free(buff);

}


void reduce_block(int tam,t_config* archivoFCB){
	
	int cpb = cpb_get();
	int j = cpb-1;
	int bloque_punteros[cpb];
	char* nombre_archivo = config_get_string_value(archivoFCB,"NOMBRE_ARCHIVO");
	int puntero_indirecto = config_get_int_value(archivoFCB,"PUNTERO_INDIRECTO");
	int puntero_directo = config_get_int_value(archivoFCB,"PUNTERO_DIRECTO");
	char* stringAux;
	init_bloque_punteros(puntero_indirecto,cpb,bloque_punteros,nombre_archivo);

	for(int i = 0; i<tam ; i++){

		while(bloque_punteros[j] == 0){ //Recorro el array de punteros buscando la ultima posicion ocupada
			if (j == -1){
				break;
			}
			j--;
		}
		if(j!=-1){
			int bloque_a_liberar = bloque_punteros[j];
			bloque_punteros[j] = 0;
			bitarray_clean_bit(bitmap,bloque_a_liberar);
			
			log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",bloque_a_liberar,bitarray_test_bit(bitmap,bloque_a_liberar));
			
			if (j == 0){
				bitarray_clean_bit(bitmap,puntero_indirecto);
				
				log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",puntero_indirecto,bitarray_test_bit(bitmap,puntero_indirecto));
				
				stringAux = string_from_format("%d",0);
				config_set_value(archivoFCB,"PUNTERO_INDIRECTO",stringAux);
				free(stringAux);
			}
			continue;
		}
		//Si todo el bloque de punteros esta vacio, libero los otros dos punteros para dejarlo en 0 (dudoso)
		
		bitarray_clean_bit(bitmap,puntero_directo);
		
		log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",puntero_directo,bitarray_test_bit(bitmap,puntero_directo));
		
		stringAux = string_from_format("%d",0);
		config_set_value(archivoFCB,"PUNTERO_DIRECTO",stringAux);
		free(stringAux);
		
		break;
	}
	int offset = puntero_indirecto*tamanio_bloque;

	msync(buffer_bitmap,sizeof(buffer_bitmap),MS_INVALIDATE);

	memcpy((addr + offset),bloque_punteros,tamanio_bloque);
	msync((addr + offset),tamanio_bloque,MS_INVALIDATE);
	sleep(delay/1000);
	
	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,bloque_punteros,puntero_indirecto),puntero_indirecto);
	
	

	config_save(archivoFCB);

}

void expand_block(int tam,t_config *archivoFCB,int tamanio_archivo){

	int j = 0;
	int t = 0;
	int cpb = cpb_get();
	int bloque_punteros[cpb];
	char* string_para_logs;
	char* nombre_archivo = config_get_string_value(archivoFCB,"NOMBRE_ARCHIVO");
	int puntero_indirecto = config_get_int_value(archivoFCB,"PUNTERO_INDIRECTO");
	int puntero_directo = config_get_int_value(archivoFCB,"PUNTERO_DIRECTO");

	init_bloque_punteros(puntero_indirecto,cpb,bloque_punteros,nombre_archivo);

	for (int i = 0; i<tam;i++){

		while (bitarray_test_bit(bitmap,j))
		{
			
			log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",j,bitarray_test_bit(bitmap,j));
			
			j++;		
		}
		
		log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",j,bitarray_test_bit(bitmap,j));
		
		if (puntero_directo == 0 && tamanio_archivo == 0){ //Si no tengo un primer bloque lo asigno
			puntero_directo = j;
			string_para_logs = string_from_format("%d",j);
			config_set_value(archivoFCB,"PUNTERO_DIRECTO",string_para_logs);
			free(string_para_logs);
			bitarray_set_bit(bitmap,j);
			
			log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",j,bitarray_test_bit(bitmap,j));
			
			
			j++;
			tamanio_archivo = 1;
			continue;
		}
		if(puntero_indirecto==0){ //Si no tengo bloque de punteros lo asigno
			puntero_indirecto = j;
			string_para_logs = string_from_format("%d",j);
			config_set_value(archivoFCB,"PUNTERO_INDIRECTO",string_para_logs);
			free(string_para_logs);
			bitarray_set_bit(bitmap,j);
			
			log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",j,bitarray_test_bit(bitmap,j));
			
			
			j++;
			i--; //No cuenta como bloque agregado, xq no aumenta el tamanio del archivo
			continue;
		}
		while(bloque_punteros[t] != 0){
			t++; //Busco la primera posicion libre en el bloque de punteros
		}
		bloque_punteros[t] = j;
		bitarray_set_bit(bitmap,j);
		log_info(logger, "Acceso a Bitmap - Bloque: %d - Estado: %d",j,bitarray_test_bit(bitmap,j));
		
		j++;
	}
	
	msync(buffer_bitmap,sizeof(buffer_bitmap),MS_INVALIDATE);

	//Copio el array de punteros al bloque indirecto y le hago un sync en memoria
	if(puntero_indirecto!=0){
		int offset = puntero_indirecto*tamanio_bloque;
		memcpy((addr + offset),bloque_punteros,tamanio_bloque);
		msync((addr + offset),tamanio_bloque,MS_INVALIDATE);
		sleep(delay/1000);
		
		log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,bloque_archivo_get(puntero_directo,puntero_indirecto,bloque_punteros,puntero_indirecto),puntero_indirecto);
		
	}
	
	config_save(archivoFCB);
}

void truncate_archivo(char* nombre_archivo,int cantidad){
	// Asumimos que recibimos el tamanio en bytes
	int nuevo_tamanio;
	char* stringAux;
	stringAux = string_from_format("%s/%s", path_FCB,nombre_archivo);
	t_config* fcb = iniciar_config(stringAux);
	free(stringAux);
	int tamanioArchivo;

	tamanioArchivo = config_get_int_value(fcb,"TAMANIO_ARCHIVO");

	if(tamanioArchivo == cantidad){
		config_destroy(fcb);
		return;
	}

	if (cantidad >= tamanioArchivo){
		nuevo_tamanio = ceil((cantidad-tamanioArchivo)/tamanio_bloque);
		expand_block(nuevo_tamanio,fcb,tamanioArchivo);
		
		stringAux = string_from_format("%d",cantidad);
		config_set_value(fcb,"TAMANIO_ARCHIVO",stringAux);
		free(stringAux);
		config_save(fcb);
		config_destroy(fcb);		
		return;
	}
	nuevo_tamanio = ceil((tamanioArchivo-cantidad)/tamanio_bloque);
	reduce_block(nuevo_tamanio,fcb);
	stringAux = string_from_format("%d",cantidad);
	config_set_value(fcb,"TAMANIO_ARCHIVO",stringAux);
	free(stringAux);
	config_save(fcb);
	config_destroy(fcb);
	return;
}

int cpb_get(){
	return tamanio_bloque/4; // Se utilizan int para representar los punteros que tiene un tam de 4 bytes
}

int bloque_archivo_get(int puntero_directo,int puntero_indirecto,int* punteros,int puntero_duda){
	int cpb = cpb_get();
	if (puntero_duda == puntero_directo){
		return 1;
	}
	if (puntero_duda == puntero_indirecto){
		return 2;
	}
	for (int i = 0;i<cpb;i++){
		if (puntero_duda == punteros[i]){
			return i+3;
		}
	}
	return 0;
}

// ===============   Inits 	 ===============

void init_bloque_punteros(int puntero_indirecto,int cpb,int array_punteros[],char* nombre_archivo){
	if (puntero_indirecto == 0){
		for(int i = 0;i<cpb;i++){
			array_punteros[i] = 0;
		}
		return;
	}

	memcpy(array_punteros,(addr+ (puntero_indirecto*tamanio_bloque)),tamanio_bloque);
	sleep(delay/1000);
	
	log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d",nombre_archivo,2,puntero_indirecto);
	

}

void init_bitmap(){
	
	char* pathBitmap;
	pathBitmap = config_get_string_value(config,"PATH_BITMAP");
	int archivo_bitmap = open(pathBitmap,O_RDWR|O_CREAT,0644);

	
	size_t tam_bitmap = ceil(cant_bloques/8); // Tamaño del bitmap en bytes (un bit por bloque)
	
	ftruncate(archivo_bitmap,tam_bitmap);

	buffer_bitmap = mmap(NULL,tam_bitmap,PROT_WRITE|PROT_READ,MAP_SHARED,archivo_bitmap,0);
	bitmap = bitarray_create_with_mode(buffer_bitmap,tam_bitmap,LSB_FIRST);

	for (int i = 0; i<tam_bitmap;i++){
		bitarray_clean_bit(bitmap,i);
	}
	msync(buffer_bitmap,tam_bitmap,MS_INVALIDATE);
	close(archivo_bitmap);

}

void levantar_bitmap(){
	
	char* pathBitmap;
	pathBitmap = config_get_string_value(config,"PATH_BITMAP");

	int archivo_bitmap = open(pathBitmap,O_RDWR);
	if(archivo_bitmap == -1){
		init_bitmap(); // falta log
		return;
	}

	size_t tam_bitmap = ceil(cant_bloques/8); // Tamaño del bitmap en bytes (un bit por bloque)

	// Mapea los bits del archivo al buffer
	buffer_bitmap = mmap(NULL,tam_bitmap,PROT_WRITE|PROT_READ,MAP_SHARED,archivo_bitmap,0);
	// Crea el bitmap con los datos del buffer
	bitmap = bitarray_create_with_mode(buffer_bitmap,tam_bitmap,LSB_FIRST);

	close(archivo_bitmap);

}

void init_superBloque(){ 
	t_config* superbloque_config;
	char* pathSuperBloque;

	pathSuperBloque = config_get_string_value(config,"PATH_SUPERBLOQUE");
	superbloque_config = iniciar_config(pathSuperBloque);
	
	cant_bloques = config_get_int_value(superbloque_config, "BLOCK_COUNT");
	tamanio_bloque = config_get_int_value(superbloque_config, "BLOCK_SIZE");

	config_destroy(superbloque_config);
}

void init_archivo_bloques(){
	
	char* pathBloques;
	pathBloques = config_get_string_value(config,"PATH_BLOQUES");
	
	int tamanioArchivo = cant_bloques * tamanio_bloque;

	int blocks = open(pathBloques,O_RDWR|O_CREAT,0644);
	struct stat s;
	fstat(blocks,&s);
	size_t size_blocks = s.st_size;

	if (size_blocks == 0){
		ftruncate(blocks,tamanioArchivo);
	}

	addr = mmap(NULL,tamanioArchivo,PROT_WRITE|PROT_READ,MAP_SHARED,blocks,0);

	close(blocks);

}

void init_fileSystem(){
	
	// ------------- CLIENTE ----------------
	
	ip_memoria = config_get_string_value(config, "IP_MEMORIA");
	puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
	delay = config_get_int_value(config,"RETARDO_ACCESO_BLOQUE");
	path_FCB = config_get_string_value(config,"PATH_FCB");
	
	mkdir(path_FCB,0777);

	init_superBloque();
	levantar_bitmap();
	init_archivo_bloques();
	
	

	// -------------  SERVER  --------------

	log_info(logger, "FileSystem listo para recibir a Kernel");

	pthread_t server_thread;
	pthread_create(&server_thread, NULL,(void*) handle_clients, NULL);

	pthread_join(server_thread,NULL);

	bitarray_destroy(bitmap);
	terminar_programa(logger, config);
	
	
}

int main(void){

	logger = iniciar_logger("./config/fileSystem.log","FILESYSTEM");
	config = iniciar_config("./config/fileSystem.config");


	init_fileSystem();

	return EXIT_SUCCESS;
}

//Funciones FCB


void create_fcb(char* filename){
    char* fcb_path = string_from_format("%s/%s",path_FCB,filename);
    char* zero = "0";
    t_config* config = malloc(sizeof(t_config));
    config->path = string_duplicate(fcb_path);
    config->properties = dictionary_create();
    dictionary_put(config->properties, "NOMBRE_ARCHIVO", string_duplicate(filename));
    dictionary_put(config->properties, "TAMANIO_ARCHIVO", string_duplicate(zero));
    dictionary_put(config->properties, "PUNTERO_DIRECTO", string_duplicate(zero));
    dictionary_put(config->properties, "PUNTERO_INDIRECTO", string_duplicate(zero));
    config_save(config);
    config_destroy(config);
    free(fcb_path);
}
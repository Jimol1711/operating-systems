#include <stdio.h>
#include <pthread.h>

void *thread(void *ptr) { 
	char* nombre  = (char*) ptr;     // Castear argumento
	printf("Thread - %s\n", nombre); // Trabajo en paralelo
	return NULL;                     // Retorno
}

int main() {
	pthread_t pid_1, pid_2; // Guardar PID de los threads lanzados
	char* nombre_1 = "primero";
	char* nombre_2 = "segundo";

	pthread_create(&pid_1, NULL, thread, nombre_1); // lanzar thread1
	pthread_create(&pid_2, NULL, thread, nombre_2); // lanzar thread2

	pthread_join(pid_1, NULL);  // esperar thread 1
	pthread_join(pid_2, NULL);  // esperar thread 2
	return 0;
}


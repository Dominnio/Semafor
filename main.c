#include <time.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

//#define MAX 5
/*
Semafor 0 - liczba wolnych pozycji bufora (opuszczany przy produkcji, podnoszony przy konsumpcji)
Semafor 1 - liczba zajętych pozycji bufora (opuszczany przy konsumpcji, podnoszony przy produkcji)
Semafor 2 - dostęp do pola przechowującego indeks do zapisu (opuszczony - nie wolno)
Semafor 3 - dostęp do pola przechowującego indeks do odczytu (opuszczony - nie wolno)

Inicjowanie :
Semafor 0 - MAX
Semafor 1 - 0
Semafor 2 - 1
Semafor 3 - 1

Na końcu jest zwolnienie segmentu pamięci współdzielonej.

shm - share memory
sem - semafor
*/


static struct sembuf buf;
void podnies(int semid, int semnum);
void opusc(int semid, int semnum);
void  producent(int number_of_products,int producent_number);
void  konsument(int number_of_products, int number_of_producers);

void delay(float msec){
    clock_t end = msec + clock();
    while(end > clock());
}

void podnies(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = 1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Podnoszenie semafora");
        exit(1);
    }
}

void opusc(int semid, int semnum){
    buf.sem_num = semnum;
    buf.sem_op = -1;
    buf.sem_flg = 0;
    if (semop(semid, &buf, 1) == -1){
        perror("Opuszczenie semafora");
        exit(1);
    }
}


int MAX;
int main(int argc, char* argv[]){
    int pid;
    if(argc<4){
        printf("Za mala liczba argumentow!");
     }
    //int number_of_producers = 3; // np. 4
    //MAX = 8; // np. 8
    //int number_of_products = 45;  // np. 15

    int number_of_producers = atoi(argv[1]);
    MAX = atoi(argv[2]);
    int number_of_products = atoi(argv[3]);

    pid = fork();
    if(pid == 0){
        int pid_p; int i;
        for(i=0;i<number_of_producers;i++){
            pid_p = fork();
            if(pid_p == 0 ){
                producent(number_of_products,i);
                break;
            }else{
                continue;
            }
        }
    }else{
       konsument(number_of_products,number_of_producers);
    }
    return 0;
}

void  producent(int number_of_products,int producent_number)
{
    int shmid, semid, i;
	int *buf;

 	shmid = shmget(45282, (MAX+2)*sizeof(int), IPC_CREAT|0600);
	if (shmid == -1){
		perror("Utworzenie segmentu pamieci wspoldzielonej");
 		exit(1);
	}

 	buf = (int*)shmat(shmid, NULL, 0);
	if (buf == NULL){
		perror("Przylaczenie segmentu pamieci wspoldzielonej");
 		exit(1);
	}

    #define indexZ buf[MAX]
    #define indexO buf[MAX+1]

 	semid = semget(45282, 4, IPC_CREAT|IPC_EXCL|0600);
	if (semid == -1){
		semid = semget(45282, 4, 0600);
		if (semid == -1){
			perror("Utworzenie tablicy semaforow");
			exit(1);
 		}
	}
	else{
 		indexZ = 0;
		indexO = 0;
		if (semctl(semid, 0, SETVAL, (int)MAX) == -1){
 			perror("Nadanie wartosci semaforowi 0");
			exit(1);
		}
 		if (semctl(semid, 1, SETVAL, (int)0) == -1){
			perror("Nadanie wartosci semaforowi 1");
			exit(1);
 		}
		if (semctl(semid, 2, SETVAL, (int)1) == -1){
			perror("Nadanie wartosci semaforowi 2");
 			exit(1);
		}
		if (semctl(semid, 3, SETVAL, (int)1) == -1){
 			perror("Nadanie wartosci semaforowi 3");
			exit(1);
		}
 	}

    float percent;
    float f;
    int delay_t;
    int delay_tf;
    srand(time(NULL)+producent_number);
    delay_t = rand()%10 + 1;
    delay_tf = delay_t;
    delay_tf = delay_tf/10;
    for (i=0; i<number_of_products; i++){
 		opusc(semid, 0);
		opusc(semid, 2);
		buf[indexZ] = i;
		f = i+1;
		delay(delay_tf);
		percent = f*100/number_of_products;
		if((i+1)%(number_of_products/5) == 0)
		printf("Producent: %d ## Czas produkcji: %d ## Numer towaru: %d ## Wykonano: %.2f%%\n", producent_number+1,delay_t,i+1, percent);
		indexZ = (indexZ+1)%MAX;
		podnies(semid, 2);
		podnies(semid, 1);
 	}

	shmctl(shmid,IPC_RMID,NULL);
}

void  konsument(int number_of_products, int number_of_producers)
{
 	int shmid, semid, i;
	int *buf;

 	shmid = shmget(45282, (MAX+2)*sizeof(int), IPC_CREAT|0600);
	if (shmid == -1){
		perror("Utworzenie segmentu pamieci wspoldzielonej");
 		exit(1);
	}

 	buf = (int*)shmat(shmid, NULL, 0);
	if (buf == NULL){
		perror("Przylaczenie segmentu pamieci wspoldzielonej");
 		exit(1);
	}

    #define indexZ buf[MAX]
    #define indexO buf[MAX+1]

 	semid = semget(45282, 4, IPC_CREAT|IPC_EXCL|0600);
	if (semid == -1){
		semid = semget(45282, 4, 0600);
 		if (semid == -1){
			perror("Utworzenie tablicy semaforow");
			exit(1);
 		}
   	}
	else{
 		indexZ = 0;
		indexO = 0;
		if (semctl(semid, 0, SETVAL, (int)MAX) == -1){
 			perror("Nadanie wartosci semaforowi 0");
 			exit(1);
		}
 		if (semctl(semid, 1, SETVAL, (int)0) == -1){
			perror("Nadanie wartosci semaforowi 1");
			exit(1);
 		}
		if (semctl(semid, 2, SETVAL, (int)1) == -1){
			perror("Nadanie wartosci semaforowi 2");
 			exit(1);
		}
		if (semctl(semid, 3, SETVAL, (int)1) == -1){
 			perror("Nadanie wartosci semaforowi 3");
			exit(1);
		}
	 }

        srand(time(NULL));
		for (i=0; i<number_of_producers * number_of_products; i++){
		opusc(semid, 1);
		opusc(semid, 3);
		delay(rand()%2);
		if((i+1)%(number_of_producers*number_of_products/10) == 0)
		printf("Konsumpcja numer: %d\n", i+1);
 		indexO = (indexO+1)%MAX;
		podnies(semid, 3);
 		podnies(semid, 0);
	}
	shmctl(shmid,IPC_RMID,NULL);
}

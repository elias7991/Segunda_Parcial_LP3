#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <termio.h>
#include <sys/ioctl.h>


#define STDINFD  0
#undef getc

//Agregar elementos a la cola: Listo
//Sacar elementos de la cola:ReLizado

#define TIEMPO 1
#define TO_RIGHT 1
#define TO_LEFT 0
#define MAX_CARRIL 3
#define TIME_MOV 1
#define MAX_AUTOS_PASAN 4

//Strtuct para pasar la direccion al hilo de los autos esperando
typedef struct {
    int direccion;
}param;

//Struct para representar un vehiculo
typedef struct{
    int direccion; //izquierda o derecha
    char descripcion[7]; //nombre del vehiculo "auto**"
}vehiculo;

typedef struct espacio{
    vehiculo v;
    struct espacio* sig;
}nodo; //nodo para colas

typedef struct {
    nodo* primero;
    nodo* ultimo;
}cola;

typedef struct {
    cola *col;
    vehiculo v;
}paramAdd;

typedef struct {
    int direccion;
}paramDecol;

/*
Cuando el contador pasan por ejemplo llega a 4 entonces 
sabes que tenes que pasarle ahora el token a la otra dirección para
ya no encolar vehiculos de la misma direccion y cerar para volver
a contar
*/
/*
typedef struct {
  vehiculo* carril[MAX_CARRIL];
  int pasan;
}puente;
*/

pthread_mutex_t colDer;
pthread_mutex_t colIzq;
pthread_mutex_t crearAuto;

vehiculo* carril[MAX_CARRIL]; //vehiculos en el carril
int token = TO_RIGHT; //variable global para definir quienes pasan, por defecto los que van a la derecha empiezan
cola vehiculoToDerecha; //cola para vehiculos a la derecha
cola vehiculoToIzquierda; //cola para vehiculos a la izquierda
int cantidad=0; //cantidad de vehiculos creados
int whaitToRight = 0;
pthread_mutex_t lockWhaitToRight;
int whaitToLeft = 0;
pthread_mutex_t lockWhaitToLeft;
int stop = 0; //variable para detener el programa
int lleno = 0; //indica si hay autos en el carril
char cmd[20];

void *agregar_vehiculo(void *paramAdd);
vehiculo crear_vehiculo(int direccion);
void movimiento_carril( vehiculo* vehiculoEntrante, int direccion);
vehiculo *decolar(void *Decol);
void *thread(void *v);
void *start(void *arg);
char inkey();

int main(){
    /*if (signal (SIGINT, nada) == SIG_ERR)
	{
		perror ("No se puede cambiar signal");
	}*/
    vehiculoToDerecha.primero=NULL;
    vehiculoToDerecha.ultimo=NULL;
    vehiculoToIzquierda.primero=NULL;
    vehiculoToIzquierda.ultimo=NULL;

    pthread_t *t_start;
    pthread_t *t_add_right;
    pthread_t *t_add_left;
    
    //usamos fgets para leer cadenas con espacio, nos guarda hasta el salto de linea
    while(1){
        char c;
        sprintf(cmd, "");
        do{
            c=inkey();
            if((int)c==127){
                cmd[strlen(cmd)-1]='\0';
                system("clear");
            }else{
                system("clear");
                sprintf(cmd, "%s%c", cmd, c);
            }
            
            printf("%s",cmd);
            
            
            
        }while(c!='\n');
        system("clear");
        if(strcmp(cmd, "start\n") == 0){
            t_start = (pthread_t *) malloc(sizeof(pthread_t));
            pthread_create(t_start, NULL, start, NULL);

        }else if(strcmp(cmd, "car der\n") == 0){

            //Encolar autos a la cola vehiculoDerecha
            t_add_right = (pthread_t *) malloc(sizeof(pthread_t));
            vehiculo nuevoAuto =crear_vehiculo(TO_RIGHT);
            paramAdd* aggVehi = (paramAdd*)malloc(sizeof(paramAdd));
            aggVehi->v = nuevoAuto;
            aggVehi->col = &vehiculoToDerecha;
            pthread_create (t_add_right, NULL, agregar_vehiculo, (void *) aggVehi);

        }else if(strcmp(cmd, "car izq\n") == 0){

            //Encolar autos a la cola vehiculoIzquierda
            t_add_right = (pthread_t *) malloc(sizeof(pthread_t));

            vehiculo nuevoAuto =crear_vehiculo(TO_LEFT);

            paramAdd* aggVehi = (paramAdd*)malloc(sizeof(paramAdd));
            aggVehi->v = nuevoAuto;
            aggVehi->col = &vehiculoToIzquierda;

            pthread_create (t_add_right, NULL, agregar_vehiculo, (void *) aggVehi);
            

        }else if(strcmp(cmd, "status\n") == 0){
            printf("Vehículos que van a la derecha\n");
            nodo *aux = (nodo *)malloc(sizeof(nodo));
            aux = (vehiculoToDerecha.primero);
            while(aux != NULL){
                printf("=> %s\n", aux->v.descripcion);
                aux = aux->sig;
            }
            printf("Vehiculos que van a la izquierda\n");
            aux = (vehiculoToIzquierda.primero);
            while(aux != NULL){
                printf("<= %s\n", aux->v.descripcion);
                aux = aux->sig;
            }
        }else if(strcmp(cmd,"stop\n")==0){
            stop = 1;
            break;
        }else {
            printf("COMANDO INVALIDO\n");
        }


    }

    /*aquí se usaría mecanismos para matar los hilos que se crearon*/
   
}


/*
* Debe de encargarse de la verificación si existen autos en espera
*/
void *thread(void *arg){
    while(1){
        
        param* p = (param *)(arg);
        int direccion = p->direccion;
        //printf("inicio %d\n",direccion);
        cola vehiculos;

        if(direccion==TO_RIGHT){
            pthread_mutex_lock(&colDer);
            vehiculos = vehiculoToDerecha;
        }else if(direccion == TO_LEFT){
            pthread_mutex_lock(&colIzq);
            vehiculos = vehiculoToIzquierda;
        }
        if(vehiculos.primero!=NULL){
            if(direccion==TO_RIGHT){
                //pthread_mutex_lock(&lockWhaitToRight);
                whaitToRight =1;
                //pthread_cond_wait(&cv, &lockWhaitToRight); //no volver a colsultar hasta que cambie de direccion el Token o se saque un auto
            }
            if(direccion==TO_LEFT){
                //pthread_mutex_lock(&lockWhaitToRight);
                whaitToLeft =1;
                //pthread_cond_wait(&cv, &lockWhaitToRight); //no volver a colsultar hasta que cambie de direccion el Token o se saque un auto
            }
        }else{
            if(direccion==TO_RIGHT){
                //pthread_mutex_lock(&lockWhaitToRight);
                whaitToRight =0;
                //pthread_cond_wait(&cv, &lockWhaitToRight); //no volver a colsultar hasta que cambie de direccion el Token o se saque un auto
            }
            if(direccion==TO_LEFT){
                //pthread_mutex_lock(&lockWhaitToRight);
                whaitToLeft =0;
                //pthread_cond_wait(&cv, &lockWhaitToRight); //no volver a colsultar hasta que cambie de direccion el Token o se saque un auto
            }
        }
        if(direccion==TO_RIGHT){
            pthread_mutex_unlock(&colDer);
        }else if(direccion == TO_LEFT){
            pthread_mutex_unlock(&colIzq);
        }
        //printf("fin %d\n",direccion);
        sleep(1);
    }
}

void movimiento_carril(vehiculo* vehiculoEntrante, int direccion){
    if(direccion==TO_RIGHT){
        carril [MAX_CARRIL-1]=NULL; //ultimo auto a la derecha sale
        for(int i = MAX_CARRIL - 1 ; i > 0 ; i--){
            carril [i]=carril[i-1]; //vehiculo mueve a la derecha
        }
        carril [0]=vehiculoEntrante;
    }else if(direccion==TO_LEFT){
        carril [0]=NULL; //ultimo auto a la izquierda sale
        for(int i = 0 ; i < MAX_CARRIL-1 ; i++){
            carril [i]=carril[i+1]; //vehiculo mueve a la izquierda
        }
        carril [MAX_CARRIL-1]=vehiculoEntrante;
    }
    if(carril[0]!=NULL || carril[1]!=NULL || carril[2]!=NULL){
        lleno =1;
    }else{
        lleno = 0;
    }
    if(carril[0]==NULL && carril[1]==NULL && carril[2]==NULL){
        printf("===========================================\n");
        printf("===========================================\n");
        printf("=======vacio=======vacio=======vacio=======\n");
        printf("===========================================\n");
        printf("===========================================\n");
    } else if(carril[0]==NULL && carril[1]==NULL && carril[2]!=NULL) {
        if(carril[2]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("==============================>>>>>>>>=====\n");
            printf("=======vacio=======vacio======>%s>=====\n",carril[2]->descripcion);
            printf("==============================>>>>>>>>=====\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("==============================<<<<<<<<=====\n");
            printf("=======vacio=======vacio======<%s<=====\n",carril[2]->descripcion);
            printf("==============================<<<<<<<<=====\n");
            printf("===========================================\n");
        }
    } else if(carril[0]==NULL && carril[1]!=NULL && carril[2]!=NULL) {
        if(carril[1]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("================>>>>>>>>======>>>>>>>>=====\n");
            printf("=====vacio======>%s>======>%s>=====\n",carril[1]->descripcion,carril[2]->descripcion);
            printf("================>>>>>>>>======>>>>>>>>=====\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("================<<<<<<<<======<<<<<<<<=====\n");
            printf("=====vacio======<%s<======<%s<=====\n",carril[1]->descripcion,carril[2]->descripcion);
            printf("================<<<<<<<<======<<<<<<<<=====\n");
            printf("===========================================\n");
        }
    } else if(carril[0]!=NULL && carril[1]!=NULL && carril[2]!=NULL) {
        if(carril[1]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("==>>>>>>>>======>>>>>>>>======>>>>>>>>=====\n");
            printf("==>%s>======>%s>======>%s>=====\n",carril[0]->descripcion,carril[1]->descripcion,carril[2]->descripcion);
            printf("==>>>>>>>>======>>>>>>>>======>>>>>>>>=====\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("==<<<<<<<<======<<<<<<<<======<<<<<<<<=====\n");
            printf("==<%s<======<%s<======<%s<=====\n",carril[0]->descripcion,carril[1]->descripcion,carril[2]->descripcion);
            printf("==<<<<<<<<======<<<<<<<<======<<<<<<<<=====\n");
            printf("===========================================\n");
        }
    } else if(carril[0]!=NULL && carril[1]!=NULL && carril[2]==NULL) {
        if(carril[1]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("==>>>>>>>>======>>>>>>>>===================\n");
            printf("==>%s>======>%s>======vacio========\n",carril[0]->descripcion,carril[1]->descripcion);
            printf("==>>>>>>>>======>>>>>>>>===================\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("==<<<<<<<<======<<<<<<<<===================\n");
            printf("==<%s<======<%s<======vacio========\n",carril[0]->descripcion,carril[1]->descripcion);
            printf("==<<<<<<<<======<<<<<<<<===================\n");
            printf("===========================================\n");
        }
    } else if(carril[0]!=NULL && carril[1]==NULL && carril[2]==NULL) {
        if(carril[0]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("==>>>>>>>>=================================\n");
            printf("==>%s>======vacio======vacio===========\n",carril[0]->descripcion);
            printf("==>>>>>>>>=================================\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("==<<<<<<<<=================================\n");
            printf("==<%s<======vacio======vacio===========\n",carril[0]->descripcion);
            printf("==<<<<<<<<=================================\n");
            printf("===========================================\n");
        }
    } else if(carril[0]==NULL && carril[1]!=NULL && carril[2]==NULL) {
        if(carril[1]->direccion == TO_RIGHT) {
            printf("===========================================\n");
            printf("================>>>>>>>>===================\n");
            printf("=====vacio======>%s>======vacio========\n",carril[1]->descripcion);
            printf("================>>>>>>>>===================\n");
            printf("===========================================\n");
        } else {
            printf("===========================================\n");
            printf("================<<<<<<<<===================\n");
            printf("=====vacio======<%s<======vacio========\n",carril[1]->descripcion);
            printf("================<<<<<<<<===================\n");
            printf("===========================================\n");
        }
    }
    /*
    for(int i=0;i<MAX_CARRIL;i++){
        char verAuto[20];
        if(carril[i]!=NULL){
            sprintf(verAuto, "%d auto %s", carril[i]->direccion, carril[i]->descripcion);
            printf("%s\n",verAuto);
        }else
            printf("Vacio\n");
    }
    */
    printf("Vehículos que van a la derecha\n");
            nodo *aux = (nodo *)malloc(sizeof(nodo));
            aux = (vehiculoToDerecha.primero);
            while(aux != NULL){
                printf("=> %s\n", aux->v.descripcion);
                aux = aux->sig;
            }
            printf("Vehiculos que van a la izquierda\n");
            aux = (vehiculoToIzquierda.primero);
            while(aux != NULL){
                printf("<= %s\n", aux->v.descripcion);
                aux = aux->sig;
            }
}

void *start(void *arg){

    pthread_t *esperaToRight; //hilo para controlar si hay autos en espera para ir a la derecha
    pthread_t *esperaToLeft; //hilo para controlar si hay autos en espera para ir a la izquierda

    param *controlDerecha = malloc(sizeof(param));
    param *controlIzquierda = malloc(sizeof(param));
    controlDerecha->direccion = TO_RIGHT;
    controlIzquierda->direccion = TO_LEFT;

    esperaToRight = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(esperaToRight, NULL, thread, (void*)controlDerecha);

    esperaToLeft = (pthread_t *) malloc(sizeof(pthread_t));
    pthread_create(esperaToLeft, NULL, thread, (void*)controlIzquierda);

    int tiempoDesdeUltimoAuto=0;
    while (1){
        sleep(TIEMPO);
        system("clear");
        if(/*whaitToRight==1 && */token==TO_RIGHT){
            paramDecol *decol = malloc(sizeof(paramDecol)); //puntero para decolar
            decol->direccion = TO_RIGHT;
            vehiculo* nuevo =malloc(sizeof(vehiculo));
            if(MAX_AUTOS_PASAN>tiempoDesdeUltimoAuto){
                nuevo = decolar((void*)decol);
            }else
                nuevo = NULL;
            movimiento_carril(nuevo, token);

            if(whaitToLeft==1)
                tiempoDesdeUltimoAuto++;
            else
                tiempoDesdeUltimoAuto= 0;

        }else if(/*whaitToLeft==1 && */token==TO_LEFT){
            paramDecol *decol = malloc(sizeof(paramDecol)); //puntero para decolar
            decol->direccion = TO_LEFT;
            vehiculo* nuevo =NULL;
            if(MAX_AUTOS_PASAN>tiempoDesdeUltimoAuto){
                nuevo = decolar((void*)decol);
            }
            
            movimiento_carril(nuevo, token);

            if(whaitToRight==1)
                tiempoDesdeUltimoAuto++;
            else
                tiempoDesdeUltimoAuto= 0;
                
        }
        printf("\n\n");
        if(whaitToLeft==1 && token==TO_RIGHT){
            if(lleno == 0){
                token = TO_LEFT;
                tiempoDesdeUltimoAuto= 0;
            }
        }else if(whaitToRight==1 && token==TO_LEFT){
            if(lleno == 0){
                token = TO_RIGHT;
                tiempoDesdeUltimoAuto= 0;
            }
        }
        if(stop==1){
            break;
        }
        
        printf("%s\n",cmd);
    }
    
}

vehiculo crear_vehiculo(int direccion){
    vehiculo nuevo;
    nuevo.direccion = direccion;
    //zona critica para asignar nombre a un nuevo vehiculo
    pthread_mutex_lock(&crearAuto);
    char name[10] ; 
    if(cantidad>9)
        sprintf(name, "auto%d", cantidad);
    else{
        sprintf(name, "auto0%d", cantidad);
    }
    strcpy(nuevo.descripcion, name);
    cantidad++;
    pthread_mutex_unlock(&crearAuto);
    return nuevo;
}


//Agregar elementos a la cola
void *agregar_vehiculo(void *p){

    paramAdd *par=p;
    vehiculo vehi = par->v;
    nodo *nuevo=malloc(sizeof(nodo));
    if(vehi.direccion==TO_RIGHT){
        pthread_mutex_lock(&colDer); //colocar vehiculo con direccion a la derecha
    }else if(vehi.direccion == TO_LEFT){
        pthread_mutex_lock(&colIzq); //colocar vehiculo con direccion a la izquierda
    }
    if(par->col->primero==NULL){
        
        nuevo->v = vehi;
        nuevo->sig = NULL;
        par->col->primero = nuevo;
        par->col->ultimo = nuevo;
    }else{
        nuevo= malloc(sizeof(nodo));
        nuevo->v = vehi;
        nuevo->sig = NULL;
        par->col->ultimo->sig = nuevo;
        par->col->ultimo = nuevo;
    }
    if(vehi.direccion==TO_RIGHT){
        pthread_mutex_unlock(&colDer);
    }else if(vehi.direccion == TO_LEFT){
        pthread_mutex_unlock(&colIzq);
    }   
}

vehiculo* decolar(void *decol){
    paramDecol * dir = (paramDecol*)decol;
    vehiculo* nuevo = malloc(sizeof(vehiculo));
    cola *c;
    //printf("Inicio decl\n");
    if(dir->direccion==TO_RIGHT){
        pthread_mutex_lock(&colDer);
        c= &vehiculoToDerecha;
    }else if(dir->direccion==TO_LEFT){
        pthread_mutex_lock(&colIzq);
        c= &vehiculoToIzquierda;
    }
    if(c->primero!=NULL){
        nuevo = &(c->primero->v);
        if(c->primero->sig==NULL){
            c->ultimo= NULL;
            c->primero = c->primero->sig;
        }else{
            c->primero=c->primero->sig;
        }
    }else {
        nuevo = NULL;
    }
    if(dir->direccion==TO_RIGHT){
        pthread_mutex_unlock(&colDer);
    }else if(dir->direccion==TO_LEFT){
        pthread_mutex_unlock(&colIzq);
    }
    //printf("Fin decolar\n");
    return nuevo;
}

/***
*
* (c) 2004 ackbar
*/
char inkey(void) {
  char c;
  struct termio param_ant, params;

  ioctl(STDINFD,TCGETA,&param_ant);

  params = param_ant;
  params.c_lflag &= ~(ICANON|ECHO);
  params.c_cc[4] = 1;

  ioctl(STDINFD,TCSETA,&params);

  fflush(stdin); fflush(stderr); fflush(stdout);
  read(STDINFD,&c,1);

  ioctl(STDINFD,TCSETA,&param_ant);
  return c;
}
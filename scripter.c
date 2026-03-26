/* scripter: este programa simula una shell que lee un archivo de comandos 
y ejecuta cada línea como si fuera una instrucción de terminal. 
Admite ejecución en background, pipes y redirecciones. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>

/* CONST VARS */
const int max_line = 1024;
const int max_commands = 10;
#define max_redirections 3 //stdin, stdout, stderr
#define max_args 15

/* VARS TO BE USED FOR THE STUDENTS */
char *argvv[max_args];
char *filev[max_redirections];
int background = 0;

int contador = 1; //contador de número de procesos en segundo plano

struct ComandoBackground {
    pid_t pid; //PID del proceso
    char nombre[200]; //nombre del comando
    int finalizado; //0 si no ha finalizado y 1 si ha finalizado
};

struct ComandoBackground procesos[100];

//signal handler para SIGCHLD
void handler_sigchld(int signal) {
    int estado;
    pid_t pid;
    while ((pid = waitpid(-1, &estado, WNOHANG)) > 0) {
        int encontrado = 0;
        for (int i = 0; i < contador && (encontrado==0); i++) {
            if (procesos[i].pid == pid && procesos[i].finalizado == 0) {
                procesos[i].finalizado = 1; //marcamos como finalizado
                encontrado = 1; //sale del bucle
            }
        }
    }
}

/**
 * This function splits a char* line into different tokens based on a given character
 * @return Number of tokens 
 */
int tokenizar_linea(char *linea, char *delim, char *tokens[], int max_tokens) {
    int i = 0;
    char *token = strtok(linea, delim);
    while (token != NULL && i < max_tokens - 1) {
        tokens[i++] = token;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return i;
}

/**
 * This function processes the command line to evaluate if there are redirections. 
 * If any redirection is detected, the destination file is indicated in filev[i] array.
 * filev[0] for STDIN
 * filev[1] for STDOUT
 * filev[2] for STDERR
 */
 void procesar_redirecciones(char *args[]) {
    //initialization for every command
    filev[0] = NULL;
    filev[1] = NULL;
    filev[2] = NULL;
    //Store the pointer to the filename if needed.
    //args[i] set to NULL once redirection is processed
    for (int i = 0; args[i] != NULL; i++) {
        if (strcmp(args[i], "<") == 0) {
            filev[0] = args[i+1];
            args[i] = NULL;
            args[i+1] = NULL;
            i++;  //incrementamos i para que no pare el bucle si hay más redirecciones
        } else if (strcmp(args[i], ">") == 0) {
            filev[1] = args[i+1];
            args[i] = NULL;
            args[i+1] = NULL;
            i++;
        } else if (strcmp(args[i], "!>") == 0) {
            filev[2] = args[i+1];
            args[i] = NULL;
            args[i+1] = NULL;
            i++;
        }
    }
}

/*funcion para eliminar las comillas de los argumentos*/
void eliminar_comillas(char *args[]) {
    for (int i = 0; args[i] != NULL; i++) {
        int len = strlen(args[i]);
        if (args[i][0] == '"' && len > 1) {
            for (int k = 0; k < len; k++) {
                args[i][k] = args[i][k + 1];
            }
            len--;
        }
        if (len > 0 && args[i][len - 1] == '"') {
            args[i][len - 1] = '\0';
        }
    }
}

/**
 * This function processes the input command line and returns in global variables: 
 * argvv -- command an args as argv 
 * filev -- files for redirections. NULL value means no redirection. 
 * background -- 0 means foreground; 1 background.
 */
 int procesar_linea(char *linea) {
    char *comandos[max_commands];
    int num_comandos = tokenizar_linea(linea, "|", comandos, max_commands);

    //Check if background is indicated
    background = 0;
    if (strchr(comandos[num_comandos - 1], '&')) {
        background = 1;
        char *pos = strchr(comandos[num_comandos - 1], '&');
        //remove character 
        *pos = '\0';
    }

    //declaramos los pipes que vamos a usar y un array para guardar los pids de los hijos
    int pipes[max_commands - 1][2];
    pid_t pids[max_commands];

    //creamos los pipes que conectan los comandos
    for (int i = 0; i < num_comandos - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Error al crear el pipe");
            exit(-1);
        }
    }

    //bucle que ejecuta los comandos
    for (int i = 0; i < num_comandos; i++) {
        //restauramos a NULL argvv para que no haya restos de argumentos de otros comandos
        for (int j = 0; j < max_args; j++) {
            argvv[j] = NULL;
        }
        
        // tokenizar los argumentos del comando
        tokenizar_linea(comandos[i], " \t\n" , argvv, max_args);
        // eliminar las comillas de los argumentos
        eliminar_comillas(argvv);
        procesar_redirecciones(argvv);

        // proceso hijo
        pids[i] = fork();
        if (pids[i] == -1) {
            perror("Error creando el proceso hijo con fork");
            exit(-1);
        }

        if (pids[i] == 0) {
            //si no es el primer comando, redirige entrada estándar al pipe anterior
            if (i > 0) {
                dup2(pipes[i-1][0], STDIN_FILENO);
            }
            //si no es el último comando, redirige salida estándar al pipe siguiente
            if (i < num_comandos - 1) {
                dup2(pipes[i][1], STDOUT_FILENO);
            }

            //si es el primer comando y se ha indicado redirección de entrada estándar, se redirige 
            if (filev[0] && i == 0) {
                int fd_in = open(filev[0], O_RDONLY);
                if (fd_in == -1) {
                    perror("Error al abrir el archivo de entrada");
                    exit(-1);
                    }
                dup2(fd_in, STDIN_FILENO);
                close(fd_in);
            }
            //si es el último comando y se ha indicado redirección de salida estándar, se redirige 
            if (filev[1] && i == num_comandos - 1) {
                int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                if (fd_out == -1) {
                    perror("Error al abrir el archivo de salida");
                    exit(-1);
                    }
                dup2(fd_out, STDOUT_FILENO);
                close(fd_out);
            }
            //si es el último comando y se ha indicado redirección estándar de errores, se redirige 
            if (filev[2] && i == num_comandos - 1) {
            	int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
            	if (fd_err == -1) {
                    perror("Error al abrir el archivo de error");
                    exit(-1);
                    }
                dup2(fd_err, STDERR_FILENO);
                close(fd_err);
            }


            //cerramos  los pipes abiertos en el hijo
            for (int j = 0; j < num_comandos - 1; j++) {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            //ejecutamos el comando
            execvp(argvv[0], argvv);
            //si llega hasta aquí, ha habido un error en el execvp
            perror("Error ejecutando execvp");
            exit(-1);
        }
    }

    //cerramos los pipes del padre
    for (int i = 0; i < num_comandos - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    //si no se ha especificado background, esperamos a los hijos
    if (background==0) {
        for (int i = 0; i < num_comandos; i++) {
        	waitpid(pids[i], NULL, 0);
        }
    } else {
        if (num_comandos >= 1) {
            //se imprime el pid del proceso
            printf("%d", pids[num_comandos - 1]);
            //guardamos el pid 
            procesos[contador - 1].pid = pids[num_comandos - 1];
            //guardamos el nombre del comando
            strcpy(procesos[contador - 1].nombre, argvv[0]);
            //establecemos como no finalizado
            procesos[contador - 1].finalizado = 0;
            //incrementamos el contador de procesos en background
            contador++;
        }
    }

    return num_comandos;
}


int main(int argc, char *argv[]) {
    //comprobamos que el número de argumentos es dos
    if (argc != 2) {
        printf("Usage: %s <script_fichero_con_comandos>\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        perror("Error abriendo el archivo script"); 
        return -1;
    }

    signal(SIGCHLD, handler_sigchld); //cuando un hijo termine se ejecuta handler_sigchld

    //declaramos las variables necesarias para leer el fichero
    char buffer[max_line];
    int indice = 0; 
    char c;
    int primera_linea = 1;
    ssize_t n;

    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            buffer[indice] = '\0';
            //si es la primera línea comprobamos que tenga el formato que debe tener
            if (primera_linea == 1) {
                if (strcmp(buffer, "## Script de SSOO") != 0) {
                    perror("Error la primera línea es distinta de ## Script de SSOO"); 
                    close(fd); 
                    return -1;
                }
                primera_linea = 0; //ya hemos leído la primera línea
            } else {
                //si el índice es 0 es que hemos encontrado una línea vacía
                if (indice == 0) {
                    perror("Error se ha encontrado una línea vacía"); 
                    close(fd); 
                    return -1;
                }
                //procesamos el/los comando/s de la línea
                procesar_linea(buffer);
            }
            //reiniciamos para la siguiente línea
            indice = 0;
        } else {
            //almacenamos los caracteres que vamos leyendo en el buffer
            if (indice < max_line - 1) {
                buffer[indice] = c;
                indice++;
            }
            else { 
                perror("Error se ha encontrado una Línea demasiado larga"); 
                close(fd); 
                return -1; 
            }
        }
    }

    //procesamos la última línea si no termina en \n
    if (indice > 0) {
        buffer[indice] = '\0';
        if (primera_linea == 1) {
            if (strcmp(buffer, "## Script de SSOO") != 0) {
                perror("Error la primera línea es distinta de ## Script de SSOO");
                close(fd);
                return -1;
            }
        } else {
            procesar_linea(buffer);
        }
    }

    close(fd);
    return 0;
}

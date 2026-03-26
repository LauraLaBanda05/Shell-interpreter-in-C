#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

const int max_line = 1024;

int main(int argc, char ** argv) {
    //comprobamos que el número de argumentos no sea menor de tres
    if (argc < 3) {
        printf("Usage: %s <ruta_fichero> <cadena_busqueda>\n", argv[0]);
        return -1;
    }
    
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("Error abriendo el fichero");
        return -1;
    }

    //declaramos las variables necesarias para leer el fichero
    char buffer[max_line];
    int indice = 0;
    char c;
    int encontrada = 0;

    while (read(fd, &c, 1) == 1) {
        if (c == '\n') {
            buffer[indice] = '\0';
            //budcamos al cadena indicada en el buffer
            if (strstr(buffer, argv[2])) { 
                //si la encuentra, imprime la línea
                printf("%s\n", buffer);
                encontrada = 1;
            }

            indice = 0;  // reiniciar indice para la siguiente línea
        } else {
            //almacenamos los caracteres que vamos leyendo en el buffer
            if (indice < max_line - 1) {
                buffer[indice] = c;
                indice++;
            } else {
                perror("Error se ha encontrado una Línea demasiado larga"); 
                close(fd);
                return -1;
            }
        }
    }

    //si no ha encontrado la cadena, muestra el mensaje
    if (encontrada == 0) {
        printf("\"%s\" not found.\n", argv[2]);
    }
    
    close(fd);
    return 0;
}

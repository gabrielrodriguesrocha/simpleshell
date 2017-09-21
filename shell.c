#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX 256
#define MAX_SUB 10

int main() {
  char comando[MAX];
  char *argv[MAX][MAX];
  char filename[MAX];
  char *subcomandos[MAX_SUB];
  char *walk;
  int pid, qtd_sub;
  short int paralelo;

  while (1) {
    printf("> ");
    fgets(comando, MAX, stdin);

    subcomandos[0] = strtok(comando, "&\n");
    int i, j;
    for(i = 1, paralelo = 0, qtd_sub = 1; i < MAX_SUB && (subcomandos[i] = strtok(NULL, "&\n")); i++) {
	  printf("%s\n", subcomandos[i]);
      while(subcomandos[i][0] == ' ')
        subcomandos[i] = subcomandos[i] + 1;
      paralelo = 1;
      qtd_sub++;
    }

    //printf("%s\n", subcomandos[0]);
    //printf("%d\n", subcomandos[1][0]);

    for (i = 0; i < qtd_sub; i++) {
      argv[i][0] = strtok(subcomandos[i], " \n");
      //printf("%s\n", argv[i][0]);
      for (j = 1; j < MAX && (argv[i][j] = strtok(NULL, " \n")); j++);
    }

    //printf("%s\n", argv[0][0]);
    //printf("%d\n", argv[1] == NULL);
    //printf("%d\n", argv[2] == NULL);
    
    if (!strcmp(comando, "exit")) {
      exit(EXIT_SUCCESS);
    }
    for (i = 0; i < qtd_sub; i++) {
      pid = fork();
      if (pid) {
        if (!paralelo)
          waitpid(pid, NULL, 0); 
      } else {
        execvp(subcomandos[i], argv[i]);
        printf("Erro ao executar comando!\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

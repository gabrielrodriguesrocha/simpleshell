#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h> // Constantes O_*

#define MAX 256
#define MAX_SUB 20
#define MAX_ARG 50

typedef struct fp {
	char *in, *out;
} fp;

int main() {
  char comando[MAX];
  char *argv[MAX_SUB][MAX_ARG];
  char *filename[MAX_SUB];
	fp arquivos[MAX_SUB];
  char *subcomandos[MAX_SUB];
	int fd[3];
  char *walk;
  int pid, qtd_sub;
  short int paralelo;

	int i,j;

  while (1) {
    printf("> ");
    fgets(comando, MAX, stdin);

		/* Parsing de subcomandos (paralelos ou não)
		 * Um comando é composto por um subcomando trivialmente */  
    subcomandos[0] = strtok(comando, "&\n");
    for(i = 1, paralelo = 0, qtd_sub = 1;
			  i < MAX_SUB && (subcomandos[i] = strtok(NULL, "&\n"));
			  i++, qtd_sub++) {
      while(subcomandos[i][0] == ' ')
        subcomandos[i]++;
      paralelo = 1;
    }

		/* Parsing de subcomandos com direcionamento
		 * Observe que, na verdade, só é necessário obter qual o nome do arquivo
		 * para o qual a entrada ou saída será direcionada
		 * O código abaixo pode ser estendido para o caso de direcionar FDs arbitrários
		 * (com certo trabalho) */
		for (i = 0, j = 0; i < qtd_sub; i++, j++) {
			if (subcomandos[i]) {
				strtok(subcomandos[i], "<");
				arquivos[j].in = strtok(NULL, "<");
				strtok(subcomandos[i], ">");
				arquivos[j].out = strtok(NULL, ">");
				while (arquivos[j].in && arquivos[j].in[0] == ' ')
					arquivos[j].in++;
				while (arquivos[j].out && arquivos[j].out[0] == ' ')
					arquivos[j].out++;
			}
		}

		/* Parsing de argumentos de subcomandos
 		 * Supõe-se que argumentos não contém whitespace */
    for (i = 0; i < qtd_sub; i++) {
      argv[i][0] = strtok(subcomandos[i], " \n");
      for (j = 1; j < MAX && (argv[i][j] = strtok(NULL, " \n<>&")); j++);
    }

		/*----------------------------
		 *  Execução dos subcomandos
		 *----------------------------*/

    if (!strcmp(comando, "exit")) {
      exit(EXIT_SUCCESS);
    }
    
		/*  O código abaixo simula execução paralela.
 		 *  Teríamos que usar threads para executar vários subcomandos paralelamente de fato
 		 *  ...não vamos usar threads. */
		for (i = 0; i < qtd_sub; i++) {
      pid = fork();
      if (pid) {
        if (!paralelo)
          waitpid(pid, NULL, 0); 
      } else { // Felizmente os FDs internos não se alteram com duplicação, mesmo com troca de imagem
				if (arquivos[i].in) {
					fd[0] = open(arquivos[i].in, O_RDONLY);
					dup2(fd[0], 0);
					arquivos[i].in = NULL;
					close(fd[0]);
				}
				else if (arquivos[i].out) {
					fd[1] = open(arquivos[i].out, O_WRONLY | O_CREAT);
					dup2(fd[1], 1);
					arquivos[i].out = NULL;
					close(fd[1]);
				}
        execvp(subcomandos[i], argv[i]);
        printf("Erro ao executar comando!\n");
        exit(EXIT_FAILURE);
      }
    }

  }

}

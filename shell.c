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
	short int bg;

	int i,j;

	while (1) {
		printf("> ");
		fgets(comando, MAX, stdin);

		/* Parsing de subcomandos (bgs ou não)
		 * Um comando é composto por um subcomando trivialmente */  
		subcomandos[0] = strtok(comando, "&\n");
		for(i = 1, qtd_sub = 1, bg = 0;
			i < MAX_SUB && (subcomandos[i] = strtok(NULL, "&"));
			i++) {

			if (subcomandos[i][0] != '\n')
					qtd_sub++;
		
			while(subcomandos[i][0] == ' ')
					subcomandos[i]++;
			bg = 1;
		}

		/* Parsing de subcomandos com direcionamento
	 	* Observe que, na verdade, só é necessário obter qual o nome do arquivo
	 	* para o qual a entrada ou saída será direcionada
	 	* O código abaixo pode ser estendido para o caso de direcionar FDs arbitrários
	 	* (com certo trabalho) */
		for (i = 0, j = 0; i < qtd_sub; i++, j++) {
			if (subcomandos[i]) {
				arquivos[j].in = strchr(subcomandos[i], '<');
				arquivos[j].out = strchr(subcomandos[i], '>');
				if(arquivos[j].in) {
					arquivos[j].in = strtok(arquivos[j].in, " \n");
					arquivos[j].in = strtok(NULL, " \n");
					while(arquivos[j].in[0] == ' ')
							arquivos[j].in++;
				}
				if(arquivos[j].out) {
					arquivos[j].out = strtok(arquivos[j].out, " \n");
					arquivos[j].out = strtok(NULL, " \n");
					while(arquivos[j].out[0] == ' ')
							arquivos[j].out++;
				}
			}
		}

		/* Parsing de argumentos de subcomandos
 		 * Supõe-se que argumentos não contém whitespace */
		for (i = 0; i < qtd_sub; i++) {
			argv[i][0] = strtok(subcomandos[i], " \n");
			for (j = 1; j < MAX && (argv[i][j] = strtok(NULL, " \n<>&")); j++);
		}

		/*----------------------------*
		 *  Execução dos subcomandos  *
		 *----------------------------*/
	
		if (!strcmp(comando, "exit")) {
			exit(EXIT_SUCCESS);
		}
    
		/*  O código abaixo simula execução não determinística *
		 *	de subcomandos (outros processos) no background.   */
		for (i = 0; i < qtd_sub; i++) {
		pid = fork();
		if (pid) {
			if (!bg)
				waitpid(pid, NULL, 0); 
			} else { 
				if (arquivos[i].in) { // Redirecionamento de entrada
						fd[0] = open(arquivos[i].in, O_RDONLY);
						dup2(fd[0], 0);
						arquivos[i].in = NULL;
						close(fd[0]);
				}
				if (arquivos[i].out) { // Redirecionamento de saída
						fd[1] = open(arquivos[i].out, O_WRONLY | O_CREAT, 0666);
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

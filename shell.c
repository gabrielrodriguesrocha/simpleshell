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
	char *argv_internal[MAX_SUB][MAX_ARG];
	char *pipecomandos[MAX_SUB][MAX_SUB];
	fp arquivos[MAX_SUB];
	char *subcomandos[MAX_SUB];
	int fd[3];
	char *walk;
	int pid, qtd_sub, qtd_pipe;
	short int paralelo_flag, pipe_flag;

	/*  Abaixo está descrita a gramática (ideal) deste shell.
 	 *  O símbolo tradicional para denotar união de regras, "|",
 	 *  foi substituído por "OU" para evitar ambiguidades.
 	 *  O símbolo '' denota a cadeia vazia e '\n' é um comando trivial. */

	/*********************************************************************
 	 *  comando -> subcomando '&' comando OU '\n'                        *
 	 *  subcomando -> subexpressão '|' subcomando OU subexpressão        *
 	 *  subexpressão -> executável argumento                             *
 	 *  executável -> x in $PATH OU x in $PWD                            *
 	 *  argumento -> (x in executável.argument_list) ' ' argumento OU '' *
 	 *********************************************************************/

	/*  Na pŕatica, subcomando é composto por no máximo duas subexpressões
	 *  separadas por um "|". */

	int i,j,k;

	while (1) {
		printf("> ");
		fgets(comando, MAX, stdin);

		/* Parsing de subcomandos (paralelos ou não)
		 * Um comando é composto por um subcomando trivialmente */
		subcomandos[0] = strtok(comando, "&\n");
		for(i = 1, qtd_sub = 1, paralelo_flag = 0;
			i < MAX_SUB && (subcomandos[i] = strtok(NULL, "&"));
			i++) {

			if (subcomandos[i][0] != '\n')
					qtd_sub++;

			while(subcomandos[i][0] == ' ')
					subcomandos[i]++;
			paralelo_flag = 1;
		}

		/*  Parsing de subcomandos com pipe
 		 *  O código abaixo é bem convolucionado,
 		 *  entretanto sua função é registrar quais subcomandos
 		 *  tem quais subexpressões. */

		for(i = 0, qtd_pipe = 1, pipe_flag = 0; i < qtd_sub; i++) {
			pipecomandos[i][0] = strtok(subcomandos[i], "|");
			for(j = 1; j < MAX_SUB && (pipecomandos[i][j] = strtok(NULL, "|")); j++, pipe_flag = 1, qtd_pipe++) {
				while (pipecomandos[i][j][0] == ' ')
					pipecomandos[i][j]++;
			}
		}


		/* Parsing de redirecionamento
 		 * Na verdade, o código abaixo tem dois casos:
 		 *
 		 * 1. se algum símbolo de pipe foi encontrado,
 		 * 		o parsing é feito através da quebra de
 		 * 		subcomandos em subexpressões com argumentos
 		 * 		e que possivelmente terão E/S redirecionada;
 		 * 		-> (caso em que pipe_flag == 1)
 		 *
 		 * 2. senão, o parsing é feito tratando subcomandos
 		 * 		como as próprias subexpressões, com argumentos
 		 * 		e redirecionamento.
 		 * 		-> (caso em que pipe_flag == 0)
 		 *
 		 * Essa é uma forma razoavelmente preguiçosa e
 		 * perigosa de resolver um problema de ambiguidade na gramática. */

		if (pipe_flag) {
			for (i = 0, j = 0; i < qtd_pipe; i++, j++) {
				if (pipecomandos[i][j]) {
					arquivos[j].in = strchr(pipecomandos[i][j], '<');
					arquivos[j].out = strchr(pipecomandos[i][j], '>');
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
		}
		else {
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
		}

		/* Parsing de argumentos de subcomandos
 		 * Supõe-se que argumentos não contém whitespace */
		int l;
		for (i = 0, l = 0; i < qtd_sub; i++)
			for (j = 0; (argv_internal[l][0] = strtok(pipecomandos[i][j], " \n")); j++, l++)
				for(k = 1; k < MAX_SUB && (argv_internal[l][k] = strtok(NULL, " <>&|\n")); k++);

	/*----------------------------*
	 *  Execução dos subcomandos  *
	 *----------------------------*/

		if (!strcmp(comando, "exit")) {
			exit(EXIT_SUCCESS);
		}

		/*  O código abaixo simula execução paralela.
 	 	 *  Teríamos que usar threads para executar vários subcomandos paralelamente de fato
	 	 *  ...não vamos usar threads. */
		for (i = 0; i < qtd_sub; i++) {
			pid = fork();
			if (pid) { // Processo pai

				if (!paralelo_flag)
					waitpid(pid, NULL, 0);

			}
			else { // Processo filho

				/*  Tratamento de redirecionamento de E/S */
				if (arquivos[i].in) {
					fd[0] = open(arquivos[i].in, O_RDONLY);
					dup2(fd[0], 0);
					arquivos[i].in = NULL;
					close(fd[0]);
				}
				if (arquivos[i].out) {
					fd[1] = open(arquivos[i].out, O_WRONLY | O_CREAT, 0666);
					dup2(fd[1], 1);
					arquivos[i].out = NULL;
					close(fd[1]);
				}

				/*  Tratamento de piping */
				if (pipe_flag) {
					pipe(fd);
					int pipe_pid;
					if ((pipe_pid = fork())) {
						close(fd[1]);
						dup2(fd[0], 0);
						waitpid(pipe_pid, NULL, 0);
						execvp(pipecomandos[i][1], argv_internal[i+1]);
						printf("Erro ao executar comando %s!\n", pipecomandos[i][1]);
						exit(EXIT_FAILURE);
					}
					else {
						close(fd[0]);
						dup2(fd[1], 1);
						execvp(pipecomandos[i][0], argv_internal[i]);
						printf("Erro ao executar comando %s!\n", pipecomandos[i][0]);
						exit(EXIT_FAILURE);
					}
				}
				/*  Tratamento de subexpressão sem piping */
				else {
					execvp(subcomandos[i], argv_internal[i]);
					printf("Erro ao executar comando!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}

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
	int fd[2], r_fd[2];
	int pid, qtd_sub, qtd_pipe;
	short int paralelo_flag, pipe_flag;

	/*  Abaixo está descrita a gramática (ideal) deste shell.
 	 *  O símbolo tradicional para denotar união de regras, "|",
 	 *  foi substituído por "OU" para evitar ambiguidades.
 	 *  O símbolo '' denota a cadeia vazia e '\n' é um comando trivial. */

	/*********************************************************************
 	 *  comando -> subcomando '&' comando OU '\n'                        *
 	 *  subcomando -> subexpressão '|' subcomando OU subexpressão        *
 	 *  subexpressão -> executável argumento OU                          *
	 *                  executável argumento redireção                   *
	 *  redireção -> '>' arquivo OU '<' arquivo OU                       *
	 *               '<' arquivo '>' arquivo                             *
 	 *  executável -> x in $PATH OU x in $PWD                            *
 	 *  argumento -> (x in executável.argument_list) ' ' argumento OU '' *
	 *  arquivo -> .+                                                    *
 	 *********************************************************************/

	int i,j,k;

	while (1) {
		printf("> ");
		fgets(comando, MAX, stdin);

		/* Parsing de subcomandos (paralelos ou não)
		 * Um comando é composto por um subcomando trivialmente 
		 * subcomandos[i] corresponde ao i-ésimo subcomando.
		 *
		 * Exemplo:
		 * 
		 * cat < a.txt | sort -r > b.txt & gnome-calculator &
		 * ------------V---------------    --------V-------
		 *  		 subcomandos[0]             subcomandos[1]   */

		subcomandos[0] = strtok(comando, "&\n");
		for(i = 1, qtd_sub = 1, paralelo_flag = 0;
			i < MAX_SUB && (subcomandos[i] = strtok(NULL, "&"));
			i++) {

			if (subcomandos[i][0] != '\n')
					qtd_sub++;

			while(subcomandos[i][0] == ' ') // Remoção de whitespace
					subcomandos[i]++;
			paralelo_flag = 1;
		}

		/*  Parsing de subcomandos com pipe
 		 *  O código abaixo é bem convolucionado,
 		 *  entretanto sua função é registrar quais subcomandos
 		 *  tem quais subexpressões. 
 		 *  pipecomandos[i][j] corresponde à j-ésima subexpressão
 		 *  do i-ésimo comando.
 		 *  
		 *  Exemplo:
 		 *     cat < a.txt     |   sort -r > b.txt   &  gnome-calculator & 
 		 *     -----V-----         -------V-------      -------V--------
 		 *  
		 *  pipecomandos[1][0]    pipecomandos[0][1]   pipecomandos[1][0]
 		 *
 		 *  Note que pipecomandos[1][0] é identificado mesmo 
 		 *  sem que essa subexpressão redirecione sua saída
 		 *  para outro processo.  */

		for(i = 0, qtd_pipe = 1, pipe_flag = 0; i < qtd_sub; i++) {
			pipecomandos[i][0] = strtok(subcomandos[i], "|");
			for(j = 1; j < MAX_SUB && (pipecomandos[i][j] = strtok(NULL, "|")); j++, pipe_flag = 1, qtd_pipe++) {
				while (pipecomandos[i][j][0] == ' ') // Remoção de whitespace
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
 		 * 	Exemplo:
 		 * 	   cat < a.txt | sort -r > b.txt
 		 * 	         --V--             --V--
 		 * 	      arquivos[0].in    arquivos[1].out
 		 *
 		 * 2. senão, o parsing é feito tratando subcomandos
 		 * 		como as próprias subexpressões, com argumentos
 		 * 		e redirecionamento.
 		 * 		-> (caso em que pipe_flag == 0)
 		 *
 		 * 	Exemplo:
 		 * 		sort <    shell.c    >    shell_sorted.c & ls -lh > a.txt &
 		 * 		          ---V---         -------V------            --V--
 		 * 		       arquivos[0].in     arquivos[0].out       arquivos[1].out
 		 *
 		 * Essa é uma forma razoavelmente preguiçosa e
 		 * perigosa de resolver um problema de ambiguidade na gramática. */

		if (pipe_flag) {

			for (i = 0, k = 0; i < qtd_pipe; i++) {
				for (j = 0; pipecomandos[i][j]; j++, k++) {
					arquivos[k].in = strchr(pipecomandos[i][j], '<');
					arquivos[k].out = strchr(pipecomandos[i][j], '>');
					if(arquivos[k].in) {
						arquivos[k].in = strtok(arquivos[j].in, " \n");
						arquivos[k].in = strtok(NULL, " \n");
						while(arquivos[k].in[0] == ' ')
								arquivos[k].in++;
					}
					if(arquivos[k].out) {
						arquivos[k].out = strtok(arquivos[k].out, " \n");
						arquivos[k].out = strtok(NULL, " \n");
						while(arquivos[k].out[0] == ' ')
								arquivos[k].out++;
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
 		 * Supõe-se que argumentos não contém whitespace
 		 * Exemplo:
 		 *
 		 *	ls -l -h | sort -r & df -h &
 		 *
 		 * ls   -> argv_internal[0][0]
 		 * -l   -> argv_internal[0][1]
 		 * -h   -> argv_internal[0][2]
 		 * | 
 		 * sort -> argv_internal[1][0]
 		 * -r   -> argv_internal[1][1]
 		 * &
 		 * df   -> argv_internal[2][0]
 		 * -h   -> argv_internal[2][1]
 		 * &
 		 *
 		 * */

		int l;
		for (i = 0, l = 0; i < qtd_sub; i++) // Laço para cada subcomando
			for (j = 0; (argv_internal[l][0] = strtok(pipecomandos[i][j], " \n")); j++, l++) // Laço para cada subexpressão (pipe)
				for(k = 1; k < MAX_SUB && (argv_internal[l][k] = strtok(NULL, " <>&|\n")); k++); // Laço para cada argumento

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
			if (pid) { // Processo pai

				if (!paralelo_flag) // Execução no background
					waitpid(pid, NULL, 0);

			}
			else { // Processo filho

				/*  Tanto a execução de subcomandos com piping e sem piping
				 *  usam a chamada de sistema execvp, para execução c/ argumentos. */

				/*  TRATAMENTO DE PIPING
				 *
				 *  A ideia é criar uma cadeia de subexpressões,
				 *  em que cada subexpressão gera um par de FDs
				 *  através de um pipe, de forma que o FD de entrada
				 *  da i-ésima subexp. é o FD de leitura do pipe
				 *  da (i-1)-ésimo subexp.
				 *
				 *  A chamada de sistema pipe toma um vetor fd
				 *  de inteiros com 2 elementos e cria um pipe implícito
				 *  onde fd[0] é o FD de leitura e fd[1] é o FD de escrita.
				 *  Logo as subexp. escrevem em fd[1] e leem do fd[0]
				 *  de subexps. anteriores, obtido através da variável input.
				 *
				 *  Exemplo:
				 *  		
				 *  		ls -l | sort | wc -l
				 *
				 *  1ª iteração: (A primeira subexp. desconsidera o input)
				 *  		subexp. = ls -l
				 *  		fd[0]   = 2
				 *  		fd[1]   = 3
				 *  		input   = ?
				 *
				 *  2ª iteração: (As subexps. intermediárias redirecionam entrada e saída)
				 *  		subexp. = sort
				 *  		fd[0]		= 4
				 *  		fd[1]   = 5
				 *  		input   = 2
				 *  		
				 *  3ª iteração: (A subexp. ignora o fd[1], e imprime para STDOUT ou um arquivo)
				 *  		subexp. = wc -l
				 *  		fd[0]   = 6
				 *  		fd[1]   = 7
				 *  		input   = 4
				 *
				 *  Fim de execução.*/

				if (pipe_flag && qtd_pipe) {
					int pipe_pid, input;
					for(j = 0; pipecomandos[i][j] && qtd_pipe; j++) {
						pipe(fd);
						if (arquivos[j].in) {
							r_fd[0] = open(arquivos[j].in, O_RDONLY);
							arquivos[j].in = NULL;
						}
						else
							r_fd[0] = 0;
		
						if (arquivos[j].out) {
							r_fd[1] = open(arquivos[j].out, O_WRONLY | O_CREAT, 0666);
							arquivos[j].out = NULL;
						}
						else
							r_fd[1] = 1;
	
						if ((pipe_pid = fork())) {
							waitpid(pipe_pid, NULL, 0);
							input = fd[0];
							close(fd[1]);
							qtd_pipe--;
						}
						else {
							if (j == 0) { // Primeira subexpressão
								if(r_fd[0] != 0) { // Redirecionamento de entrada
									dup2(r_fd[0], 0);
									close(r_fd[0]);
								}
								dup2(fd[1], 1);
							}
							else if (pipecomandos[i][j+1]) { // Subexpressões interiores
								dup2(input, 0);
								dup2(fd[1], 1);
								close(input);
							}
							else { // Subexpressão final
								if(r_fd[1] != 1) { // Redirecionamento de saída
									dup2(r_fd[1], 1);
									close(r_fd[1]);
								}
								dup2(input, 0);
								close(fd[1]);
								close(input);
							}
							execvp(pipecomandos[i][j], argv_internal[j]);
							printf("Erro ao executar comando %s!\n", pipecomandos[i][j]);
							exit(EXIT_FAILURE);
						}
					}
				}
				/*  EXECUÇÃO USUAL */
				else {
					if (arquivos[i].in) {
						r_fd[0] = open(arquivos[i].in, O_RDONLY);
						dup2(r_fd[0], 0);
						arquivos[i].in = NULL;
						close(r_fd[0]);
					}
	
					if (arquivos[i].out) {
						r_fd[1] = open(arquivos[i].out, O_WRONLY | O_CREAT, 0666);
						dup2(r_fd[1], 1);
						arquivos[i].out = NULL;
						close(r_fd[1]);
					}

					execvp(subcomandos[i], argv_internal[i]);
					printf("Erro ao executar comando!\n");
					exit(EXIT_FAILURE);
				}
			}
		}
	}
}

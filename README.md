# simpleshell

*A shell so simple, it makes you feel good about your own shell*

Este shell foi desenvolvida para o 2º experimento da disciplina Laboratório de Sistemas Operacionais,
ministrada na Universidade Federal de São Carlos (Campus Sorocaba) pelo Prof. Dr. Gustavo M. D. Vieira.

Descrição da gramática do shell:
* comando → subcomando '&' comando | '\n'
* subcomando → subexpressão '|' subcomando | subexpressão
* subexpressão → executável argumento | executável argumento redireção
* redireção → '>' arquivo | '<' arquivo | '<' arquivo '>' arquivo | ''
* executável → x in $PATH | x in $PWD
* argumento → (x in executável.argument\_list) ' ' argumento | ''
* arquivo → .+

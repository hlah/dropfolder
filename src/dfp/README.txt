codigo com exemplo de uso da implementacao de troca de mensagens "confiavel".

$ make

$ ./main
	printa msgs recebidas e 'gerencia' abertura de novas portas.
	comando 'new_user Y' indica desejo do cliente de abrir conexao com servidor na porta Y do cliente.
	servidor responde com 'connect to X', e começa a escutar por X e enviar por Y.

$ ./client localhost
    conecta com main e envia mensagens lidas pela entrada.
	caso receba comando 'connecto to X', abre um socket que envia para porta X.

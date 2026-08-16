//AINDA sem modo noturno

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

typedef struct timespec crono;

typedef struct {
  bool terminou;
  int pontos;
  int tiros;
  char arma;
  int fase;
  char campo[13];
  char onda[20];
  int inimigos_inativos;
  int escudos;
  crono relogio;
  double intervalo;
} estado_t;

// Configura o terminal para receber teclas individualmente,
// sem precisar pressionar Enter, e desativa o eco das teclas digitadas.
void configura_terminal()
{
  if (system("stty raw opost -echo min 0 time 1") != 0) {
    perror("erro na execução de system(\"stty\")");
    fprintf(stderr, "você tem o programa stty instalado?\n");
    exit(1);
  }

  if (setvbuf(stdin, NULL, _IONBF, 0) != 0) {
    perror("erro na execução de setvbuf()");
    exit(1);
  }
}

// Lê um caractere da entrada padrão sem esperar o usuário pressionar Enter.
char lechar()
{
  fflush(stdout);

  char c;

  if (fread(&c, 1, 1, stdin) == 1) {
    return c;
  }

  return 0;
}

// Monta e executa o comando para reproduzir o arquivo de som correspondente
// ao caractere recebido.
void toca_som(char som)
{
  char comando[100];

  if (som == 'N' || som == 'n') {
    sprintf(comando, "aplay -q Sons/11.3.wav &");
  } else if (som == ')') {
    sprintf(comando, "aplay -q Sons/12.3.wav &");
  } else if (som == ' ') {
    sprintf(comando, "aplay -q Sons/x.3.wav &");
  } else {
    sprintf(comando, "aplay -q Sons/%c.3.wav &", som);
  }

  system(comando);
}

// Troca a arma atualmente selecionada para a próxima arma disponível.
void troca_arma(estado_t *est)
{
  if (est->arma == '9') {
    est->arma = 'n';
  } else if (est->arma == 'n') {
    est->arma = '0';
  } else {
    est->arma++;
  }

  toca_som(est->arma);
}

// Percorre o campo de jogo procurando o primeiro inimigo que corresponde
// à arma atualmente selecionada.
int procura_inimigo(estado_t *est)
{
  for (int i = 3; i < 13; i++) {
    if (est->campo[i] == est->arma) {
      return i;
    }
  }

  return -1;
}

// Percorre o campo de jogo procurando uma nave 'N'.
int procura_nave(estado_t *est)
{
  for (int i = 3; i < 13; i++) {
    if (est->campo[i] == 'N') {
      return i;
    }
  }

  return -1;
}

// Remove um inimigo do campo de jogo, substituindo seu caractere por um espaço.
void destroi_inimigo(estado_t *est, int pos)
{
  est->campo[pos] = ' ';
}

// Calcula a pontuação obtida ao destruir um inimigo.
int pontos_inimigo(int pos, char tipo)
{
  int pontos = 13 - pos;

  if (tipo == 'n') {
    pontos *= 2;
  }

  return pontos;
}

// Processa um disparo caso ainda existam tiros disponíveis.
void atira(estado_t *est)
{
  int pos;

  if (est->tiros <= 0) {
    return;
  }

  est->tiros--;

  if (est->arma == 'n') {
    pos = procura_inimigo(est);

    if (pos != -1) {
      est->pontos += pontos_inimigo(pos, est->campo[pos]);
      destroi_inimigo(est, pos);
      toca_som(est->arma);
      return;
    }

    pos = procura_nave(est);

    if (pos != -1) {
      est->campo[pos] = 'n';
      toca_som(est->arma);
      return;
    }

    toca_som('x');
    return;
  }

  pos = procura_inimigo(est);

  if (pos == -1) {
    toca_som('x');
    return;
  }

  est->pontos += pontos_inimigo(pos, est->campo[pos]);
  destroi_inimigo(est, pos);
  toca_som(est->arma);
}

// Lê uma tecla e executa a ação correspondente.
void processa_teclado(estado_t *est)
{
  char c = lechar();

  if (c == 27) {
    est->terminou = true;
  } else if (c == 9) {
    troca_arma(est);
  } else if (c == 13) {
    atira(est);
  }
}

// Inicia ou reinicia um cronômetro armazenando o instante atual.
void crono_inicia(crono *c) {
  clock_gettime(CLOCK_MONOTONIC, c);
}

// Calcula quantos segundos se passaram desde o instante armazenado
// no cronômetro até o momento atual.
double crono_parcial(crono *c) {
  crono agora;
  clock_gettime(CLOCK_MONOTONIC, &agora);

  double segundos = agora.tv_sec - c->tv_sec;
  double nanosegundos = agora.tv_nsec - c->tv_nsec;

  return segundos + 1e-9 * nanosegundos;
}

// Inicializa os valores gerais da partida, como pontuação, fase,
// arma inicial e intervalo inicial entre os movimentos dos inimigos.
void inicializa_estado(estado_t *est) {
  est->terminou = false;
  est->pontos = 0;
  est->fase = 1;
  est->arma = '0';
  est->intervalo = 2.0;
}

// Sorteia aleatoriamente os tipos de ataques.
char sorteia_tipo() {
  int n = rand() % 11;

  if (n == 10) {
    return 'N';
  }

  return '0' + n;
}

// Preenche o vetor onda com os 20 ataques que serão utilizados
// durante a onda atual.
void cria_onda(estado_t *est) {
  for (int i = 0; i < 20; i++) {
    est->onda[i] = sorteia_tipo();
  }
}

// Ativa o próximo ataque armazenado na onda quando a última posição
// do campo está livre.
void ativa_inimigo(estado_t *est)
{
  if (est->inimigos_inativos > 0
  && est->campo[12] == ' ') {
    int indice = 20 - est->inimigos_inativos;
    est->campo[12] = est->onda[indice];

    toca_som(est->campo[12]);

    est->inimigos_inativos--;
  }
}

// Verifica se um caractere do campo representa um inimigo ou ataque.
bool eh_inimigo(char c) {
  return c != ' ' && c != ')';
}

// Move todos os inimigos uma posição em direção à nave.
void move_inimigos(estado_t *est)
{
  char novo_campo[13];

  for (int i = 0; i < 13; i++) {
    novo_campo[i] = ' ';
  }

  for (int i = 0; i < 3; i++) {
    novo_campo[i] = est->campo[i];
  }

  for (int i = 0; i < 13; i++) {
    if (eh_inimigo(est->campo[i])) {
      if (i == 0) {
        est->terminou = true;
      } else if (est->campo[i - 1] == ')') {
        novo_campo[i - 1] = ' ';
        est->escudos--;
        toca_som(')');
      } else {
        novo_campo[i - 1] = est->campo[i];
      }
    }
  }

  for (int i = 0; i < 13; i++) {
    est->campo[i] = novo_campo[i];
  }
}

// Verifica se já passou o intervalo necessário para realizar um movimento.
void processa_tempo(estado_t *est)
{
  if (crono_parcial(&est->relogio) >= est->intervalo) {
    move_inimigos(est);
    ativa_inimigo(est);
    crono_inicia(&est->relogio);
  }
}

// Inicializa os dados específicos de uma nova onda.
void inicializa_onda(estado_t *est) {
  est->tiros = 30;
  est->escudos = 3;
  est->inimigos_inativos = 20;

  for (int i = 0; i < 13; i++) {
    if (i < 3) {
      est->campo[i] = ')';
    } else {
      est->campo[i] = ' ';
    }
  }

  crono_inicia(&est->relogio);
}

// Verifica se existe algum inimigo ativo no campo de jogo.
bool existem_inimigos(estado_t *est)
{
  for (int i = 3; i < 13; i++) {
    if (eh_inimigo(est->campo[i])) {
      return true;
    }
  }

  return false;
}

// Verifica se a onda terminou.
bool terminou_onda(estado_t *est)
{
  return est->inimigos_inativos == 0
  && !existem_inimigos(est);
}

// Exibe na tela a pontuação, a quantidade de tiros, a arma selecionada
// e o estado atual das 13 posições do campo de jogo.
void apresenta(estado_t *est)
{
  printf("%d %d %c ", est->pontos, est->tiros, est->arma);

  for (int i = 0; i < 13; i++) {
    printf("%c", est->campo[i]);
  }

  printf("\r");
}

// Adiciona à pontuação os bônus recebidos ao terminar uma onda.
void pontua_final_onda(estado_t *est)
{
  est->pontos += est->tiros * 2;
  est->pontos += est->escudos * 10;
}

// Exibe o resumo da onda concluída.
void apresenta_resumo(estado_t *est)
{
  printf("\n");
  printf("Fim da fase %d\n", est->fase);
  printf("Pontuacao: %d\n", est->pontos);
  printf("Tiros restantes: %d\n", est->tiros);
  printf("Escudos restantes: %d\n", est->escudos);
  printf("Pressione r para continuar.\n");
}

// Reproduz uma sequência de sons para indicar que a onda terminou.
void toca_fim_onda()
{
  system("aplay -q Sons/x.3.wav Sons/12.3.wav Sons/11.3.wav &");
}

// Reproduz uma sequência de sons para indicar que a partida terminou.
void toca_fim_partida()
{
  system("aplay -q Sons/12.3.wav Sons/x.3.wav Sons/11.3.wav Sons/12.3.wav &");
}

// Executa uma onda enquanto ela não terminar e enquanto o jogador não morrer.
void joga_onda(estado_t *est)
{
  while (!est->terminou && !terminou_onda(est)) {
    processa_teclado(est);
    processa_tempo(est);
    apresenta(est);
  }

  if (!est->terminou) {
    pontua_final_onda(est);
    toca_fim_onda();
    apresenta_resumo(est);
  }
}

// Aguarda o jogador pressionar a tecla 'r' para continuar para a próxima onda.
void espera_reinicio()
{
  char c;

  do {
    c = lechar();
  } while (c != 'r');
}

// Atualiza os parâmetros da próxima fase.
void atualiza_fase(estado_t *est)
{
  est->fase++;
  est->intervalo *= 0.9;
}

// Prepara uma nova onda inicializando seus dados e sorteando
// os 20 ataques que serão utilizados nela.
void prepara_onda(estado_t *est)
{
  inicializa_onda(est);
  cria_onda(est);
}

// Lê do arquivo pontuacoes.txt as três maiores pontuações já registradas.
void carrega_pontuacoes(int pontuacoes[3])
{
  FILE *arquivo = fopen("pontuacoes.txt", "r");

  if (arquivo == NULL) {
    pontuacoes[0] = 0;
    pontuacoes[1] = 0;
    pontuacoes[2] = 0;
    return;
  }

  for (int i = 0; i < 3; i++) {
    if (fscanf(arquivo, "%d", &pontuacoes[i]) != 1) {
      pontuacoes[i] = 0;
    }
  }

  fclose(arquivo);
}

// Abre o arquivo pontuacoes.txt e grava nele as três maiores pontuações.
void salva_pontuacoes(int pontuacoes[3])
{
  FILE *arquivo = fopen("pontuacoes.txt", "w");

  if (arquivo == NULL) {
    return;
  }

  for (int i = 0; i < 3; i++) {
    fprintf(arquivo, "%d\n", pontuacoes[i]);
  }

  fclose(arquivo);
}

// Compara a pontuação de uma partida com as três maiores pontuações
// armazenadas.
void atualiza_pontuacoes(int pontos)
{
  int pontuacoes[3];

  carrega_pontuacoes(pontuacoes);

  if (pontos > pontuacoes[0]) {
    pontuacoes[2] = pontuacoes[1];
    pontuacoes[1] = pontuacoes[0];
    pontuacoes[0] = pontos;
  } else if (pontos > pontuacoes[1]) {
    pontuacoes[2] = pontuacoes[1];
    pontuacoes[1] = pontos;
  } else if (pontos > pontuacoes[2]) {
    pontuacoes[2] = pontos;
  }

  salva_pontuacoes(pontuacoes);
}

// Controla toda a execução da partida.
void joga_partida(estado_t *est)
{
  prepara_onda(est);

  while (!est->terminou) {
    joga_onda(est);

    if (!est->terminou) {
      espera_reinicio();
      atualiza_fase(est);
      prepara_onda(est);
    }
  }

  toca_fim_partida();
  atualiza_pontuacoes(est->pontos);
}

// Restaura as configurações normais do terminal depois que o jogo termina.
void normaliza_terminal()
{
  system("stty sane");
}

// Função principal do programa.
int main() {
  estado_t estado;

  srand(time(NULL));

  inicializa_estado(&estado);

  configura_terminal();

  joga_partida(&estado);

  normaliza_terminal();

  return 0;
}

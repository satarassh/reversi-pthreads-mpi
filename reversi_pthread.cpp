#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <pthread.h>


//Definiramo velikost igralnega polja
#define BOARD_HEIGHT 8
#define BOARD_WIDTH 8
#define BOARD_SIZE (BOARD_HEIGHT*BOARD_WIDTH)

//Mozne barve polja
#define BLACK (-1)
#define WHITE 1
#define EMPTY 0

//Maksimalna globina iskanja Minimax algoritma
#define MAX_DEPTH 4

//Podatkovne strukture potrebne pri postavljanju novega ploscka
#define DIRECTIONS 8
const int direction[DIRECTIONS][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 }, { -1, 0 }, { 1, 1 }, { 1, -1 }, { -1, 1 }, { -1, -1 } };


#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define NTHREADS 8
#define NGAMES 10

struct posible_move {
  int i;
  int j;
};

struct params {
  int id;
  char board[BOARD_SIZE];
  int player_color;
  int max_depth;
  int depth;
  int count;
  int displacement;
};

struct posible_move moves[BOARD_SIZE];
int ret_score[BOARD_SIZE];

//Inicializiramo zacetno igralno polje
void initBoard(char *board){
  for (int i = 0; i < BOARD_HEIGHT; i++)
  for (int j = 0; j < BOARD_WIDTH; j++)
    board[i*BOARD_WIDTH + j] = EMPTY;

  board[27] = WHITE;
  board[28] = BLACK;
  board[35] = BLACK;
  board[36] = WHITE;

}

//Izrisi igralno polje
void printBoard(char *board){
  printf("  ");
  for (int j = 0; j < BOARD_WIDTH; j++)
    printf("-%d", j + 1);
  printf("\n");
  for (int i = 0; i < BOARD_HEIGHT; i++){
    printf("%d| ", i + 1);
    for (int j = 0; j < BOARD_WIDTH; j++)
    {
      char c = ' ';
      if (board[i*BOARD_WIDTH + j] == WHITE) c = 'O';
      if (board[i*BOARD_WIDTH + j] == BLACK) c = 'X';
      printf("%c ", c);
    }
    printf("\b|\n");
  }
  printf("  ");
  for (int j = 0; j < BOARD_WIDTH; j++)
    printf("--");
  printf("\n");
}

//Preveri, ce je poteza veljavna
int isValidMove(int player, char *board, int row, int column){

  //Ce polje ni prazno potem poteza ni veljavna
  if (board[row*BOARD_WIDTH + column] != EMPTY)
    return 0;

  //V vse smeri iz trenutnega polja pogledamo, ce preko nasprotnikovih plosckov pridemo do svojega ploscka
  int valid = 0;
  for (int i = 0; i < DIRECTIONS; i++){
    int metOpponent = 0;
    for (int di = row + direction[i][0], dj = column + direction[i][1]; di >= 0 && di < BOARD_HEIGHT && dj >= 0 && dj < BOARD_WIDTH; di += direction[i][0], dj += direction[i][1]){
      if (board[di*BOARD_WIDTH + dj] == player){
        if (!metOpponent)
          break;
        else{
          //Ce najdemo tako pot je poteza veljavna
          valid = 1;
          return valid;
        }
      }
      else if (board[di*BOARD_WIDTH + dj] == -player)
        metOpponent = 1;
      else
        break;
    }

  }
  return valid;

}

//Preverimo ce lahko trenutni igralec postavi svoj ploscek
int canPlayerMakeMove(int player, char *board){
  for (int i = 0; i < BOARD_HEIGHT; i++)
  for (int j = 0; j < BOARD_WIDTH; j++)
  if (isValidMove(player, board, i, j))
    return 1;

  return 0;


}

//Preverimo ce je ze konec igre
int isGameOver(char *board){
  //Ce noben od igralcev ne more narediti poteze je igre konec
  return (!canPlayerMakeMove(WHITE, board) && !canPlayerMakeMove(BLACK, board));

}
//Postavimo ploscek na polje in pobarvamo ustrezne ploscke v svojo barvo
void placePiece(int row, int column, int player_color, char * board){

  for (int i = 0; i < DIRECTIONS; i++){
    int metOpponent = 0;
    for (int di = row + direction[i][0], dj = column + direction[i][1]; di >= 0 && di < BOARD_HEIGHT && dj >= 0 && dj < BOARD_WIDTH; di += direction[i][0], dj += direction[i][1]){

      if (board[di*BOARD_WIDTH + dj] == player_color){
        if (!metOpponent)
          break;
        else{
          int count = MAX(abs(di - row), abs(dj - column));
          for (int c = 0; c < count; c++)
            board[(row + c*direction[i][0])*BOARD_WIDTH + column + c*direction[i][1]] = player_color;
          break;
        }
      }
      else if (board[di*BOARD_WIDTH + dj] == -player_color)
        metOpponent = 1;
      else
        break;
    }

  }
}

//Cloveski igralec preberemo vrstico in stolpec, kamor naj se postavi ploscek
int humanMove(char *board, int player_color) {
  int row = -1;
  int column = -1;
  if (isGameOver(board))
    return 0;
  do {
    printf("Input row and column ([1..8]<Enter>[1..8]):\n");
    scanf("%d", &row);
    scanf("%d", &column);
    printf("\n");
    row -= 1;
    column -= 1;
  } while (row >= 9 || row < 0 || column >= 9 || column >= 9 || !isValidMove(player_color, board, row, column));
  placePiece(row, column, player_color, board);
  return 1;
}

int evaluateBoard(char *board){
  int sum = 0;
  //Izracunamo razliko med stevilom belih in crnih plosckov
  for (int i = 0; i < BOARD_HEIGHT; i++)
  for (int j = 0; j < BOARD_WIDTH; j++)
    sum += board[i*BOARD_WIDTH + j];

  return sum;
}


//Prestejemo koliko je vseh plosckov na igralnem polju
int countPieces(char *board){
  int sum = 0;

  for (int i = 0; i < BOARD_HEIGHT; ++i)
  for (int j = 0; j < BOARD_WIDTH; ++j)
  if (board[i*BOARD_WIDTH + j] != 0)
    sum++;

  return sum;

}

//Rekurzivni minimax algoritem
int minimax(char *board, int player_color, int max_depth, int depth, int row, int column){
  //Postavimo ploscek
  placePiece(row, column, player_color, board);

  //Nastavimo najmanjse mozno zacetno stevilo tock
  int score = -INT_MAX*player_color;

  //Pogledamo ce smo ze prisli do najvecje mozne globine rekurzije
  if (depth > max_depth)
  {
    score = evaluateBoard(board);

    return score;
  }
  //Generiramo vse mozne poteze nasprotnega igralca in se spustimo en nivo nizje
  int moved = 0;
  char tempBoard[BOARD_SIZE];
  for (int i = 0; i < BOARD_HEIGHT; i++)
  for (int j = 0; j < BOARD_WIDTH; j++)
    //Preverimo ce je nasprotnikova poteza veljavna
  if (isValidMove(-player_color, board, i, j)) {
    moved = 1;
    //Ustvarimo si zacasno igralno polje
    memcpy(tempBoard, board, BOARD_SIZE);
    //Spustimo se en nivo nizje po rekurziji in odigramo potezo
    int tempScore = minimax(tempBoard, -player_color, max_depth, depth + 1, i, j);
    //Pogledamo ce je poteza smiselna
    if (tempScore*player_color > score*player_color){
      score = tempScore;
    }
  }
  //Ce ni bilo mozno narediti nobene poteze evalviramo igralno polje in koncamo
  if (!moved)
  {
    score = evaluateBoard(board);
  }
  return score;
}

void *minimax_thread(void *args) {
  struct params *arguments = (struct params *) args; //potreben cast argumentov nazaj v pravi tip
  for (int i = arguments->displacement; i < arguments->count; i++)
  {
    ret_score[i] = minimax(arguments->board, arguments->player_color, arguments->max_depth, arguments->depth, moves[i].i, moves[i].j);
  }

  return NULL;
}

//Racunalniski igralec
int computerMove(char * board, int player_color, int max_depth){
  int row = -1;
  int column = -1;
  int score = -INT_MAX*player_color;

  //Preverimo ce je ze konec igre
  if (isGameOver(board))
    return 0;

  int n_moves = 0;

  memset(ret_score, 0, sizeof(ret_score));
  memset(moves, 0, sizeof(moves));

  //Generiramo in izvedemo vse mozne poteze racunalnika
  for (int i = 0; i < BOARD_HEIGHT; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++){
      if (board[i*BOARD_WIDTH + j] == EMPTY && isValidMove(player_color, board, i, j)) {
        moves[n_moves].i = i;
        moves[n_moves].j = j;
        n_moves++;
      }
    }
  }

  int rem = n_moves%NTHREADS; //elementi ki so ostali po delitvi med niti
  int sendcounts[NTHREADS];   //koliko elementov posljemo vsaki niti
  int displs[NTHREADS];       //odmiki za vsak segment
  int sum = 0;

  pthread_t threads[NTHREADS];
  struct params args[NTHREADS];

  printf("n_moves = %d\n",n_moves);

  for (int i = 0; i < NTHREADS; i++) {
    sendcounts[i] = n_moves / NTHREADS;
    if (rem > 0) {
      sendcounts[i]++;
      rem--;
    }

    displs[i] = sum;
    sum += sendcounts[i];

    //printf("Thread %d -> disp=%d sendcount=%d\n",i,displs[i], sendcounts[i]);
  }

  int dynamic_n = 0;
  if (n_moves < NTHREADS) dynamic_n = n_moves;
  else dynamic_n = NTHREADS;

  for (int i = 0; i < dynamic_n; i++)
  {
    memcpy(args[i].board, board, BOARD_SIZE);
    args[i].player_color = player_color;
    args[i].max_depth = max_depth;
    args[i].depth = 1;
    args[i].count = sendcounts[i];
    args[i].displacement = displs[i];
    args[i].id = i;

    pthread_create(&threads[i], NULL, minimax_thread, (void *)&args[i]);
  }

  for (int i = 0; i < dynamic_n; i++)
  {
    pthread_join(threads[i], NULL);
  }

  for (int i = 0; i < n_moves; i++)
  {
    //Najdemo najboljso potezo
    if (ret_score[i] * player_color > score*player_color) {
      score = ret_score[i];
      row = moves[i].i;
      column = moves[i].j;
    }
  }

  //Izvedemo najbolje ocenjeno potezo
  if (row >= 0 && column >= 0)
    placePiece(row, column, player_color, board);
  return 1;
}

//Nakljucni racunalniski igralec
int computerRandomMove(char * board, int player_color, int max_depth){
  int row = -1;
  int column = -1;
  //Preverimo ce je ze konec igre
  if (isGameOver(board))
    return 0;

  struct move{
    int row;
    int column;
  };
  struct move available_moves[64];
  //Generiramo vse mozne poteze racunalnika
  int n_moves = 0;
  for (int i = 0; i < BOARD_HEIGHT; i++) {
    for (int j = 0; j < BOARD_WIDTH; j++){
      if (board[i*BOARD_WIDTH + j] == EMPTY && isValidMove(player_color, board, i, j)) {
        available_moves[n_moves].row = i;
        available_moves[n_moves].column = j;
        n_moves++;
      }
    }
  }

  //Izberemo nakljucno potezo
  if (n_moves>0){
    int move_index = rand() % n_moves;
    row = available_moves[move_index].row;
    column = available_moves[move_index].column;
    if (row >= 0 && column >= 0)
      placePiece(row, column, player_color, board);
  }
  return 1;
}

int main() {
  FILE *f = fopen("testi_pthreads.txt", "a");
  fprintf(f, "------------------------------------------------------------------------------------------\n");
  for (int i = 1; i <= NGAMES; i++)
  {
    //Inicializiramo zacetno polje
    char board[64] = { 0 };
    initBoard(board);
    //Nastavimo barve igralcev
    int player1 = BLACK; //Player X
    int player2 = WHITE; //Player O
    int score;
    srand(time(NULL));
    printf("-------START-------\n");
    printBoard(board);
    float moves_times = 0;

    //Izvajamo poteze dokler ni konec igre
    int moves = 0;

    double t_all_1 = clock();
    while (1){
      printf("\nPlayer X:\n");

      double t1 = clock();
      if (!computerMove(board, player1, MAX_DEPTH))
        break;
      double t2 = clock();

      moves_times += (t2 - t1) / (float)CLOCKS_PER_SEC;

      printf("(%d) Time for minimax computer move: %f\n", moves + 1, (t2 - t1) / (float)CLOCKS_PER_SEC);
      moves++;

      printBoard(board);
      printf("\nPlayer O:\n");
      if (!computerRandomMove(board, player2, 1))
        break;
      printBoard(board);
    }

    double t_all_2 = clock();

    //Izpisemo rezultat
    printf("-----GAME OVER-----\n");
    printBoard(board);
    score = evaluateBoard(board);
    int total = countPieces(board);
    printf("SCORE: Player X:%d Player O:%d\n", (total - score) / 2, (total - score) / 2 + score);

    float avg_move_time = moves_times / moves;

    printf("\nAverage move time for %d moves: %f\n", moves, avg_move_time);
    printf("Total time: %f\n", (t_all_2 - t_all_1) / (float)CLOCKS_PER_SEC);

    //Zapisemo v .txt datoteko za lazje spremljanje rezultatov - avtomatizacija testiranja
    if (f == NULL)
    {
      printf("Error opening file!\n");
      exit(1);
    }

    fprintf(f, "GAME %d/%d\n", i, NGAMES);
    fprintf(f, "NTHREADS %d -- MAX_DEPTH %d\n", NTHREADS, MAX_DEPTH);
    fprintf(f, "SCORE: Player X:%d Player O:%d\n", (total - score) / 2, (total - score) / 2 + score);
    fprintf(f, "Average move time for %d moves: %f\n", moves, avg_move_time);
    fprintf(f, "Total time: %f\n", (t_all_2 - t_all_1) / (float)CLOCKS_PER_SEC);
    fprintf(f, "\n\n");
  }

  fclose(f);
  putchar('\a');
  getchar();

  return 0;
}

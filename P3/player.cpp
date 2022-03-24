#include <iostream>
#include <cstdlib>
#include <vector>
#include <queue>
#include "player.h"
#include "environment.h"

using namespace std;

const double masinf=9999999999.0, menosinf=-9999999999.0;

Player::Player(int jug){
    jugador_=jug;
}

void Player::Perceive(const Environment & env){
    actual_=env;
}

double Puntuacion(int jugador, const Environment &estado){
    double suma=0;

    for (int i=0; i<7; i++)
      for (int j=0; j<7; j++){
         if (estado.See_Casilla(i,j)==jugador){
            if (j<3)
               suma += j;
            else
               suma += (6-j);
         }
      }

    return suma;
}

double ValoracionTest(const Environment &estado, int jugador){
    int ganador = estado.RevisarTablero();

    if (ganador==jugador)
         return 99999999.0; // Gana el jugador que pide la valoración
    else if (ganador!=0)
         return -99999999.0; // Pierde el jugador que pide la valoración
    else if (estado.Get_Casillas_Libres()==0)
         return 0;  // Hay un empate global
    else
      return Puntuacion(jugador,estado);
}

//
// ──────────────────────────────────────────────────────────────────
//                            HEURÍSTICA
// ──────────────────────────────────────────────────────────────────
//

int adyacentesVaciasVertical(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool 
      cond1 = fila+1<7 and estado.See_Casilla(fila+1,col) == 0,
      cond2 = fila+2<7 and estado.See_Casilla(fila+2,col) == 0;
   if (nConsec == 3){
      if (cond1){
         nCombinaciones++;
      }
   } else {
      if (cond1 and cond2){
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int consecutivasVertical(int fila, int col, const Environment & estado, int nConsec, int jugador){
   int consec = 0;
   if (fila + nConsec- 1 < 7) {
      for (int i=0; i<nConsec;i++) {
         if (estado.See_Casilla(fila,col) == estado.See_Casilla((fila+i),col) or
            estado.See_Casilla(fila,col) == estado.See_Casilla((fila+i),col)+3) {
               consec++;
         } else {
            break;
         }
      }
   }
   if (nConsec == consec) {
      return adyacentesVaciasVertical(fila+nConsec-1,col,estado,nConsec,jugador);
   }  
   return 0;
}

int adyacentesHorizontalDcha(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condDcha1 = col+1<7 and estado.See_Casilla(fila,col+1) == 0,
        condDcha2 = col+2<7 and estado.See_Casilla(fila,col+2) == 0;

   if (nConsec == 2){
      if (condDcha1 and condDcha2){
         nCombinaciones++;
      }
   } else {
      if (condDcha1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesHorizontalIzda(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condIzda1 = col-1>=0 and estado.See_Casilla(fila,col-1) == 0,
        condIzda2 = col-2>=0 and estado.See_Casilla(fila,col-2) == 0;

   if (nConsec == 2){
      if (condIzda1 and condIzda2){
         nCombinaciones++;
      }
   } else {
      if (condIzda1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesHorizontalAmbosLados(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool cond1 = col-1>=0 and estado.See_Casilla(fila,col-1) == 0,
        cond2 = col+2 <7 and estado.See_Casilla(fila,col+2) == 0;

   if (nConsec == 2){
      if (cond1 and cond2){
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int consecutivasHorizontal(int fila, int col, const Environment & estado, int nConsec, int jugador){
   int consec = 0;
   if (col + nConsec - 1 < 7){
      for (int i=0; i<nConsec; i++) {
         if (estado.See_Casilla(fila,col) == estado.See_Casilla(fila,(col+i)) or
            estado.See_Casilla(fila,col) == estado.See_Casilla(fila,(col+i))+3) {
               consec++;
         } else {
            break;
         }
      }
   }
   if (nConsec == consec){
      return
         ( adyacentesHorizontalDcha(fila, col+nConsec-1, estado, nConsec, jugador)
         + adyacentesHorizontalIzda(fila, col, estado, nConsec, jugador) 
         + adyacentesHorizontalAmbosLados(fila, col, estado, nConsec, jugador) );
   }
   return 0;
}

int adyacentesAntidiagonalDcha(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condDcha1 = col+1<7 and fila+1<7 and estado.See_Casilla(fila+1,col+1) == 0,
        condDcha2 = col+2<7 and fila+2<7 and estado.See_Casilla(fila+2,col+2) == 0;

   if (nConsec == 2){
      if (condDcha1 and condDcha2){
         nCombinaciones++;
      }
   } else {
      if (condDcha1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesAntidiagonalIzda(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condIzda1 = col-1>=0 and fila-1>=0 and estado.See_Casilla(fila-1,col-1) == 0,
        condIzda2 = col-2>=0 and fila-2>=0 and estado.See_Casilla(fila-2,col-2) == 0;

   if (nConsec == 2){
      if (condIzda1 and condIzda2){
         nCombinaciones++;
      }
   } else {
      if (condIzda1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesAntidiagonalAmbosLados(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool cond1 = col-1>=0 and fila-1>=0 and estado.See_Casilla(fila-1,col-1) == 0,
        cond2 = col+2<7 and fila+2<7 and estado.See_Casilla(fila+2,col+2) == 0;

   if (nConsec == 2){
      if (cond1 and cond2){
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesDiagonalPrincipalDcha(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condDcha1 = col+1<7 and fila-1>=0 and estado.See_Casilla(fila-1,col+1) == 0,
        condDcha2 = col+2<7 and fila-2>=0 and estado.See_Casilla(fila-2,col+2) == 0;

   if (nConsec == 2){
      if (condDcha1 and condDcha2){
         nCombinaciones++;
      }
   } else {
      if (condDcha1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesDiagonalPrincipalIzda(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool condIzda1 = col-1>=0 and fila+1<7 and estado.See_Casilla(fila+1,col-1) == 0,
        condIzda2 = col-2>=0 and fila+2<7 and estado.See_Casilla(fila+2,col-2) == 0;

   if (nConsec == 2){
      if (condIzda1 and condIzda2){
         nCombinaciones++;
      }
   } else {
      if (condIzda1) {
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int adyacentesDiagonalPrincipalAmbosLados(int fila, int col, const Environment &estado, int nConsec, int jugador){
   int nCombinaciones = 0;
   bool cond1 = col-1>=0 and fila+1<7 and estado.See_Casilla(fila+1,col-1) == 0,
        cond2 = col+2<7 and fila-2>=0 and estado.See_Casilla(fila-2,col+2) == 0;

   if (nConsec == 2){
      if (cond1 and cond2){
         nCombinaciones++;
      }
   }
   return nCombinaciones;
}

int consecutivasDiagonal(int fila, int col, const Environment & estado, int nConsec, int jugador){
   int
      consec = 0,
      nCombinaciones = 0;
   if (fila + nConsec - 1 < 7 and col + nConsec - 1 < 7){
      for (int i=0; i<nConsec;i++){
         if (estado.See_Casilla(fila,col) == estado.See_Casilla((fila+i),(col+i)) or
            estado.See_Casilla(fila,col) == estado.See_Casilla((fila+i),(col+i))+3) {
               consec++;
         }
         else break;
      }
   }
   if (nConsec == consec){
      nCombinaciones =
         ( adyacentesAntidiagonalDcha(fila+nConsec-1, col+nConsec-1, estado, nConsec, jugador)
         + adyacentesAntidiagonalIzda(fila, col, estado, nConsec, jugador) 
         + adyacentesAntidiagonalAmbosLados(fila, col, estado, nConsec, jugador) );
   }
   consec = 0;
   if (fila - nConsec + 1 >= 0 and col + nConsec - 1 < 7){
      for (int i=0; i<nConsec;i++){
         if (estado.See_Casilla(fila,col) == estado.See_Casilla((fila-i),(col+i)) or
            estado.See_Casilla(fila,col) == estado.See_Casilla((fila-i),(col+i))+3) {
               consec++;
         }
         else break;
      }
   }
   if (nConsec == consec){
      nCombinaciones +=
         ( adyacentesDiagonalPrincipalDcha(fila, col+nConsec-1, estado, nConsec, jugador)
         + adyacentesDiagonalPrincipalIzda(fila+nConsec-1, col, estado, nConsec, jugador) 
         + adyacentesDiagonalPrincipalAmbosLados(fila, col, estado, nConsec, jugador) );
   }
   return nCombinaciones;
}

double consecutivas(const Environment &estado, int jugador, int nConsec){
   int contador = 0;

   for (int i=0; i<7; i++){
      for (int j=0; j<7; j++){
         if (estado.See_Casilla(i,j) == jugador or estado.See_Casilla(i,j) == jugador+3){
            contador += consecutivasVertical(i,j,estado,nConsec,jugador);
            contador += consecutivasHorizontal(i,j,estado,nConsec,jugador);
            contador += consecutivasDiagonal(i,j,estado,nConsec,jugador);
         }
      }
   }
   return contador;
}

double Valoracion(const Environment &estado, int jugador){
   int
      ganador = estado.RevisarTablero(),
      rival = 1;
   double
      rival_tres, rival_dos, jug_tres, jug_dos, 
      jug, oponente;

   if (jugador == 1) {
      rival = 2;
   }

   if (ganador==jugador)
      return 99999999.0; // Gana el jugador que pide la valoración
   else if (ganador!=0)
      return -99999999.0; // Pierde el jugador que pide la valoración
   else if (estado.Get_Casillas_Libres()==0)
      return 0;  // Hay un empate global
   else {
      rival_tres = consecutivas(estado, rival, 3);
      rival_dos = consecutivas(estado, rival, 2);
      jug_tres = consecutivas(estado, jugador, 3);
      jug_dos = consecutivas(estado, jugador, 2);

      jug = (jug_tres + jug_dos);
      oponente = (rival_tres + rival_dos);

      int result = jug-oponente;
      result += Puntuacion(jugador, estado);
      for (int j=0;j<7;j++) {
         if (estado.Get_Ocupacion_Columna(j)>4) {
            for (size_t i =0; i<7; i++){
               if (estado.See_Casilla(i,j)==jugador) {
                  result += (7-i);
               }
            }
         }
      }
      return result;
   }
}

void JuegoAleatorio(bool aplicables[], int opciones[], int &j){
    j=0;
    for (int i=0; i<8; i++){
        if (aplicables[i]){
           opciones[j]=i;
           j++;
        }
    }
}

Environment::ActionType Player::Think(){
   const int PROFUNDIDAD_ALFABETA = 8; // Umbral máximo de profundidad para el método de poda Alpha-Beta

   Environment::ActionType accion; // acción que se va a devolver
   bool aplicables[8];  // Vector de booleanos empleado para obtener las acciones aplicables en el estado actual
                        // aplicables[0]==true si PUT1 es aplicable
                        // aplicables[1]==true si PUT2 es aplicable
                        // aplicables[2]==true si PUT3 es aplicable
                        // aplicables[3]==true si PUT4 es aplicable
                        // aplicables[4]==true si PUT5 es aplicable
                        // aplicables[5]==true si PUT6 es aplicable
                        // aplicables[6]==true si PUT7 es aplicable
                        // aplicables[7]==true si BOOM es aplicable

   double valor; // Almacena el valor con el que se etiqueta el estado tras el proceso de búsqueda
   double alpha, beta; // Cotas de la poda Alpha-Beta

   int n_act; //Posibles acciones en el estado actual

   n_act = actual_.possible_actions(aplicables); // Obtengo las acciones aplicables en el estado actual en "aplicables"
   int opciones[10];

   // Muestra por consola las acciones aplicables
   cout << "Acciones aplicables ";
   (jugador_==1) ? cout << "Verde: " : cout << "Azul: ";
   for (int t=0; t<8; t++)
   if (aplicables[t])
      cout << " " << actual_.ActionStr( static_cast< Environment::ActionType > (t)  );
   cout << endl;
   
   alpha = menosinf; 
   beta = masinf;     
   
   valor = poda_AlfaBeta(actual_, jugador_, PROFUNDIDAD_ALFABETA, accion, alpha, beta);

   return accion;
}

//
// ──────────────────────────────────────────────────────────────────
//                         PODA ALPHA-BETA
// ──────────────────────────────────────────────────────────────────
//

void max(double & max, double value, Environment::ActionType & act, int best_act){
   if(value > max){
      max = value;
      act = static_cast< Environment::ActionType > (best_act);
   }
}

void min(double & min, double value, Environment::ActionType & act, int best_act){
   if(value < min){
      min = value;
      act = static_cast< Environment::ActionType > (best_act);
   }
}

double Player::poda_AlfaBeta (const Environment & actual, int jugador, int profundidad, Environment::ActionType & accion, double alpha, double beta){

   bool opciones[8];
   int hijos = actual.possible_actions(opciones);
   Environment::ActionType ultima_accion;

   if (actual.JuegoTerminado() || profundidad == 0 || hijos == 0)
      return Valoracion(actual, jugador); 
   else{
      int mejor_accion = -1; 
      Environment nodo = actual.GenerateNextMove(mejor_accion); 

      if(jugador == actual.JugadorActivo()){

         int cont = 0;         
         while (cont < hijos && beta > alpha){
            cont++;
            max(alpha, (poda_AlfaBeta(nodo, jugador, profundidad-1 , ultima_accion, alpha, beta)), accion, mejor_accion);
            nodo = actual.GenerateNextMove(mejor_accion);
         }

         return alpha;

      }else{
         int cont = 0;         
         while (cont < hijos && beta > alpha){

         cont++;
         min(beta, (poda_AlfaBeta(nodo, jugador, profundidad-1 , ultima_accion, alpha, beta)), accion, mejor_accion);
         nodo = actual.GenerateNextMove(mejor_accion);

         }
         return beta;
      }
   }
}
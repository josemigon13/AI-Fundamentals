/*  José Miguel González Cañadas
	3º curso en el Doble Grado en Ingeniería Informática y Matemáticas
	DNI 77184606X
*/

#include "../Comportamientos_Jugador/jugador.hpp"
#include "motorlib/util.h"
#include <iostream>
#include <unordered_set>
#include <stack>
#include <queue>
#include <cassert>
#include <iterator>
#include <limits>

void ComportamientoJugador::calcularObjetivos(Sensores sensores)
{
	cout << "sensores.num_destinos : " << sensores.num_destinos << endl;
	obj.clear();
	for (int i=0; i<sensores.num_destinos; i++){
		estado st;
		st.fila = sensores.destino[2*i];
		st.columna = sensores.destino[2*i+1];
		obj.push_back(st);
	}
}

// Distancia de Manhattan (base de nuestra heurística)
int distManhattan(const estado origen, const estado obj){
	return abs(origen.fila-obj.fila) + abs(origen.columna-obj.columna);
}
/* 
	Éste es el método principal de la práctica.
   	Entrada: información de los sensores.
   	Salida: acción a realizar.
   	Consultar fichero "comportamiento.hpp"
*/
Action ComportamientoJugador::think(Sensores sensores) {
	Action act = actIDLE;

	actual.fila        = sensores.posF;
	actual.columna     = sensores.posC;
	actual.orientacion = sensores.sentido;

	cout << "Fila: " << actual.fila << endl;
	cout << "Col : " << actual.columna << endl;
	cout << "Ori : " << actual.orientacion << endl;

	if (sensores.nivel != 4){

		calcularObjetivos(sensores);
		if(!hayPlan)
			hayPlan=pathFinding(sensores.nivel, actual, obj, plan);
		else{
			if (plan.size() > 0){
				act = plan.front();
				plan.erase(plan.begin());
			} else {
				cout << "Terminada la búsqueda\n";
			}	
		}
	} else{
		act = thinkNivel4(sensores);
	}
  	return act;
}

Action ComportamientoJugador::thinkNivel4(const Sensores& sensores) {
    instante++;
 
    // Inicialización de objetivos
    if (obj.empty()) {
        calcularObjetivos(sensores);
    }
	ActualizarObjs(actual, obj.begin());
 
    // Actualizar el mapa ahora que sabemos nuestra posición y orientación
    actualizarMapa(sensores);
 
    // Actualizar bikini y zapatillas ahora que sabemos el mapa
    actualizarBikZap(actual);
 
    // Si estamos en la batería, esperar a que recarguemos
    if (mapaResultado[actual.fila][actual.columna] == 'X') {
        yendo_a_recargar = false;
        int esperarBateria;
        esperarBateria = (instante > 2700 ? 500 : 2900);
        if (sensores.bateria < esperarBateria) {
            return Action::actIDLE;
        }
    }
 
    bool calcular_plan = false;
    // Si hemos descubierto nueva información del mapa, calcular plan de nuevo
    if (mapaResultado != oldMapaResultado)
        calcular_plan = true;
    oldMapaResultado = mapaResultado;
 
    // Si el estado actual tiene los tres objetivos, calcular los nuevos
    // objetivos, actualizar el estado actual y calcular plan
    if (actual.obj[0] and 
		actual.obj[1] and 
		actual.obj[2]) 
	{
        calcularObjetivos(sensores);
        for (int i = 0; i < 3; i++)
            actual.obj[i] = false;
        ActualizarObjs(actual, obj.begin());
        calcular_plan = true;
    }
 
    estado recarga;
	bool recarga_encontrada = recargaMasCercana(actual, recarga, recargas.begin());
    if (recarga_encontrada and 
		distManhattan(actual, recarga) <= 4 and 
		sensores.bateria < 700)
	{
        yendo_a_recargar = true;
        calcular_plan = true;
    }
 
    if (sensores.bateria < 500 and recarga_encontrada) {
        yendo_a_recargar = true;
        calcular_plan = true;
    }

	objetivo_actual = objetivoMasCercanoNoConseguido(actual, obj.begin());
	int dist = distManhattan(actual, objetivo_actual);
	ir_con_anchura = actual.bik ? (dist <= 16 ? (instante > 2000 ? (sensores.bateria > 2000 ? true : false) : false) : false): 
	actual.zap ? (dist <= 8 ? (instante > 1750 ? (sensores.bateria > 2200 ? true : false) : false) : false) : 
	dist <= 12 ? (instante > 2500 ? (sensores.bateria > 2500 ? true : false) : false) : false;

    if (calcular_plan or plan.empty()) {
        plan.clear();
        bool found_a_plan;
        if (yendo_a_recargar) {
            found_a_plan = pathFinding_AEstrella(actual, recarga, plan, 4);
        } else if (ir_con_anchura){
			found_a_plan = pathFinding_Anchura(actual, objetivo_actual, plan);
		} else {
            found_a_plan = pathFinding_AEstrella(actual, objetivo_actual, plan, 4);
        }
        VisualizaPlan(actual, plan);
    }
 
    if (plan.empty())
        return Action::actIDLE;
 
    Action accion = plan.front();
 
    // Esperar pacientemente a que el aldeano se quite de en medio
    if (accion == Action::actFORWARD && sensores.superficie[2] == 'a')
        return Action::actIDLE;
 
    plan.erase(plan.begin());
    return accion;
}
/**
 * @brief  Calcula la recarga más cercana y la devuelve en 'recarga'
 * @param  actual: estado actual
 * @param  recarga : recarga más cercana
 * @return Si la ha encontrado
 */
bool ComportamientoJugador::recargaMasCercana(const estado& actual, estado& recarga, list<estado>::const_iterator it){
	bool found=false;
	int d=numeric_limits<int>::max();
	while (it != recargas.end()){
		int d_nueva = distManhattan(actual, *it);
		if (d_nueva < d){
			recarga=*it;
			d = d_nueva;
			found=true;
		}
		++it;
	}
	return found;
}

estado ComportamientoJugador::objetivoMasCercanoNoConseguido(const estado &actual, list<estado>::const_iterator it)
{
	int d=numeric_limits<int>::max();
	estado obj;
	for (int i=0; i<3; i++, ++it){
		if (!actual.obj[i]){
			int d_nueva = distManhattan(actual, *it);
			if (d_nueva < d){
				obj=*it;
				d = d_nueva;
			}
		}
	}
	return obj;
}

// Llama al algoritmo de búsqueda que se utilizará en cada comportamiento del agente
// Level representa el comportamiento en el que fue iniciado el agente
bool ComportamientoJugador::pathFinding (int level, const estado &origen, const list<estado> &destino, list<Action> &plan){
	switch (level){
		case 0:
			cout << "Nivel 0\n";
		    return pathFinding_Profundidad(origen,obj.front(),plan);
		break;

		case 1:
			cout << "Nivel 1\n";
		    return pathFinding_Anchura(origen,obj.front(),plan);
		break;

		case 2:
			cout << "Nivel 2\n";
		    return pathFinding_AEstrella(origen,obj.front(),plan,2);
		break;

		case 3:
			cout << "Nivel 3\n";
		    return pathFinding_AEstrella3(origen,obj,plan);
		break;
	}

	return false;
}

/**
 * @brief Actualizar el mapa visible
 * Además de actualizarlo, guarda la localización de las recargas
 * @param sensores: sensores del juego
 */
void ComportamientoJugador::actualizarMapa(Sensores sensores) {
	int a = 0;
	int a_f, a_c, b_f, b_c;
	int F = sensores.posF, C = sensores.posC;

	switch (sensores.sentido) {
		case norte:	a_f = 0; a_c = 1; b_f = -1; b_c = -1;	break;
		case sur:	a_f = 0; a_c = -1; b_f = 1; b_c = 1;	break;
		case este:	a_f = 1; a_c = 0; b_f = -1; b_c = 1;	break;
		case oeste:	a_f = -1; a_c = 0; b_f = 1; b_c = -1;	break;
	}

	for (int i = 0; i <=3; i++) {
		for (int j=0; j < 2*i+1; j++) {
			if ( mapaResultado[F+j*a_f][C+j*a_c] == '?' ) {
				// actualizamos casilla '?'
				mapaResultado[F+j*a_f][C+j*a_c]= sensores.terreno[a];
				if ( sensores.terreno[a] == 'X' )
					recargas.push_back(estado{F+j*a_f, C+j*a_c});
			}
			a++;
		}
		F += b_f; C += b_c;
	}
}

int ComportamientoJugador::interact(Action accion, int valor){
  return false;
}

void reconstruirPlan(const estado& final, list<Action>& plan) {
	estado current = final;
	while (current.padre) {
		estado padre = *current.padre;
		if (current.fila != padre.fila || current.columna != padre.columna) {
			plan.push_front(actFORWARD);
		} else if (current.orientacion == (padre.orientacion + 1)%4) {
			plan.push_front(actTURN_R);
		} else if (current.orientacion == (padre.orientacion + 3)%4) {
			plan.push_front(actTURN_L);
		} else {
			assert(false);
		}
		current = padre;
	}
}
bool ComportamientoJugador::enRango(const estado & st){
	return ((st.fila>=0) and (st.columna>=0) and (st.fila<mapaResultado.size()) and (st.columna<mapaResultado[0].size()));
}

estado ComportamientoJugador::EstadoForward(const estado&st){
	int fil=st.fila, col=st.columna;
	switch (st.orientacion) {
		case 0: fil--; break;
		case 1: col++; break;
		case 2: fil++; break;
		case 3: col--; break;
	}
	return {fil,col,st.orientacion, st.zap, st.bik, {st.obj[0], st.obj[1], st.obj[2]}, &st};
}

estado EstadoTurnR(const estado &st){
	return{st.fila, st.columna, (st.orientacion+1)%4, st.zap, st.bik, {st.obj[0], st.obj[1], st.obj[2]}, &st};
}
estado EstadoTurnL(const estado &st){
	return{st.fila, st.columna, (st.orientacion+3)%4, st.zap, st.bik, {st.obj[0], st.obj[1], st.obj[2]}, &st};
}

bool ComportamientoJugador::esObstaculo(estado &st){
	int fil=st.fila, col=st.columna;
	 unsigned char casilla = mapaResultado[fil][col];

	// Compruebo que no me salgo fuera del rango del mapa
	if (fil<0 or fil>=mapaResultado.size()) return true;
	if (col<0 or col>=mapaResultado[0].size()) return true;

	// Compruebo si en esa casilla hay un obstáculo infranqueable
	if (casilla == 'P' or casilla == 'M')
		return true;
	else
	  	return false;
}

// Sacar por la consola la secuencia del plan obtenido
void ComportamientoJugador::PintaPlan(list<Action> plan) {
	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			cout << "A ";
		} else if (*it == actTURN_R){
			cout << "D ";
		} else if (*it == actTURN_L){
			cout << "I ";
		} else {
			cout << "- ";
		}
		it++;
	}
	cout << endl;
}


// Función auxiliar para establecer a cero todas las casillas de una matriz
void AnularMatriz(vector<vector<unsigned char> > &m){
	for (int i=0; i<m[0].size(); i++){
		for (int j=0; j<m.size(); j++){
			m[i][j]=0;
		}
	}
}


// Pinta sobre el mapa el plan calculado
void ComportamientoJugador::VisualizaPlan(const estado & st, const list<Action> &plan){
  	AnularMatriz(mapaConPlan);
	  estado cst = st;
	auto it = plan.begin();
	while (it!=plan.end()){
		if (*it == actFORWARD){
			switch (cst.orientacion) {
				case 0: cst.fila--; break;
				case 1: cst.columna++; break;
				case 2: cst.fila++; break;
				case 3: cst.columna--; break;
			}
			mapaConPlan[cst.fila][cst.columna]=1;
		} else if (*it == actTURN_R){
			cst.orientacion = (cst.orientacion+1)%4;
		} else {
			cst.orientacion = (cst.orientacion+3)%4;
		}
		++it;
	}
}

struct Hasher {
	public:
		size_t operator()(const estado&a) const{
			size_t hash = a.fila;
			hash |= a.columna << 8;
			hash |= a.orientacion << 16;
			hash |= a.bik << 18;
			hash |= a.zap << 19;
			for (int i=0; i<3; i++){
				hash |= a.obj[i] << (20+i);
			}
			return hash;
		}
};

struct ComparaEstados {
	bool operator()(const estado& a, const estado& n) const {
		return (a.fila == n.fila && a.columna == n.columna && a.orientacion == n.orientacion &&
		a.bik == n.bik && a.zap == n.zap && a.obj[0] == n.obj[0] && a.obj[1] == n.obj[1] && a.obj[2] == n.obj[2]);
	}
};

struct ComparaNodosSegunEstado {
	bool operator()(const estado& st1, const estado& st2) const {
		return ComparaEstados()(st1, st2);
	}
};

struct ComparaNodosSegunCosteYHeuristica {
	bool operator()(const estado& st1, const estado& st2) const {
		return st1.g_n + st1.h_n > st2.g_n + st2.h_n;
	}
};

/*---------------------- Implementación de la búsqueda en profundidad ----------------------*/


bool ComportamientoJugador::pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan) {

	cout << "Calculando plan...\n";
	plan.clear();
	unordered_set<estado,Hasher,ComparaNodosSegunEstado> closedSet;
	stack<estado> openSet;

  	estado current = origen;
	current.padre=nullptr;

	openSet.push(current);

  	while (!openSet.empty() and
  		  (current.fila!=destino.fila or current.columna != destino.columna)){

  		openSet.pop();

		if (closedSet.find(current) == closedSet.end()){
			auto it = (closedSet.insert(current)).first;
			estado hijoTurnR = EstadoTurnR(*it);
			openSet.push(hijoTurnR);
			estado hijoTurnL = EstadoTurnL(*it);
			openSet.push(hijoTurnL);
			estado hijoForward = EstadoForward(*it);
			if (!esObstaculo(hijoForward)){
				openSet.push(hijoForward);
			}
		}

		// Tomo el siguiente valor en openSet
		if (!openSet.empty()){
			current = openSet.top();
		}
	}

  	cout << "Se ha completado la búsqueda\n";

	if (current.fila == destino.fila and current.columna == destino.columna){
		cout << "Cargando el plan...\n";
		reconstruirPlan(current, plan);
		PintaPlan(plan);
		VisualizaPlan(origen, plan);
		return true;
	} else {
		cout << "No se ha podido encontrar ningún plan\n";
		return false;
	}
}

/*---------------------- Implementación de la búsqueda en anchura ----------------------*/

bool ComportamientoJugador::pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan) {

	cout << "Calculando plan...\n";
	plan.clear();
	unordered_set<estado,Hasher,ComparaNodosSegunEstado> closedSet;
	queue<estado> openSet;

  	estado current = origen;
	current.padre=nullptr;

	openSet.push(current);

  	while (!openSet.empty() and
  		  (current.fila!=destino.fila or current.columna != destino.columna)){

  		openSet.pop();

		if (closedSet.find(current) == closedSet.end()){
			auto it = (closedSet.insert(current)).first;
			estado hijoTurnR = EstadoTurnR(*it);
			openSet.push(hijoTurnR);
			estado hijoTurnL = EstadoTurnL(*it);
			openSet.push(hijoTurnL);
			estado hijoForward = EstadoForward(*it);
			if (enRango(hijoForward)){
				if (!esObstaculo(hijoForward)){
					openSet.push(hijoForward);
				}
			}
		}

		// Tomo el siguiente valor en openSet
		if (!openSet.empty()){
			current = openSet.front();
		}
	}

  	cout << "Se ha completado la búsqueda\n";

	if (current.fila == destino.fila and current.columna == destino.columna){
		cout << "Cargando el plan...\n";
		reconstruirPlan(current, plan);
		PintaPlan(plan);
		VisualizaPlan(origen, plan);
		return true;
	} else {
		cout << "No se ha podido encontrar ningún plan\n";
		return false;
	}
}

// Actualizar la posesión de bik y/o zap
void ComportamientoJugador::actualizarBikZap(estado & s){
	int fil = s.fila,
	    col = s.columna;
	if(mapaResultado[fil][col] == 'K'){
		s.bik = true;
		s.zap = !s.bik;
	} else if(mapaResultado[fil][col] == 'D'){
		s.zap = true;
		s.bik = !s.zap;
	}
}

// Coste de batería asociado a una acción
int ComportamientoJugador::coste_bateriaN4(estado est, bool zap, bool bik, Action act){
	char casilla = mapaResultado[est.fila][est.columna];
	double t = (double)instante / 3000;

	if (casilla == 'T') {
		return 2;
	} else if (casilla == '?') {
		if (bik){
			return 15*t + (1-t);
		} else if (zap){
			return 20*t + (1-t);
		} else {
			return 25 *t + (1-t);
		}
	} else {
		switch(act){
		case actFORWARD:
			if (casilla == 'A'){
				return (bik ? t + (1-t)*10 : t*150 + (1-t)*200);
			} else if (casilla == 'B'){
				return (zap ? t + (1-t)*15 : t*40 + (1-t)*100);
			}
		break;
		default:
			if (casilla == 'A'){
				return (bik ? t + (1-t)*5 : t*100 + (1-t)*500);
			} else if (casilla == 'B'){
				return (zap ? 1 : t + (1-t)*3);
			}
		break;
		}
	}
	return 1;
}

// Coste de batería asociado a una acción
int ComportamientoJugador::coste_bateria(estado est, bool zap, bool bik, Action act){
	char casilla = mapaResultado[est.fila][est.columna];

	if (casilla == 'T') {
		return 2;
	} else {
		switch(act){
		case actFORWARD:
			if (casilla == 'A'){
				return (bik ? 10 : 200);
			} else if (casilla == 'B'){
				return (zap ? 15 : 100);
			}
		break;
		default:
			if (casilla == 'A'){
				return (bik ? 5 : 500);
			} else if (casilla == 'B'){
				return (zap ? 1 : 3);
			}
		break;
		}
	}
	return 1;
}

// Expande un estado a partir de su padre
estado ComportamientoJugador::generarSucesor(const estado &padre, Action value, int level) {
	
	estado sucesor;
	switch(value){
		case actTURN_R:
			sucesor = EstadoTurnR(padre);
			sucesor.g_n = padre.g_n;
			sucesor.g_n += level==4 ? coste_bateriaN4(padre, padre.zap, padre.bik, actTURN_R) : coste_bateria(padre, padre.zap, padre.bik, actTURN_R);
		break;
		case actTURN_L:
			sucesor = EstadoTurnL(padre);
			sucesor.g_n = padre.g_n;
			sucesor.g_n += level==4 ? coste_bateriaN4(padre, padre.zap, padre.bik, actTURN_L) : coste_bateria(padre, padre.zap, padre.bik, actTURN_L);
		break;
		case actFORWARD:
			sucesor = EstadoForward(padre);
			sucesor.g_n = padre.g_n;
			sucesor.g_n += level==4 ? coste_bateriaN4(padre, padre.zap, padre.bik, actFORWARD) : coste_bateria(padre, padre.zap, padre.bik, actFORWARD);
		break;
	}
	sucesor.padre=&padre;
	return sucesor;
}

/*---------------------- Implementación de la búsqueda A* con sólo un objetivo ----------------------*/

bool ComportamientoJugador::pathFinding_AEstrella(const estado &origen, const estado &destino, list<Action> &plan, int level) {
	cout << "Calculando plan...\n";
	plan.clear();
	unordered_set<estado,Hasher,ComparaNodosSegunEstado> closedSet;
	priority_queue<estado,vector<estado>,ComparaNodosSegunCosteYHeuristica> openSet;

	estado current = {origen};
	current.padre=nullptr;
	current.h_n=distManhattan(current, destino);

	openSet.push(current);

  	while (!openSet.empty() and
  		  (current.fila!=destino.fila or current.columna != destino.columna)){

  		openSet.pop();
		actualizarBikZap(current);

		if (closedSet.find(current) == closedSet.end()){
			auto it = (closedSet.insert(current)).first;
			estado hijoTurnR = level==4 ? generarSucesor(*it, actTURN_R, 4) : generarSucesor(*it, actTURN_R, 2);
			hijoTurnR.h_n=distManhattan(hijoTurnR, destino);
			openSet.push(hijoTurnR);
			estado hijoTurnL = level==4 ? generarSucesor(*it, actTURN_L, 4) : generarSucesor(*it, actTURN_L, 2);
			hijoTurnL.h_n=distManhattan(hijoTurnL, destino);
			openSet.push(hijoTurnL);
			estado hijoForward = EstadoForward(*it);
			if (enRango(hijoForward)){
				if (!esObstaculo(hijoForward)){
					hijoForward.g_n = current.g_n;
					hijoForward.g_n += level==4 ? coste_bateriaN4(current, current.zap, current.bik, actFORWARD):coste_bateria(current, current.zap, current.bik, actFORWARD);
					hijoForward.h_n=distManhattan(hijoForward, destino);
					openSet.push(hijoForward);
				}
			}
		}

		// Tomo el siguiente valor en openSet
		if (!openSet.empty()){
			current = openSet.top();
		}
	}

  	cout << "Se ha completado la búsqueda\n";

	if (current.fila == destino.fila and current.columna == destino.columna){
		cout << "Cargando el plan...\n";
		reconstruirPlan(current, plan);
		PintaPlan(plan);
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No se ha podido encontrar ningún plan\n";
		return false;
	}
}

// Calcular heurística basada en la distancia de Manhattan
int calcularHeu(const estado & s, list<estado>::const_iterator it){

	int h=0,
		n=0;
	for (int i=0; i<3; i++, ++it){
		if (!s.obj[i]){
			h += distManhattan(s, *it);
			++n;
		}
	}
	if(n!=0) h/=n;

	return h;
}

// Actualizar los obj conseguidos en cada instante
void ComportamientoJugador::ActualizarObjs(estado &s, list<estado>::const_iterator it) {
	for (int i=0; i<3; i++, ++it){
		if (it->fila==s.fila and it->columna==s.columna){
			s.obj[i]=true;
		}
	}
}

/*---------------------- Implementación de la búsqueda A* con tres obj ----------------------*/

bool ComportamientoJugador::pathFinding_AEstrella3(const estado &origen, const list<estado> &destinos, list<Action> &plan) {

	cout << "Calculando plan...\n";
	plan.clear();
	unordered_set<estado,Hasher,ComparaNodosSegunEstado> closedSet;
	priority_queue<estado,vector<estado>,ComparaNodosSegunCosteYHeuristica> openSet;

	estado current = {origen};
	current.padre=nullptr;
	current.h_n=calcularHeu(current, destinos.begin());

	openSet.push(current);

  	while (!openSet.empty() and
  		  (!current.obj[0] or !current.obj[1] or !current.obj[2])){

  		openSet.pop();
		actualizarBikZap(current);

		if (closedSet.find(current) == closedSet.end()){
			auto it = (closedSet.insert(current)).first;
			estado hijoTurnR = generarSucesor(*it, actTURN_R,3);
			ActualizarObjs(hijoTurnR, destinos.begin());
			hijoTurnR.h_n=calcularHeu(hijoTurnR, destinos.begin());
			openSet.push(hijoTurnR);
			estado hijoTurnL = generarSucesor(*it, actTURN_L,3);
			ActualizarObjs(hijoTurnL, destinos.begin());
			hijoTurnL.h_n=calcularHeu(hijoTurnL, destinos.begin());
			openSet.push(hijoTurnL);
			estado hijoForward = EstadoForward(*it);
			if (!esObstaculo(hijoForward)){
				hijoForward.g_n = current.g_n + coste_bateria(current, current.zap, current.bik, actFORWARD);
				ActualizarObjs(hijoForward, destinos.begin());
				hijoForward.h_n=calcularHeu(hijoForward, destinos.begin());
				openSet.push(hijoForward);
			}
		}

		// Tomo el siguiente valor en openSet
		if (!openSet.empty()){
			current = openSet.top();
		}
	}

  	cout << "Se ha completado la búsqueda\n";

	if (current.obj[0] and current.obj[1] and current.obj[2]){
		cout << "Cargando el plan...\n";
		reconstruirPlan(current, plan);
		PintaPlan(plan);
		VisualizaPlan(origen, plan);
		return true;
	}
	else {
		cout << "No se ha podido encontrar ningún plan\n";
		return false;
	}
}
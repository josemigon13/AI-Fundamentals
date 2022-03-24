/*  José Miguel González Cañadas
	  3º curso en el Doble Grado en Ingeniería Informática y Matemáticas
	  DNI 77184606X
*/

#ifndef COMPORTAMIENTOJUGADOR_H
#define COMPORTAMIENTOJUGADOR_H

#include "comportamientos/comportamiento.hpp"
#include <list>
#include <queue>
#include <iterator>
#include <set>

struct estado {
  int fila;
  int columna;
  int orientacion;
  bool zap; bool bik;
  bool obj[3];
  const estado * padre;
  int g_n; int h_n;
};

class ComportamientoJugador : public Comportamiento {
  public:
    ComportamientoJugador(unsigned int size) : Comportamiento(size), actual{}, objetivo_actual{}, hayPlan(false),  
    yendo_a_recargar(false), instante(0), ir_con_anchura(false){}
    ComportamientoJugador(std::vector< std::vector< unsigned char> > mapaR) : Comportamiento(mapaR), actual{}, objetivo_actual{}, hayPlan(false),  
    yendo_a_recargar(false), instante(0), ir_con_anchura(false){}
    ComportamientoJugador(const ComportamientoJugador & comport) : Comportamiento(comport), actual{}, objetivo_actual{}, hayPlan(false),  
    yendo_a_recargar(false), instante(0), ir_con_anchura(false){}
    ~ComportamientoJugador(){}

    Action think(Sensores sensores);
    Action thinkNivel4(const Sensores& sensores);
    int interact(Action accion, int valor);
    void VisualizaPlan(const estado &st, const list<Action> &plan);
    ComportamientoJugador * clone(){return new ComportamientoJugador(*this);}

  private:
    estado actual;
    estado objetivo_actual;
    list<estado> recargas;
    list<estado> obj;
    list<Action> plan;
    vector< vector< unsigned char> > oldMapaResultado;
    bool hayPlan;
    bool yendo_a_recargar;
    bool ir_con_anchura;
    int instante;

    bool pathFinding(int level, const estado &origen, const list<estado> &destino, list<Action> &plan);
    bool pathFinding_Profundidad(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_Anchura(const estado &origen, const estado &destino, list<Action> &plan);
    bool pathFinding_AEstrella(const estado &origen, const estado &destino, list<Action> &plan, int level);
    bool pathFinding_AEstrella3(const estado &origen, const list<estado> &destinos, list<Action> &plan);

    void PintaPlan(list<Action> plan);
    void ActualizarObjs(estado &s, list<estado>::const_iterator it);
    void actualizarBikZap(estado & s);
    void calcularObjetivos(Sensores sensores);
    void actualizarMapa(Sensores sensores);
    bool esObstaculo(estado &st);
    bool enRango(const estado & st);
    bool recargaMasCercana(const estado& actual, estado& recarga, list<estado>::const_iterator it);
    int coste_bateria(estado est, bool zap, bool bik, Action accion);
    int coste_bateriaN4(estado est, bool zap, bool bik, Action accion);
    estado generarSucesor(const estado &padre, Action value, int level);
    estado objetivoMasCercanoNoConseguido(const estado &actual, list<estado>::const_iterator it);
    estado EstadoForward(const estado&st);
};
#endif
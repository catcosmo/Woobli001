#include "Arduino.h"

class Gegner
{
  public:
    void Spawn(int position, int richtung, int speed, int schleife, int groesse);
    void Tick();
    void Kill();
    bool Aktiv();
    int _position;
    int _schleife;    //größe einer Schleife in der sich der Gegner hin und her bewegt - 0 = Gegner bleibt Punkt
    int spielerSeite; //auf welcher Seite vom Gegner befindet sich der Spieler
    int _groesse;
    bool _gross = false;
  private:
    int _richtung;
    int _speed;
    int _aktiv;
    int _startPos;
};

void Gegner::Spawn(int position, int richtung, int speed, int schleife, int groesse){
    _position = position;
    _richtung = richtung;               // 0 = unten, 1 = oben
    _schleife = schleife;               // 0 = keine bewegung, >0 = yes, value is width of schleife
    _startPos = position;
    _speed = speed;
    _aktiv = 1;
    _groesse = groesse;
}

void Gegner::Tick(){
    if(_aktiv){
        if(_schleife > 0){
            _position = _startPos + (sin((millis()/3000.0)*_speed)*_schleife);
        }else{
            if(_richtung == 0){
                _position -= _speed;
            }else{
                _position += _speed;
            }
            if(_position > 1000) {
                Kill();
            }
            if(_position <= 0) {
                Kill();
            }
        }
    }
}

bool Gegner::Aktiv(){
    return _aktiv;
}

void Gegner::Kill(){
    _aktiv = 0;
}

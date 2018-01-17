#include "Arduino.h"

class Spawner
{
  public:
    void Spawn(int position, int abstand, int speed, int richtung, long aktivieren);
    void Kill();
    int Aktiv();
    int _position;
    int _abstand;
    int _speed;
    int _richtung;
    long _letzterSpawn; //wann zuletzt gespawnt
    long _aktivieren;
  private:
    int _aktiv;
};

void Spawner::Spawn(int position, int abstand, int speed, int richtung, long aktivieren){
    _position = position;
    _abstand = abstand;
    _speed = speed;
    _richtung = richtung;
    _aktivieren = millis()+aktivieren;
    _aktiv = 1;
}

void Spawner::Kill(){
    _aktiv = 0;
    _letzterSpawn = 0;
}

int Spawner::Aktiv(){
    return _aktiv;
}

#include "Arduino.h"

class Endgegner
{
  public:
    void Spawn();
    void Hit();
    void Kill();
    bool Aktiv();
    int _position;
    int _leben;
    int _ticks;
  private:
    bool _aktiv;
};

void Endgegner::Spawn(){
    _position = 800;
    _leben = 3;
    _aktiv = 1;
}

void Endgegner::Hit(){
    _leben --;
    if(_leben == 0) {
        Kill();
        return;
    }
    if(_leben == 2){
        _position = 200;
    }else if(_leben == 1){
        _position = 600;
    }
}

bool Endgegner::Aktiv(){
    return _aktiv;
}

void Endgegner::Kill(){
    _aktiv = 0;
}

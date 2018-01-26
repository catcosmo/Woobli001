#include "Arduino.h"

class Feuer
{
  public:
    void Spawn(int left, int right, int ontime, int offtime, int offset, char* state);
    void Kill();
    int Aktiv();
    int _left;
    int _right;
    int _ontime;
    int _offtime;
    int _offset;
    long _lastOn;
    char* _state;
  private:
    int _aktiv;
};

void Feuer::Spawn(int left, int right, int ontime, int offtime, int offset, char* state){
    _left = left;
    _right = right;
    _ontime = ontime;
    _offtime = offtime;
    _offset = offset;
    _aktiv = 1;
    _lastOn = millis()-offset;
    _state = state;
}

void Feuer::Kill(){
    _aktiv = 0;
}

int Feuer::Aktiv(){
    return _aktiv;
}

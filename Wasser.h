#include "Arduino.h"

class Wasser
{
  public:
      void Spawn(int anfangsPixel, int endPixel, int richtung);
      void Kill();
      int _anfangsPixel;
      int _endPixel;
      int _richtung;
      bool _aktiv;
};

void Wasser::Spawn(int anfangsPixel, int endPixel, int richtung){
    _anfangsPixel = anfangsPixel;
    _endPixel = endPixel;
    _richtung = richtung;
    _aktiv = true;
}

void Wasser::Kill(){
    _aktiv = false;
}

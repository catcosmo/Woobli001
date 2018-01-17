#include "Arduino.h"
#define FRICTION 1

class Todeskonfetti
{
  public:
    void Spawn(int position);
    void Tick(int SCHWERKRAFT);
    void Kill();
    bool Aktiv();
    int _position;
    int _power;
  private:
    int _life;
    int _aktiv;
    int _speed;
};

void Todeskonfetti::Spawn(int position){
    _position = position;
    _speed = random(-200, 200);
    _power = 255;
    _aktiv = 1;
    _life = 220 - abs(_speed);
}

void Todeskonfetti::Tick(int SCHWERKRAFT){
    if(_aktiv){
        _life ++;
        if(_speed > 0){
            _speed -= _life/10;
        }else{
            _speed += _life/10;
        }
        if(SCHWERKRAFT && _position > 500) _speed -= 10;
        _power = 100 - _life;
        if(_power <= 0){
            Kill();
        }else{
            _position += _speed/7.0;
            if(_position > 1000){
                _position = 1000;
                _speed = 0-(_speed/2);
            }
            else if(_position < 0){
                _position = 0;
                _speed = 0-(_speed/2);
            }
        }
    }
}

bool Todeskonfetti::Aktiv(){
    return _aktiv;
}

void Todeskonfetti::Kill(){
    _aktiv = 0;
}

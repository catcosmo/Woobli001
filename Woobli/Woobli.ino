// Libs
#include "FastLED.h"
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "toneAC.h"
#include "iSin.h"
#include "RunningMedian.h"

// Eigene Klassen
#include "Gegner.h"
#include "Todeskonfetti.h"
#include "Spawn.h"
#include "Feuer.h"
#include "Endgegner.h"
#include "Wasser.h"

// MPU6050 = Geschwindigkeitssensor im Joystick
MPU6050 mpu6050;
int16_t ax, ay, az;
int16_t gx, gy, gz;

// LED setup
#define LED_NUM             100    // Anzahl LEDs
#define DATA_PIN             6     // Daten Pin für den LED Schlauch am Arduino
#define HELLIGKEIT           150
#define RICHTUNG            1     // 0 = oben nach unten, 1 = unten nach oben
#define MIN_FRAME_INTERVAL  16    // Min interval bevor neuer Frame gezeichnet wird  33ms = 30fps / 16ms = 63fps
#define SCHWERKRAFT          1     // 0/1 Schwerkraft benutzen (LED Streifen geht an der Wand hoch: 1)

//Spiel & Level
long msLetzterFrame = 0;           // Zeit des letzten Frames (wann gedrawt)
int levelNummer = 0;
long lastInputTime = 0;
#define TIMEOUT              30000
#define LEVEL_ANZAHL          9
#define LAUTSTAERKE_MAX           10
iSin isin = iSin();
const int groessenAenderung = 10;
int tick = 0;

// Joystick
#define JOYSTICK_ORIENTIERUNG 1     // [0,1,2] legt die Orientierung des Joysticks fest (in welche Richtung Steuern, in welche Wackeln)
#define JOYSTICK_RICHTUNG   0      // 0/1 Bewegungsrichtung ändern (hoch/runter)
#define ANGRIFF_THRESHOLD     30000 // Ab welchem Threshold (Geschwindigkeit) des Joysticks wird ein Angriff ausgelöst
#define JOYSTICK_TOTER_WINKEL    5     // Winkel der ignoriert wird TODO
int joystickNeigung = 0;              // Variable speichert Winkel des Joysticks
int joystickWackelSpeed = 0;            // Variable speichert max. Geschwindigkeit des Joysticks

// Wackel-Angriffe
#define ANGRIFF_GROESSE        70     // Breite der Wackelattacke (Spielfeld ist 1000 weit)
#define ANGRIFF_DAUER     500       // Dauer des Angriffs
long angriffStartzeit = 0;             //Startzeit des Angriffs
bool angreifend = 0;                // während der Angriff stattfindet = 1

//Endgegner
#define EG_GROESSE          40      //Groesse des Endgegners

// Spieler
#define SPIELER_MAX_SPEED    10     // MAximale Geschwindigkeit für den Spieler (zB. im Wasser)
char* status;                       // Status des Spiels
long statusStartZeit;               // speichert die Zeit der letzten Statusänderung für zeitabhängige Stati
int spielerPosition;                // speichert Spielerposition
int spielerPositionAnpassen;        // Ausgleich der Spielerposition
bool spielerAktiv;
long todeszeitpunkt;
int leben = 3;

// Gangs
int lebensAnzeigeLEDs[3] = {8, 9, 10};
Gegner gegnerGang[10] = {
    Gegner(), Gegner(), Gegner(), Gegner(), Gegner(), Gegner(), Gegner(), Gegner(), Gegner(), Gegner()
};
int const gegnerAnzahl = 10;
Todeskonfetti todeskonfettiGang[40] = {
    Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti(), Todeskonfetti()
};
int const todeskonfettiAnzahl = 40;
Spawner spawnGang[2] = {
    Spawner(), Spawner()
};
int const spawnAnzahl = 2;
Feuer feuerGang[4] = {
    Feuer(), Feuer(), Feuer(), Feuer()
};
int const feuerAnzahl = 4;
Wasser WasserGang[2] = {
    Wasser(), Wasser()
};
int const WasserAnzahl = 2;
Endgegner endgegner = Endgegner();

CRGB leds[LED_NUM];
RunningMedian MPUAngleSamples = RunningMedian(5);
RunningMedian MPUWobbleSamples = RunningMedian(5);

void setup() {
    Serial.begin(9600);
    while (!Serial);

    // MPU
    Wire.begin();
    mpu6050.initialize();

    // Fast LED
    FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, LED_NUM);
    FastLED.setBrightness(HELLIGKEIT);
    FastLED.setDither(1);

    // Life LEDs
    for(int i = 0; i<=3; i++){
        pinMode(lebensAnzeigeLEDs[i], OUTPUT);
        digitalWrite(lebensAnzeigeLEDs[i], HIGH);
    }

    loadLevel();
}

void loop() {
    long mm = millis();
    int brightness = 0;
    tick++;

    if(status == "PLAY"){
        if(angreifend){
            SFXangreifend();
        }else{
            SFXtilt(joystickNeigung);
        }
    }else if(status == "DEAD"){
        SFXdead();
    }

    if (mm - msLetzterFrame >= MIN_FRAME_INTERVAL) {
        getInput();
        long frameTimer = mm;
        msLetzterFrame = mm;

        if(abs(joystickNeigung) > JOYSTICK_TOTER_WINKEL){
            lastInputTime = mm;
            if(status == "SCREENSAVER"){
                levelNummer = -1;
                statusStartZeit = mm;
                status = "WIN";
            }
        }else{
            if(lastInputTime+TIMEOUT < mm){
                status = "SCREENSAVER";
            }
        }
        if(status == "SCREENSAVER"){
            screenSaverTick();
        }else if(status == "PLAY"){
            // PLAYING
            if(angreifend && angriffStartzeit+ANGRIFF_DAUER < mm) angreifend = 0;

            // If not angreifend, check if they should be
            if(!angreifend && joystickWackelSpeed > ANGRIFF_THRESHOLD){
                angriffStartzeit = mm;
                angreifend = 1;
            }

            // If still not angreifend, move!
            spielerPosition += spielerPositionAnpassen;
            if(!angreifend){
                int moveAmount = (joystickNeigung/6.0);
                if(RICHTUNG) moveAmount = -moveAmount;
                moveAmount = constrain(moveAmount, -SPIELER_MAX_SPEED, SPIELER_MAX_SPEED);
                spielerPosition -= moveAmount;
                if(spielerPosition < 0) spielerPosition = 0;
                if(spielerPosition >= 1000 && !endgegner.Aktiv()) {
                    // Reached exit!
                    levelComplete();
                    return;
                }
            }

            if(inFeuer(spielerPosition)){
                die();
            }

            // Ticks and draw calls
            FastLED.clear();
            tickWassers();
            tickSpawners();
            tickEndgegner();
            tickFeuer();
            tickGegner();
            drawSpieler();
            drawAttack();
            drawExit();
        }else if(status == "DEAD"){
            // DEAD
            FastLED.clear();
            if(!tickTodeskonfettis()){
                loadLevel();
            }
        }else if(status == "WIN"){
            // LEVEL COMPLETE
            FastLED.clear();
            if(statusStartZeit+500 > mm){
                int n = max(map(((mm-statusStartZeit)), 0, 500, LED_NUM, 0), 0);
                for(int i = LED_NUM; i>= n; i--){
                    leds[i] = CRGB::Gold;
                }
                SFXwin();
            }else if(statusStartZeit+1000 > mm){
                int n = max(map(((mm-statusStartZeit)), 500, 1000, LED_NUM, 0), 0);
                for(int i = 0; i< n; i++){
                    brightness = 255;
                    leds[i] = CRGB::Gold;
                }
                SFXwin();
            }else if(statusStartZeit+1200 > mm){
                leds[0] = CRGB(0, 255, 0);
            }else{
                nextLevel();
            }
        }else if(status == "COMPLETE"){
            FastLED.clear();
            SFXcomplete();
            if(statusStartZeit+500 > mm){
                int n = max(map(((mm-statusStartZeit)), 0, 500, LED_NUM, 0), 0);
                for(int i = LED_NUM; i>= n; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(statusStartZeit+5000 > mm){
                for(int i = LED_NUM; i>= 0; i--){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else if(statusStartZeit+5500 > mm){
                int n = max(map(((mm-statusStartZeit)), 5000, 5500, LED_NUM, 0), 0);
                for(int i = 0; i< n; i++){
                    brightness = (sin(((i*10)+mm)/500.0)+1)*255;
                    leds[i].setHSV(brightness, 255, 50);
                }
            }else{
                nextLevel();
            }
        }else if(status == "GAMEOVER"){
            // GAME OVER!
            FastLED.clear();
            statusStartZeit = 0;
        }

        Serial.print(millis()-mm);
        Serial.print(" - ");
        FastLED.show();
        Serial.println(millis()-mm);
    }
}


// ---------------------------------
// ------------ LEVELS -------------
// ---------------------------------
void loadLevel(){
    updateLeben();
    cleanupLevel();
    spielerPosition = 0;
    spielerAktiv = 1;
    switch(levelNummer){
        case 0:
            // Left or right?
            spielerPosition = 200;
            spawnGegner(400, 0, 0, 0);
            spawnWasser(450, 600, -1);
            break;
        case 1:
            // Slow moving gegner
            spawnGegner(900, 0, 1, 0);
            break;
        case 2:
            // Spawning enemies at exit every 2 seconds
            spawnGang[0].Spawn(1000, 3000, 2, 0, 0);
            break;
        case 3:
            // Feuer intro
            spawnFeuer(400, 490, 2000, 2000, 0, "OFF");
            spawnGang[0].Spawn(1000, 5500, 3, 0, 0);
            break;
        case 4:
            // Sin gegner
            spawnGegner(700, 1, 7, 275);
            spawnGegner(500, 1, 5, 250);
            break;
        case 5:
            // Wasser
            spawnWasser(100, 600, -1);
            spawnGegner(800, 0, 0, 0);
            break;
        case 6:
            // Wasser of enemies
            spawnWasser(50, 1000, 1);
            spawnGegner(300, 0, 0, 0);
            spawnGegner(400, 0, 0, 0);
            spawnGegner(500, 0, 0, 0);
            spawnGegner(600, 0, 0, 0);
            spawnGegner(700, 0, 0, 0);
            spawnGegner(800, 0, 0, 0);
            spawnGegner(900, 0, 0, 0);
            break;
        case 7:
            // Feuer run
            spawnFeuer(195, 300, 2000, 2000, 0, "OFF");
            spawnFeuer(350, 455, 2000, 2000, 0, "OFF");
            spawnFeuer(510, 610, 2000, 2000, 0, "OFF");
            spawnFeuer(660, 760, 2000, 2000, 0, "OFF");
            spawnGang[0].Spawn(1000, 3800, 4, 0, 0);
            break;
        case 8:
            // Sin gegner #2
            spawnGegner(700, 1, 7, 275);
            spawnGegner(500, 1, 5, 250);
            spawnGang[0].Spawn(1000, 5500, 4, 0, 3000);
            spawnGang[1].Spawn(0, 5500, 5, 1, 10000);
            spawnWasser(100, 900, -1);
            break;
        case 9:
            // Endgegner
            spawnEndgegner();
            break;
    }
    statusStartZeit = millis();
    status = "PLAY";
}

void spawnEndgegner(){
    endgegner.Spawn();
    moveEndgegner();
}

void moveEndgegner(){
    int spawnSpeed = 2500;
    if(endgegner._leben == 2) spawnSpeed = 2000;
    if(endgegner._leben == 1) spawnSpeed = 1500;
    spawnGang[0].Spawn(endgegner._position, spawnSpeed, 3, 0, 0);
    spawnGang[1].Spawn(endgegner._position, spawnSpeed, 3, 1, 0);
}

void spawnGegner(int position, int richtung, int speed, int schleife){
    for(int e = 0; e<gegnerAnzahl; e++){
        if(!gegnerGang[e].Aktiv()){
            gegnerGang[e].Spawn(position, richtung, speed, schleife, 1);
            gegnerGang[e].spielerSeite = position > spielerPosition?1:-1;
            return;
        }
    }
}

void spawnFeuer(int left, int right, int ontime, int offtime, int offset, char* state){
    for(int i = 0; i<feuerAnzahl; i++){
        if(!feuerGang[i].Aktiv()){
            feuerGang[i].Spawn(left, right, ontime, offtime, offset, state);
            return;
        }
    }
}

void spawnWasser(int anfangsPixel, int endPixel, int richtung){
    for(int i = 0; i<WasserAnzahl; i++){
        if(!WasserGang[i]._aktiv){
            WasserGang[i].Spawn(anfangsPixel, endPixel, richtung);
            return;
        }
    }
}

void cleanupLevel(){
    for(int i = 0; i<gegnerAnzahl; i++){
        gegnerGang[i].Kill();
    }
    for(int i = 0; i<todeskonfettiAnzahl; i++){
        todeskonfettiGang[i].Kill();
    }
    for(int i = 0; i<spawnAnzahl; i++){
        spawnGang[i].Kill();
    }
    for(int i = 0; i<feuerAnzahl; i++){
        feuerGang[i].Kill();
    }
    for(int i = 0; i<WasserAnzahl; i++){
        WasserGang[i].Kill();
    }
    endgegner.Kill();
}

void levelComplete(){
    statusStartZeit = millis();
    status = "WIN";
    if(levelNummer == LEVEL_ANZAHL) status = "COMPLETE";
    leben = 3;
    updateLeben();
}

void nextLevel(){
    levelNummer ++;
    if(levelNummer > LEVEL_ANZAHL) levelNummer = 0;
    loadLevel();
}

void gameOver(){
    levelNummer = 0;
    loadLevel();
}

void die(){
    spielerAktiv = 0;
    if(levelNummer > 0) leben --;
    updateLeben();
    if(leben == 0){
        levelNummer = 0;
        leben = 3;
    }
    for(int p = 0; p < todeskonfettiAnzahl; p++){
        todeskonfettiGang[p].Spawn(spielerPosition);
    }
    statusStartZeit = millis();
    status = "DEAD";
    todeszeitpunkt = millis();
}

// ----------------------------------
// -------- TICKS & RENDERS ---------
// ----------------------------------
void tickGegner(){
    for(int i = 0; i<gegnerAnzahl; i++){
        if(gegnerGang[i].Aktiv()){
            gegnerGang[i].Tick();
            // Hit attack?
            if(angreifend){
                if(gegnerGang[i]._position > spielerPosition-(ANGRIFF_GROESSE/2) && gegnerGang[i]._position < spielerPosition+(ANGRIFF_GROESSE/2)){
                   gegnerGang[i].Kill();
                   SFXkill();
                }
            }
            if(inFeuer(gegnerGang[i]._position)){
                gegnerGang[i].Kill();
                SFXkill();
            }
            // Draw (if still aktiv)
            if(gegnerGang[i].Aktiv()) {
                leds[getLED(gegnerGang[i]._position)] = CRGB(0, 255, 0);
                if((tick / 10000 ) % 2 == 0) {
                    for(int j = 0; j <= gegnerGang[i]._groesse;j++) {
                        leds[getLED(gegnerGang[i]._position+j*10)] = CRGB(0, 255, 0);
                        leds[getLED(gegnerGang[i]._position-j*10)] = CRGB(0, 255, 0);
                        gegnerGang[i]._gross = true;
                    }
                } else
                    gegnerGang[i]._gross = false;
                    Serial.println(gegnerGang[i]._position);

            }
            // Hit spieler?
            if(
                ((gegnerGang[i].spielerSeite == 1 && gegnerGang[i]._position <= spielerPosition) ||
                (gegnerGang[i].spielerSeite == -1 && gegnerGang[i]._position >= spielerPosition)) || (gegnerGang[i]._gross == true &&((gegnerGang[i].spielerSeite == 1 && gegnerGang[i]._position <= spielerPosition-1) ||
                (gegnerGang[i].spielerSeite == -1 && gegnerGang[i]._position >= spielerPosition+1)))
            ){
                die();
                return;
            }
        }
    }
}

void tickEndgegner(){
    // DRAW
    if(endgegner.Aktiv()){
        endgegner._ticks ++;
        for(int i = getLED(endgegner._position-EG_GROESSE/2); i<=getLED(endgegner._position+EG_GROESSE/2); i++){
            leds[i] = CRGB::DarkRed;
            leds[i] %= 100;
        }
        // CHECK COLLISION
        if(getLED(spielerPosition) > getLED(endgegner._position - EG_GROESSE/2) && getLED(spielerPosition) < getLED(endgegner._position + EG_GROESSE)){
            die();
            return;
        }
        // CHECK FOR ATTACK
        if(angreifend){
            if(
              (getLED(spielerPosition+(ANGRIFF_GROESSE/2)) >= getLED(endgegner._position - EG_GROESSE/2) && getLED(spielerPosition+(ANGRIFF_GROESSE/2)) <= getLED(endgegner._position + EG_GROESSE/2)) ||
              (getLED(spielerPosition-(ANGRIFF_GROESSE/2)) <= getLED(endgegner._position + EG_GROESSE/2) && getLED(spielerPosition-(ANGRIFF_GROESSE/2)) >= getLED(endgegner._position - EG_GROESSE/2))
            ){
               endgegner.Hit();
               if(endgegner.Aktiv()){
                   moveEndgegner();
               }else{
                   spawnGang[0].Kill();
                   spawnGang[1].Kill();
               }
            }
        }
    }
}

void drawSpieler(){
    leds[getLED(spielerPosition)] = CRGB::Gold;
}

void drawExit(){
    if(!endgegner.Aktiv()){
        leds[LED_NUM-1] = CRGB(255,20,147);
    }
}

void tickSpawners(){
    long mm = millis();
    for(int s = 0; s<spawnAnzahl; s++){
        if(spawnGang[s].Aktiv() && spawnGang[s]._aktivieren < mm){
            if(spawnGang[s]._letzterSpawn + spawnGang[s]._abstand < mm || spawnGang[s]._letzterSpawn == 0){
                spawnGegner(spawnGang[s]._position, spawnGang[s]._richtung, spawnGang[s]._speed, 0);
                spawnGang[s]._letzterSpawn = mm;
            }
        }
    }
}

void tickFeuer(){
    int A, B, p, i, brightness, flicker;
    long mm = millis();
    Feuer LP;
    for(i = 0; i<feuerAnzahl; i++){
        flicker = random8(5);
        LP = feuerGang[i];
        if(LP.Aktiv()){
            A = getLED(LP._left);
            B = getLED(LP._right);
            if(LP._state == "OFF"){
                if(LP._lastOn + LP._offtime < mm){
                    LP._state = "ON";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(3+flicker, (3+flicker)/1.5, 0);
                }
            }else if(LP._state == "ON"){
                if(LP._lastOn + LP._ontime < mm){
                    LP._state = "OFF";
                    LP._lastOn = mm;
                }
                for(p = A; p<= B; p++){
                    leds[p] = CRGB(150+flicker, 100+flicker, 0);
                }
            }
        }
        feuerGang[i] = LP;
    }
}

bool tickTodeskonfettis(){
    bool stillActive = false;
    for(int p = 0; p < todeskonfettiAnzahl; p++){
        if(todeskonfettiGang[p].Aktiv()){
            todeskonfettiGang[p].Tick(SCHWERKRAFT);
            leds[getLED(todeskonfettiGang[p]._position)] += CRGB(todeskonfettiGang[p]._power, 0, 0);
            stillActive = true;
        }
    }
    return stillActive;
}

void tickWassers(){
    int b, richtung, n, i, ss, ee, led;
    long m = 10000+millis();
    spielerPositionAnpassen = 0;

    for(i = 0; i<WasserAnzahl; i++){
        if(WasserGang[i]._aktiv){
            richtung = WasserGang[i]._richtung;
            ss = getLED(WasserGang[i]._anfangsPixel);
            ee = getLED(WasserGang[i]._endPixel);
            for(led = ss; led<ee; led++){
                b = 5;
                n = (-led + (m/100)) % 5;
                if(richtung == -1) n = (led + (m/100)) % 5;
                b = (5-n)/2.0;
                if(b > 0) leds[led] = CRGB(0, 0, 64);
            }

            if(spielerPosition > WasserGang[i]._anfangsPixel && spielerPosition < WasserGang[i]._endPixel){
                if(richtung == -1){
                    spielerPositionAnpassen = -(SPIELER_MAX_SPEED-4);
                }else{
                    spielerPositionAnpassen = (SPIELER_MAX_SPEED-4);
                }
            }
        }
    }
}

void drawAttack(){
    if(!angreifend) return;
    int n = map(millis() - angriffStartzeit, 0, ANGRIFF_DAUER, 100, 5);
    for(int i = getLED(spielerPosition-(ANGRIFF_GROESSE/2))+1; i<=getLED(spielerPosition+(ANGRIFF_GROESSE/2))-1; i++){
        leds[i] = CRGB(0, 0, n);
    }
    if(n > 90) {
        n = 255;
        leds[getLED(spielerPosition)] = CRGB(255, 255, 255);
    }else{
        n = 0;
        leds[getLED(spielerPosition)] = CRGB::Gold;
    }
    leds[getLED(spielerPosition-(ANGRIFF_GROESSE/2))] = CRGB(n, n, 255);
    leds[getLED(spielerPosition+(ANGRIFF_GROESSE/2))] = CRGB(n, n, 255);
}

int getLED(int position){
    // The world is 1000 pixels wide, this converts world units into an LED number
    return constrain((int)map(position, 0, 1000, 0, LED_NUM-1), 0, LED_NUM-1);
}

bool inFeuer(int position){
    // Returns if the spieler is in active feuer
    int i;
    Feuer LP;
    for(i = 0; i<feuerAnzahl; i++){
        LP = feuerGang[i];
        if(LP.Aktiv() && LP._state == "ON"){
            if(LP._left < position && LP._right > position) return true;
        }
    }
    return false;
}

void updateLeben(){
    // Updates the life LEDs to show how many leben the spieler has left
    for(int i = 0; i<3; i++){
       digitalWrite(lebensAnzeigeLEDs[i], leben>i?HIGH:LOW);
    }
}


// ---------------------------------
// --------- BILDSCHIRMSCHONER -----
// ---------------------------------
void screenSaverTick(){
    int n, b, c, i;
    long mm = millis();
    int mode = (mm/20000)%2;

    for(i = 0; i<LED_NUM; i++){
        leds[i].nscale8(250);
    }
    if(mode == 0){
        // Marching green <> orange
        n = (mm/250)%10;
        b = 10+((sin(mm/500.00)+1)*20.00);
        c = 20+((sin(mm/5000.00)+1)*33);
        for(i = 0; i<LED_NUM; i++){
            if(i%10 == n){
                leds[i] = CHSV( c, 255, 150);
            }
        }
    }else if(mode == 1){
        // Random flashes
        randomSeed(mm);
        for(i = 0; i<LED_NUM; i++){
            if(random8(200) == 0){
                leds[i] = CHSV( 25, 255, 100);
            }
        }
    }
}

// ---------------------------------
// ----------- JOYSTICK ------------
// ---------------------------------
void getInput(){
    // This is respeedonsible for the spieler movement speedeed and angreifend.
    // You can replace it with anything you want that passes a -90>+90 value to joystickNeigung
    // and any value to joystickWackelSpeed that is greater than ANGRIFF_THRESHOLD (defined at start)
    // For example you could use 3 momentery buttons:
        // if(digitalRead(leftButtonPinNumber) == HIGH) joystickNeigung = -90;
        // if(digitalRead(rightButtonPinNumber) == HIGH) joystickNeigung = 90;
        // if(digitalRead(attackButtonPinNumber) == HIGH) joystickWackelSpeed = ANGRIFF_THRESHOLD;

    mpu6050.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);
    int a = (JOYSTICK_ORIENTIERUNG == 0?ax:(JOYSTICK_ORIENTIERUNG == 1?ay:az))/166;
    int g = (JOYSTICK_ORIENTIERUNG == 0?gx:(JOYSTICK_ORIENTIERUNG == 1?gy:gz));
    if(abs(a) < JOYSTICK_TOTER_WINKEL) a = 0;
    if(a > 0) a -= JOYSTICK_TOTER_WINKEL;
    if(a < 0) a += JOYSTICK_TOTER_WINKEL;
    MPUAngleSamples.add(a);
    MPUWobbleSamples.add(g);

    joystickNeigung = MPUAngleSamples.getMedian();
    if(JOYSTICK_RICHTUNG == 1) {
        joystickNeigung = 0-joystickNeigung;
    }
    joystickWackelSpeed = abs(MPUWobbleSamples.getHighest());
}


// ---------------------------------
// -------------- TON --------------
// ---------------------------------
void SFXtilt(int amount){
    int f = map(abs(amount), 0, 90, 80, 900)+random8(100);
    if(spielerPositionAnpassen < 0) f -= 500;
    if(spielerPositionAnpassen > 0) f += 200;
    toneAC(f, min(min(abs(amount)/9, 5), LAUTSTAERKE_MAX));

}
void SFXangreifend(){
    int freq = map(sin(millis()/2.0)*1000.0, -1000, 1000, 500, 600);
    if(random8(5)== 0){
      freq *= 3;
    }
    toneAC(freq, LAUTSTAERKE_MAX);
}
void SFXdead(){
    int freq = max(1000 - (millis()-todeszeitpunkt), 10);
    freq += random8(200);
    int vol = max(10 - (millis()-todeszeitpunkt)/200, 0);
    toneAC(freq, LAUTSTAERKE_MAX);
}
void SFXkill(){
    toneAC(2000, LAUTSTAERKE_MAX, 1000, true);
}
void SFXwin(){
    int freq = (millis()-statusStartZeit)/3.0;
    freq += map(sin(millis()/20.0)*1000.0, -1000, 1000, 0, 20);
    int vol = 10;//max(10 - (millis()-statusStartZeit)/200, 0);
    toneAC(freq, LAUTSTAERKE_MAX);
}

void SFXcomplete(){
    noToneAC();
}

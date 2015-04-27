#include "AT91SAM9263.h"
#include <stdlib.h>
#include <stdio.h>
 
// I/O line is controlled by the PIO controller a nie peryferiow
#define PIOB_PER ((volatile unsigned int * const) 0xFFFFF400)
#define PIOC_PER ((volatile unsigned int * const) 0xFFFFF600)
// Ustawienie odpowiedniego kierunku portu IO, jako wyjscie
#define PIOB_OER ((volatile unsigned int * const) 0xFFFFF410)
#define PIOC_OER ((volatile unsigned int * const) 0xFFFFF610)
// Wymuszenie odpowiedniego stanu na wyjsciu portu, 0 dla diody
#define PIOB_CODR ((volatile unsigned int * const) 0xFFFFF434) //PIOB_CODR
#define PIOC_CODR ((volatile unsigned int * const) 0xFFFFF634) //PIOC_CODR
// zgas - 1 dla diody
#define PIOB_SODR ((volatile unsigned int * const) 0xFFFFF430) //PIOB_SODR
#define PIOC_SODR ((volatile unsigned int * const) 0xFFFFF630) //PIOC_SODR
//peripheral clock controller
#define CLOCK_ENABLE ((volatile unsigned int * const) 0xFFFFFC10) //PMC_PCER
//pio pull up enable register
#define PULL_UP_C_ENABLE ((volatile unsigned int * const) 0xFFFFF664) //PIOC_PUER
#define PULL_UP_B_ENABLE ((volatile unsigned int * const) 0xFFFFF464) //PIOB_PUER
//pio enable register
//#define PIOC_PER ((volatile unsigned int * const) 0xFFFFF600)
//output disable register
#define PIOC_ODR ((volatile unsigned int * const) 0xFFFFF614)
#define PIOB_ODR ((volatile unsigned int * const) 0xFFFFF414)
//pio controller pin data status register
#define ODCZYT_PORT_C ((volatile unsigned int * const) 0xFFFFF63C) //PIOC_PDSR
#define ODCZYT_PORT_B ((volatile unsigned int * const) 0xFFFFF43C) // PIOB_PDSR
//output data status register
#define STAN_PORTU_C ((volatile unsigned int * const) 0xFFFFF638) //PIOC_ODSR
#define STAN_PORTU_B ((volatile unsigned int * const) 0xFFFFF438) //PIOB_ODSR


#define AT91B_MAIN_OSC        16367660              // Main Oscillator MAINCK
#define AT91B_MCK             ((16367660*110/9)/2)   // Output PLLA Clock / 2 (~100 MHz)
#define AT91B_MASTER_CLOCK    AT91B_MCK
#define AT91B_SLOW_CLOCK      32768

//licznik
//tu sie ustawia overflow
#define PIT_MR ((volatile unsigned int * const) 0xFFFFFD30)
#define PIT_PIVR ((volatile unsigned int * const) 0xFFFFFD38)
#define PIT_SR ((volatile unsigned int * const) 0xFFFFFD34)
#define MASKA_PICNT ((volatile unsigned int * const) 0xFFF00000)

#define Q1 1<<31    //caly wyswietlacz
#define Q2 1<<28    //pierwsza cyfra
#define Q3 1<<30    //druga cyfra

#define A 1<<25
#define B 1<<24
#define C 1<<22
#define D 1<<21
#define E 1<<20
#define F 1<<27
#define G 1<<26
#define DOT 1<<23

//SW prawy PC4 i lewy PC5
// LED lewa PB8 i prawa PC29
// encoder PB17
// buzzer PB29

#define SW_LEFT 1<<5
#define SW_RIGHT 1<<4
#define SW_ENCODER 1<<17
#define LED_LEFT 1<<8
#define LED_RIGHT 1<<29
#define BUZZER 1<<29

//zmienne globalne
unsigned long czas = 0, czas_buzzer1 = 0, czas_buzzer2 = 0, czas_seg = 0, game_delay = 0, start = 0, stop = 0;
unsigned long czas_gry[100] = { 0 }; //zerowanie tablicy
unsigned int flaga_buzzer = 0, flaga_odblokuj_przycisk = 0, ktora_runda = 0,rundy=1,podsumowanie;

char buffer [100];

  

void Open_DBGU(){
    AT91C_BASE_DBGU->DBGU_IDR = 0xFFFFFFFF;
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_RSTRX | AT91C_US_RXDIS;
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_RSTTX  | AT91C_US_TXDIS;
    AT91C_BASE_PIOC->PIO_ASR = AT91C_PC30_DRXD | AT91C_PC31_DTXD;
    AT91C_BASE_PIOC->PIO_PDR = AT91C_PC30_DRXD | AT91C_PC31_DTXD;
    AT91C_BASE_DBGU->DBGU_BRGR = 52;
    AT91C_BASE_DBGU->DBGU_MR = AT91C_US_CHMODE_NORMAL | AT91C_US_PAR_NONE;
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_RXEN;
    AT91C_BASE_DBGU->DBGU_CR = AT91C_US_TXEN;
}
void dbgu_print_ascii(const char ch){ 
    while( !( AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_TXRDY ) ); 
    AT91C_BASE_DBGU->DBGU_THR = (char) ch; 
} 
void dbgu_read_ascii(char *ch) { 
    while( !( AT91C_BASE_DBGU->DBGU_CSR & AT91C_US_RXRDY ) ); 
    *ch = (char)AT91C_BASE_DBGU->DBGU_RHR; 
} 
void print( char *str ) { 
    int i = 0; 
    while( str[ i ] ) { 
        dbgu_print_ascii( str[ i ] ); 
        i++; 
    } 
}



int generuj(void){
    return (rand() % 3);
}



void seg_ustawCyfre(int cyfra){
    switch(cyfra){
        case 0: *PIOB_SODR = A|B|C|D|E|F; break;
        case 1: *PIOB_SODR = B|C;      break;
        case 2: *PIOB_SODR = A|B|G|E|D;   break;
        case 3: *PIOB_SODR = A|B|G|D|C; break;
        case 4: *PIOB_SODR = B|C|G|F; break;
        case 5: *PIOB_SODR = A|G|C|D|F; break;
        case 6: *PIOB_SODR = A|G|C|D|E|F; break;
        case 7: *PIOB_SODR = A|B|C; break;
        case 8: *PIOB_SODR = A|B|C|D|E|F|G; break;
        case 9: *PIOB_SODR = A|B|C|D|G|F; break;
        default: *PIOB_SODR = A|F|G|E|D; break;
    }
}




void seg_clear(void){
    *PIOB_CODR = A|B|C|D|E|F|G|DOT;
}



void OpenLED(){
    *PIOB_PER = LED_LEFT;
    *PIOC_PER = LED_RIGHT;

    *PIOB_OER = LED_LEFT;
    *PIOC_OER = LED_RIGHT;

    //domyslne zgaszenie diod
    *PIOB_SODR = LED_LEFT;
    *PIOC_SODR = LED_RIGHT;
}
void OpenSW(){
    *PIOB_PER = SW_ENCODER;
    *PIOC_PER = SW_LEFT | SW_RIGHT;

    // ustawienie przyciskow jako wejscia
    *PIOC_ODR = SW_LEFT | SW_RIGHT;
    *PIOB_ODR = SW_ENCODER;
}



void OpenBuzzer(){
    *PIOB_PER = BUZZER;
    *PIOB_OER = BUZZER;
}


void OpenSeg(){
    *PIOB_PER = Q1|Q2|Q3|A|B|C|D|E|F|G|DOT;
    *PIOB_OER = Q1|Q2|Q3|A|B|C|D|E|F|G|DOT;

    //ustaw tranzystor PNP wlacz Q1, wlacz Q2, wylacz Q3
    *PIOB_CODR = Q1;    //caly wyswietlacz
    *PIOB_CODR = Q2;    //pierwsza cyfra
    *PIOB_SODR = Q3;    //druga cyfra
    //Wylacz cyfry
    seg_clear();
}


void OpenClock(){
    *CLOCK_ENABLE = 1<<4 | 1<<3; // 1<<4 to taktowanie PIOC to PIOE, 1<<3 to PIOB
    //konfiguracja timera
    //PITIEN 1<<25 i PITEN 1<<24 to 0x3000000
    //0x98968 100ms
    //0x1E848 20ms
    //0x186A   1ms    
    *PIT_MR = 1<<24 | 1<<25 | 0x186A;
    srand(czas);
}


void ConfiugurePullUpRegister(){
    *PULL_UP_C_ENABLE = LED_LEFT | LED_RIGHT; //dla portu c dla przyciskow
    *PULL_UP_B_ENABLE = SW_ENCODER; //dla portu b dla encodera
}


void IncrementMainTime(){
    if(*PIT_PIVR & 0xFF00000) czas ++;
}


void ZapiszCzasGry(){
        if (!flaga_odblokuj_przycisk) 
            czas_gry[ktora_runda] = stop - start;
        else 
            czas_gry[ktora_runda] = czas - start;
}


void WyswietlSeg(){
        ZapiszCzasGry();
        if ((czas - czas_seg) >= 10) {
            if (!(*ODCZYT_PORT_B & Q2) && (*ODCZYT_PORT_B & Q3)){
                //zgas pierwszy zapal drugi
                *PIOB_SODR = Q2;
                *PIOB_CODR = Q3;
                seg_clear();
                seg_ustawCyfre((czas_gry[ktora_runda]/100)%10);    //wyswietla jednosci
                }
            else{
                //zapal pierwszy zgas drugi
                *PIOB_CODR = Q2;
                *PIOB_SODR = Q3;
                seg_clear();
                seg_ustawCyfre((czas_gry[ktora_runda]/100)/10);    //wyswietla dziesiatki
            }
            czas_seg = czas;
        }
}


void losujPrzycisk(){
    // losowanie przycisku
    switch(generuj()){
    case 0:
        flaga_buzzer = 1;
        flaga_odblokuj_przycisk = 1;
        czas_buzzer1 = czas;
        break;
    case 1:
        flaga_odblokuj_przycisk = 2;    
        *PIOB_CODR = LED_LEFT;
        break;
    case 2:
        flaga_odblokuj_przycisk = 3;
        *PIOC_CODR = LED_RIGHT;
        break;
    default:
        break;
    }
}



void Kara(){
        if (((czas - game_delay) >= 3000) && (flaga_odblokuj_przycisk > 0)) {    
            stop = czas;
            flaga_odblokuj_przycisk = 0;
            //domyslne zgaszenie diod
            *PIOB_SODR = LED_LEFT;
            *PIOC_SODR = LED_RIGHT;
        }
}



void NastepnaRunda(){
        if ((czas - game_delay) >= 3500){
            
            if(ktora_runda > 4){    
                
                podsumowanie = czas_gry[0]+czas_gry[1]+czas_gry[2]+czas_gry[3]+czas_gry[4];
                snprintf ( buffer, 100, "\n\rRunda %d skonczona z wynikiem %d", rundy, podsumowanie );
                print(buffer);

                ktora_runda = 0;
                rundy++;
            }
            ktora_runda ++;
            losujPrzycisk();
            start = czas;
            game_delay = czas;
        
        }
        Kara();
}
void ObslugaLewejDiody(){
        if (((*ODCZYT_PORT_C & SW_LEFT) == 0) && (flaga_odblokuj_przycisk == 2)) {
            stop = czas;
            flaga_odblokuj_przycisk = 0;
            *PIOB_SODR = LED_LEFT;
        }
}
void ObslugaPrawejDiody(){
        if (((*ODCZYT_PORT_C & SW_RIGHT) == 0) && (flaga_odblokuj_przycisk == 3)) {
            stop = czas;
            flaga_odblokuj_przycisk = 0;
            *PIOC_SODR = LED_RIGHT;
        }
}
void ObslugaBuzzera(){
        if (((*ODCZYT_PORT_B & SW_ENCODER) == 0) && (flaga_odblokuj_przycisk == 1)) {
            stop = czas;
            flaga_odblokuj_przycisk = 0; // zeby nie nacisnac przysku 2x w jednej grze
        }
        if ((czas - czas_buzzer1 >= 500) && (flaga_buzzer)){    //czas trwania 0,5s
            flaga_buzzer = 0;
        }
        if (((czas - czas_buzzer2) >= 3) && (flaga_buzzer)) {    //zmiana co 3ms
            if (*STAN_PORTU_B & BUZZER) *PIOB_CODR = BUZZER;
            else *PIOB_SODR = BUZZER;
            czas_buzzer2 = czas;
        }
}
int main(void) {

    OpenClock();
    ConfiugurePullUpRegister();
    OpenLED();
    OpenSW();
    OpenBuzzer();
    OpenSeg();
    Open_DBGU();
    print("\n\rStart: ");

    while(1){
        IncrementMainTime();//liczy glowny czas
        WyswietlSeg();//wyswietla czas rundy i zapisuje jego koncowa wartosc
        NastepnaRunda();//kontroluje uruchomienie nastepnej rundy, losuje sygnal, karze za wolny refleks
        
        ObslugaLewejDiody();
        ObslugaPrawejDiody();
        ObslugaBuzzera();
    }
}
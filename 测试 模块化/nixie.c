#include <REGX52.H>
#include "delay.h"

unsigned char NixieCode[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F,0x00};

void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;
    P0 = NixieCode[Num];
    Delay(1);
    P0 = 0x00;
}
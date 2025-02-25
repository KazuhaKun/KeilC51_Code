unsigned char LEDCode[] = {0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF, 0x7F};

void Nixie(unsigned char Location, unsigned char Num)
{
    P2_2 = Location & 0x01;
    P2_3 = (Location >> 1) & 0x01;
    P2_4 = (Location >> 2) & 0x01;
    P0 = NixieCode[Num];
    Delay(1);
    P0 = 0x00;
}
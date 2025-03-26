#ifndef __NIXIE_H__
#define __NIXIE_H__

void Nixie_SetBuf(unsigned char Location,unsigned char Num,unsigned char Point);
void Nixie_Scan(unsigned char Location,unsigned char Num,unsigned char Point);
void Nixie_Loop(void);

//extern unsigned char NixieCode[];

#endif

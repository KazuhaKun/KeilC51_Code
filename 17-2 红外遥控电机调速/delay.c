// //延时函数
// void Delay(unsigned int xms)		//@11.0592MHz
// {
//     unsigned char i, j;
//     while(xms)
//     {
//         i = 2;
//         j = 199;
//         do
//         {
//             while (--j);
//         } while (--i);
//         xms--;
//     }
// }

void Delay(unsigned int xms)
{
	unsigned char i, j;
	while(xms--)
	{
		i = 2;
		j = 239;
		do
		{
			while (--j);
		} while (--i);
	}
}


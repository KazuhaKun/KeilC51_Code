
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

void Delay100us()		//@11.0592MHz
{
	unsigned char i;

	i = 43;
	while (--i);
}

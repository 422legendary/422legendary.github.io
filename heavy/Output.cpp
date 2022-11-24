operator char*() const
{
	static long long b[320];
	int length = 1;
	b[319] = 0;
	for (int i = 32; i; i--)
	{
		long long D = d[i - 1];
		long long q;
		for (int j = length; j; j--)
		{
			D += b[319 - length + j] * 0x100000000; 
			q = D / 10;
			D -= q * 10;
			b[319 - length + j] = D;
			D = q;
		}
		while (D)
		{
			q = D / 10;
			D -= q * 10;
			b[319 - length++] = D;
			D = q;
		}
	}
	char* d = (char*) b;
	for (int i = 1; i <= length; i++)
		d[i - 1] = b[319 - length + i] + '0';
	d[length] = '\0';
	return d;
}

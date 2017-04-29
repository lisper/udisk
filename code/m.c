main()
{
    unsigned short buffer[32];
    int i;

    i = 0;
    buffer[i++] = 0012700;
    buffer[i++] = 0174400;
    buffer[i++] = 0000240;
    buffer[i++] = 0105710;
    buffer[i++] = 0000776;

    i = 0;
    buffer[i++] = 0012710;
    buffer[i++] = 0000014;
    buffer[i++] = 0105710;
    buffer[i++] = 0000774;

#if 0
    i = 0;
    buffer[i++] = 0156716;
    buffer[i++] = 0177432;
    buffer[i++] = 0105767;
    buffer[i++] = 0177427;
    buffer[i++] = 01402;
    buffer[i++] = 052716;
    buffer[i++] = 0100;
    buffer[i++] = 0012661;
    buffer[i++] = 0000004;
    buffer[i++] = 0016746;
    buffer[i++] = 0000020;
    buffer[i++] = 0005416;
#endif

#if 0
    i = 0;
    buffer[i++] = 0012761;
    buffer[i++] = 0000001;
    buffer[i++] = 0000004;
    buffer[i++] = 0105767;
#endif

#if 0
	i = 0;
	buffer[i++] = 0032711;
	buffer[i++] = 0100001;
	buffer[i++] = 0001775;
	buffer[i++] = 0100371;
#endif

#if 0
    i = 0;
    buffer[i++] = 0012700;
    buffer[i++] = 0174400;
    buffer[i++] = 0012760;
    buffer[i++] = 0000013;
    buffer[i++] = 0000004;
    buffer[i++] = 0012710;
    buffer[i++] = 0000004;
    buffer[i++] = 0105710;
    buffer[i++] = 0100376;
    buffer[i++] = 0005060;
    buffer[i++] = 0000002;
    buffer[i++] = 0005060;
    buffer[i++] = 0000004;
    buffer[i++] = 0012760;
    buffer[i++] = 0177400;
    buffer[i++] = 0000006;
    buffer[i++] = 0012710;
    buffer[i++] = 0000014;
    buffer[i++] = 0105710;
    buffer[i++] = 0100376;
//    buffer[i++] = 0000000;
    buffer[i++] = 0005007;
#endif

    write(1, buffer, i*2);
}

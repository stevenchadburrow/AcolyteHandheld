



// LCD-D0 = RE0
// LCD-D1 = RE1
// LCD-D2 = RE2
// LCD-D3 = RE3
// LCD-D4 = RE4
// LCD-D5 = RE5
// LCD-D6 = RE6
// LCD-D7 = RE7

// LCD-RS = RE8
// LCD-CS = RE9

// LCD-WR = RC3
// LCD-RD = RC4

volatile unsigned long lcd_column = 0;
volatile unsigned char lcd_request = 0;
volatile unsigned char lcd_ready = 0;

void __attribute__((optimize("O0"))) lcd_enable()
{
	PORTEbits.RE9 = 0; // CS low
}

void __attribute__((optimize("O0"))) lcd_disable()
{
	PORTEbits.RE9 = 1; // CS high
}

void __attribute__((optimize("O0"))) lcd_delay()
{
	unsigned long count = (unsigned long)((unsigned long)((SYS_FREQ / 5000))); // delay a little bit after changing RE8!!!
	_CP0_SET_COUNT(0);
	while (count > _CP0_GET_COUNT());
}

void __attribute__((optimize("O0"))) lcd_command(unsigned char val)
{
	// prepare for command
		
	PORTEbits.RE8 = 0;
		
	// use PMP

	while (PMMODEbits.BUSY) { }
	PMDIN = val;
	
	return;
}

void __attribute__((optimize("O0"))) lcd_data(unsigned char val)
{
	// prepare for data
		
	PORTEbits.RE8 = 1;
		
	// use PMP

	while (PMMODEbits.BUSY) { }
	PMDIN = val;
	
	return;
}

void __attribute__((optimize("O1"))) lcd_init()
{	
	lcd_column = 0;
	
	// initialize pins
	
	PORTE = 0x30;
	TRISE = 0x00;
	PORTCbits.RC3 = 0;
	PORTCbits.RC4 = 0;
	TRISCbits.TRISC3 = 0;
	TRISCbits.TRISC4 = 0;
	
	// set up PMP
	
	PMCON = 0;
	PMMODE = 0;
	PMADDR = 0;
	PMAEN = 0;
	PMSTAT = 0;
	
	PMCONbits.PMPTTL = 1; // TTL level 
	PMCONbits.PTWREN = 1; // enable WR pin
	PMCONbits.PTRDEN = 1; // enable RD pin
	PMMODEbits.MODE = 0x2; // master mode 2
	PMMODEbits.IRQM = 1; // enable interrupts
	
	PMMODEbits.WAITB = 0;
    PMMODEbits.WAITM = 0;
    PMMODEbits.WAITE = 0;
	
	PMCONbits.ON = 1; // enable PMP
	
	while (PMMODEbits.BUSY) { }
	PMDIN = 0xA5; // dummy write
	while (PMMODEbits.BUSY) { }
	PMDIN = 0x5A; // dummy write
	while (PMMODEbits.BUSY) { }
	
	// hard reset
	
	PORTAbits.RA14 = 0;
	PORTAbits.RA15 = 0;
	TRISAbits.TRISA14 = 0;
	TRISAbits.TRISA15 = 0;
	
	DelayMS(100);
	DelayMS(100);
	
	// bring out of reset
	
	PORTAbits.RA14 = 1;
	PORTAbits.RA15 = 1;
	
	DelayMS(100);
	DelayMS(100);
	
	// soft reset
	
	lcd_enable();
	lcd_command(0x01); // soft reset
	while (PMMODEbits.BUSY) { }
	lcd_disable();

	DelayMS(100); // delay
	DelayMS(100);
	
	// undocumented commands

	lcd_enable();
	lcd_command(0xEF);
	lcd_data(0x03);
	lcd_data(0x80);
	lcd_data(0x02);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xCF);
	lcd_data(0x00);
	lcd_data(0xC1);
	lcd_data(0x30);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xED);
	lcd_data(0x64);
	lcd_data(0x03);
	lcd_data(0x12);
	lcd_data(0x81);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xE8);
	lcd_data(0x85);
	lcd_data(0x00);
	lcd_data(0x78);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xCB);
	lcd_data(0x39);
	lcd_data(0x2C);
	lcd_data(0x00);
	lcd_data(0x34);
	lcd_data(0x02);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xF7);
	lcd_data(0x20);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xEA);
	lcd_data(0x00);
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	// documented commands
	
	lcd_enable();
	lcd_command(0x20); // inversion off
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x26); // gamma settings
	lcd_data(0x01);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x33); // vertical scrolling def
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0x01);
	lcd_data(0x40);
	lcd_data(0x00);
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x34); // no screen tearing
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x36); // memory access control
	lcd_data(0x40); // 0x40 or 0x48
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x3A); // pixel format
	lcd_data(0x55);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xB0); // rgb control
	lcd_data(0x40);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xB1); // frame control
	lcd_data(0x00);
	lcd_data(0x1B); // 0x18 or 0x13
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xB2); // frame control
	lcd_data(0x00);
	lcd_data(0x1B); // 0x18 or 0x13
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xB3); // frame control
	lcd_data(0x00);
	lcd_data(0x1B); // 0x18 or 0x13
	while (PMMODEbits.BUSY) { }
	lcd_disable();

	lcd_enable();
	lcd_command(0xB6); // display control
	lcd_data(0x08);
	lcd_data(0x82);
	lcd_data(0x27);
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xB7); // entry mode set
	lcd_data(0x06);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xC0); // power control
	lcd_data(0x23);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xC1); // power control
	lcd_data(0x10);
	while (PMMODEbits.BUSY) { }
	lcd_disable();

	lcd_enable();
	lcd_command(0xC5); // vcom control
	lcd_data(0x3E);
	lcd_data(0x28);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xC7); // vcom control
	lcd_data(0x86);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xE0); // gamma settings
	lcd_data(0x0F);
	lcd_data(0x31);
	lcd_data(0x2B);
	lcd_data(0x0C);
	lcd_data(0x0E);
	lcd_data(0x08);
	lcd_data(0x4E);
	lcd_data(0xF1);
	lcd_data(0x37);
	lcd_data(0x07);
	lcd_data(0x10);
	lcd_data(0x03);
	lcd_data(0x0E);
	lcd_data(0x09);
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xE1); // gamma settings
	lcd_data(0x00);
	lcd_data(0x0E);
	lcd_data(0x14);
	lcd_data(0x03);
	lcd_data(0x11);
	lcd_data(0x07);
	lcd_data(0x31);
	lcd_data(0xC1);
	lcd_data(0x48);
	lcd_data(0x08);
	lcd_data(0x0F);
	lcd_data(0x0C);
	lcd_data(0x31);
	lcd_data(0x36);
	lcd_data(0x0F);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xF2); // undocumented
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0xF6); // interface control
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0x00);
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	// now to finalize everything
	
	lcd_enable();
	lcd_command(0x11); // take out of sleep mode
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	DelayMS(100); // delay
	
	lcd_enable();
	lcd_command(0x38); // take out of idle mode
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	DelayMS(100); // delay
	
	lcd_enable();
	lcd_command(0x13); // normal mode
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	DelayMS(100); // delay
	
	lcd_enable();
	lcd_command(0x29); // turn on display
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	DelayMS(100); // delay
	
	// drawing black to full screen
	
	lcd_enable();
	lcd_command(0x2A); // y-values
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0xF0); // 240 pixels high
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x2B); // x-values
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0x01);
	lcd_data(0x3F); // 320 pixels wide
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_enable();
	lcd_command(0x2C); // draw pixels
	
	for (unsigned long i=0; i<320*240; i++)
	{
		lcd_data(0x00); // black
		lcd_data(0x00);
	}
	
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	lcd_ready = 1;
	
	return;
}

void __attribute__((optimize("01"))) lcd_draw()
{
	if (lcd_ready > 0)
	{
		lcd_ready = 0;
		
		lcd_column = 0;

		lcd_request = 1;
	}
	
	return;
}

void __attribute__((optimize("O1"))) lcd_half()
{	
	if (lcd_column > 1) return;
	
	unsigned long count;
	
	while (PMMODEbits.BUSY) { }
	lcd_disable();
	
	DCH3CONbits.CHEN = 0; // disable channel
	
	PORTEbits.RE8 = 0;
	
	lcd_delay();
	
	lcd_enable();
	lcd_command(0x2A); // y-values
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0x00);
	lcd_data(0xEF); // 240 pixels high
	while (PMMODEbits.BUSY) { }
	lcd_disable();

	lcd_enable();
	lcd_command(0x2B); // x-values
	if (lcd_column == 0)
	{
		lcd_data(0x00);
		lcd_data(0x20); // 0x00
		lcd_data(0x00); // 0x00
		lcd_data(0x9F); // 0x7F // 128 pixels wide
	}
	else
	{
		lcd_data(0x00);
		lcd_data(0xA0); // 0x80
		lcd_data(0x01); // 0x00
		lcd_data(0x2F); // 0xFF // 128 pixels wide
	}
	while (PMMODEbits.BUSY) { }
	lcd_disable();

	lcd_enable();
	lcd_command(0x2C); // draw pixels
	
	// use DMA to PMP
	
	PORTEbits.RE8 = 1;
	
	lcd_delay();
	
	IFS4bits.PMPIF = 0; // clear PMP flag
	DCH3INTbits.CHSDIF = 0; // clear source flag
	DCH3SSA = VirtToPhys(screen_buffer + SCREEN_XY*screen_frame + 480*128*lcd_column);
	DCH3CONbits.CHEN = 1; // enable channel
	
	//while (PMMODEbits.BUSY) { }
	DCH3ECONbits.CFORCE = 1; // force channel
	
	lcd_column++;

	return;
}


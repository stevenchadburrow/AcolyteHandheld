
void __attribute__((optimize("O0"))) Setup()
{
	// turn off analog
	ANSELA = 0x0;
	ANSELB = 0x0;
	ANSELC = 0x0;
	ANSELD = 0x0;
	ANSELE = 0x0;
	ANSELF = 0x0;
	ANSELG = 0x0;
	ANSELH = 0x0;
	//ANSELI = 0x0;
	ANSELJ = 0x0;
	//ANSELK = 0x0;
	
	// make everything output
	TRISA = 0x0;
	TRISB = 0x0;
	TRISC = 0x0;
	TRISD = 0x0;
	TRISE = 0x0;
	TRISF = 0x0;
	TRISG = 0x0;
	TRISH = 0x0;
	//TRISI = 0x0;
	TRISJ = 0x0;
	TRISK = 0x0;
	
	// set all ports to ground
	PORTA = 0x0;
	PORTB = 0x0;
	PORTC = 0x0;
	PORTD = 0x0;
	PORTE = 0x0;
	PORTF = 0x0;
	PORTG = 0x0;
	PORTH = 0x0;
	//PORTI = 0x0;
	PORTJ = 0x0;
	PORTK = 0x0;
	
	// disable all interrupts
	IEC0 = 0x0;
	IEC1 = 0x0;
	IEC2 = 0x0;
	IEC3 = 0x0;
	IEC4 = 0x0;
	IEC5 = 0x0;
	IEC6 = 0x0;
	//IEC7 = 0x0;
	
	// clear all flags
	IFS0 = 0x0;
	IFS1 = 0x0;
	IFS2 = 0x0;
	IFS3 = 0x0;
	IFS4 = 0x0;
	IFS5 = 0x0;
	IFS6 = 0x0;
	//IFS7 = 0x0;
	

	// Set up caching
	unsigned int cp0 = _mfc0(16, 0);
	cp0 &= ~0x07;
	cp0 |= 0b011; // K0 = Cacheable, non-coherent, write-back, write allocate
	//cp0 &= ~0x03;
	//cp0 |= 0x02; // K0 = Uncachable
	_mtc0(16, 0, cp0);  
	
	 
	// change tri-state on pins
	TRISB = 0xFF00; // buttons
	CNPUB = 0xFF00; // buttons
	TRISC = 0x4000; // usb-rx
	CNPUC = 0xC000; // usb-tx/rx
	TRISD = 0xF010; // buttons, ft232rl-rx
	CNPUD = 0xF030; // buttons, ft232rl-tx/rx
	TRISF = 0x013F; // joy-b, usbid
	CNPUF = 0x0004; // usbid
	TRISG = 0x0181; // tf-miso, spi-miso, uart-rx
	CNPUG = 0x0003; // uart-tx/rx
	TRISK = 0x00FF; // menu, joy-select, joy-a
	CNPUK = 0x0080; // menu
	
	// joy-select specifics
	PORTKbits.RK6 = 0; // ground when not floating
	TRISKbits.TRISK6 = 1; // high when floating

	// set oscillator and timers
	SYSKEY = 0x0; // reset
	SYSKEY = 0xAA996655; // unlock key #1
	SYSKEY = 0x556699AA; // unlock key #2
	
	CFGCONbits.DMAPRI = 1; // DMA does have highest priority (?)
	CFGCONbits.CPUPRI = 0; // CPU does not have highest priority (?)
	CFGCONbits.OCACLK = 1; // use alternate OC/TMR table
	
	PB1DIV = 0x00008001; // divide by 2
	PB2DIV = 0x00008005; // change PB2 clock to 260 / 6 = 43.333 MHz for SPI and UART
	PB3DIV = 0x00008000; // set OC and TMR clock division by 1
	PB4DIV = 0x00008001; // divide by 2
	PB5DIV = 0x00008001; // divide by 2
	//PB6DIV = 0x00008001; // divide by 2
	PB7DIV = 0x00008000; // CPU clock divide by 1
	SPLLCON = 0x01400201; // use PLL to bring external 24 MHz into 260 MHz
	
	// PRECON - Set up prefetch
	PRECONbits.PFMSECEN = 0; // Flash SEC Interrupt Enable (Do not generate an interrupt when the PFMSEC bit is set)
	PRECONbits.PREFEN = 0b11; // Predictive Prefetch Enable (Enable predictive prefetch for any address)
	PRECONbits.PFMWS = 0b100; // PFM Access Time Defined in Terms of SYSCLK Wait States (Four wait states)
    
	CFGCONbits.USBSSEN = 1; // USB?

	OSCCONbits.SLPEN = 0; // WAIT instruction puts CPU into idle mode
	OSCCONbits.NOSC = 0x1; // switch to SPLL
	OSCCONbits.OSWEN = 1; // enable the switch
	SYSKEY = 0x0; // re-lock
	
	while (OSCCONbits.OSWEN != 0) { } // wait for clock switch to complete

	// set PPS
	SYSKEY = 0x0; // reset
	SYSKEY = 0xAA996655; // unlock key #1
	SYSKEY = 0x556699AA; // unlock key #2
	CFGCONbits.IOLOCK = 0; // PPS is unlocked
	RPD2R = 0xB; // OC3 on pin RPD2 (h-sync)
	RPD3R = 0xC; // OC7 on pin RPD3 (v-sync)
	SDI1R = 0x1; // SDI1 on pin RPG8 (miso)
	RPD10R = 0x5; // SDO1 on pin RPD7 (mosi)
	// SCK1 on RD1 (sclk)
	// CS1 on RD9 (cs)
	U2RXR = 0x4; // U2RX on pin RPD4 (uart rx)
	RPD5R = 0x2; // U2TX on pin RPD5 (uart tx)
	CFGCONbits.IOLOCK = 1; // PPS is locked
	SYSKEY = 0x0; // re-lock
	 
	
	

	// set up UART here
	U2BRG = 0x0119; // 43.333 MHz to 9600 baud = 43333000/(16*9600)-1 = 281.12 = 0x0119
	
	U2MODEbits.STSEL = 0; // 1-Stop bit
	U2MODEbits.PDSEL = 0; // No Parity, 8-Data bits
	U2MODEbits.BRGH = 0; // Standard-Speed mode
	U2MODEbits.ABAUD = 0; // Auto-Baud disabled
	
	//U2MODEbits.URXINV = 1; // reverse polarity
	//U2STAbits.UTXINV = 1; // reverse polarity
	
	U2MODEbits.UEN = 0x0; // just use TX and RX
	
	U2STAbits.UTXISEL = 0x0; // Interrupt after one TX character is transmitted
	U2STAbits.URXISEL = 0x0; // interrupt after one RX character is received
	
	U2STAbits.URXEN = 1; // enable RX
	U2STAbits.UTXEN = 1; // enable TX
	
	IPC36bits.U2RXIP = 0x4; // U2RX interrupt priority level 4
	IPC36bits.U2RXIS = 0x0; // U2RX interrupt sub-priority level 0
	//IPC39bits.U2EIP = 0x4; // U2E interrupt priority level 4
	//IPC39bits.U2EIS = 0x1; // U2E interrupt sub-priority level 1
	IFS4bits.U2RXIF = 0;  // clear interrupt flag
	IEC4bits.U2RXIE = 1; // U2RX interrupt on (set priority here?)
	
	U2MODEbits.ON = 1; // turn UART on
	
	
	// set up interrupt-on-change for menu button
	CNCONKbits.ON = 1; // turn on interrupt-on-change
	CNCONKbits.EDGEDETECT = 1; // edge detect, not mismatch
	CNNEK = 0x0080; // negative edge on RK7
	CNFK = 0x0000; // clear flags
	
	IPC31bits.CNKIP = 0x1; // interrupt priority 1
	IPC31bits.CNKIS = 0x0; // interrupt sub-priority 0
	IFS3bits.CNKIF = 0;  // clear interrupt flag
	IEC3bits.CNKIE = 1; // enable interrupts
	
	
	// set up SPI here
	PORTDbits.RD9 = 1; // CS is high
	PORTDbits.RD10 = 1; // MOSI is high
	PORTDbits.RD1 = 0; // CLK is low	
	
	SPI1CONbits.ON = 0; // turn SPI module off
	SPI1CON2 = 0; // disable audio, etc.
	SPI1CONbits.DISSDI = 0; // SDI controlled by module
	SPI1CONbits.DISSDO = 0; // SDO controlled by module
	SPI1CONbits.MODE16 = 0x0; // 8-bit mode
	SPI1CONbits.MSTEN = 1; // host mode
	SPI1CONbits.MSSEN = 0; // client SS disabled
	SPI1CONbits.SSEN = 0; // client SS disabled
	SPI1CONbits.SMP = 0; // input sampled in middle of output time
	SPI1CONbits.CKE = 1; // output changes on idle-to-active clock
	SPI1CONbits.CKP = 0; // clock is idle low, active high
	SPI1CONbits.ON = 1; // turn SPI module on
	
	SPI1BUF = 0xFFFF; // dummy write
	while (SPI1STATbits.SPIRBF == 0) { } // wait
	sdcard_block[0] = SPI1BUF; // dummy read
	
	// set shadow register priorities???
	PRISS = 0x76543210; //0x76543210;
	
	// enable multi-vector interrupts???
	INTCONSET = _INTCON_MVEC_MASK;
	
	// IDK, just turn on interrupts globally???
	__builtin_enable_interrupts();
	
	
	// turn LED off by default
	PORTDbits.RD7 = 1;
	
	// clear usb buffers
	for (unsigned int i=0; i<256; i++)
	{
		usb_state_array[i] = 0x00;
		usb_cursor_x[i] = 0x0000;
		usb_cursor_y[i] = 0x0000;
		
	}	

	// for debug purposes
	//TRISKbits.TRISK7 = 1;
	//while (PORTKbits.RK7 == 0) { }
	
	return;
}

void __attribute__((optimize("O0"))) Display()
{
	// set OC2 and OC3 and TMR5, horizontal visible and sync clocks
	OC2CON = 0x0; // reset OC2
	OC2CON = 0x0000000D; // toggle, use Timer5
	OC2R = 0x06A0; // 0x04A0 // h-visible rise
	OC2RS = 0x16A0; // 0x14A0 // h-blank fall
	OC3CON = 0x0; // reset OC3
	OC3CON = 0x0000000D; // toggle, use Timer5
	OC3R = 0x0220; // h-sync rise
	OC3RS = 0x14FF; // h-sync fall
	T5CON = 0x0000; // reset Timer5, prescale of 1:1
	TMR5 = 0x0000; // zero out counter (offset some cycles)
	PR5 = 0x14FF; // h-reset (minus one)
	
	// set OC7 and TMR6, vertical sync clock
	OC7CON = 0x0; // reset OC7
	OC7CON = 0x0025; // toggle, use Timer6, 32-bit
	OC7R = 0x00001F80; // v-sync rise
	OC7RS = 0x00421DFF; // v-sync fall
	T7CON = 0x0000; // turn off Timer 7???
	T6CON = 0x0008; // prescale of 1:1, 32-bit
	TMR6 = 0x00000000; // zero out counter (offset some cycles)
	PR6 = 0x00421DFF; // v-reset (minus one)
	
	// TMR3, start of scanline
	T3CON = 0x0000; // reset Timer3, prescale of 1:1
	TMR3 = 0x0E60; // 0x1060 // zero out counter (offset some cycles)
	PR3 = 0x14FF; // h-reset (minus one)
	
	// TMR4, end of scanline
	T4CON = 0x0000; // rest Timer4, prescale of 1:1
	TMR4 = 0x0660; // 0x0860 // zero out counter
	PR4 = 0x14FF; // h-reset (minus one)
	
	IPC4bits.OC3IP = 0x7; // interrupt priority 7
	IPC4bits.OC3IS = 0x0; // interrupt sub-priority 0
	IFS0bits.OC3IF = 0; // OC3 clear flag
	IEC0bits.OC3IE = 1; // OC3 interrupt on (set priority here?)
	
	OC2CONbits.ON = 1; // turn OC2 on
	OC3CONbits.ON = 1; // turn OC3 on
	OC7CONbits.ON = 1; // turn OC7 on
	
	
	// timer 8 for audio
	T8CON = 0x0000; // reset
	T8CON = 0x0000; // prescale of 1:1, 16-bit
	TMR8 = 0x0000; // zero out counter
	PR8 = 0xC2FD; // approx three scanlines (minus one)

	IPC9bits.T8IP = 0x3; // interrupt priority 3
	IPC9bits.T8IS = 0x0; // interrupt sub-priority 0
	IFS1bits.T8IF = 0; // T8 clear flag
	IEC1bits.T8IE = 1; // T8 interrupt on
	

	// DMA setup
	IEC4bits.DMA0IE = 0; // disable interrupts
	IFS4bits.DMA0IF = 0; // clear flags
	IEC4bits.DMA1IE = 0; // disable interrupts
	IFS4bits.DMA1IF = 0; // clear flags
	IEC4bits.DMA2IE = 0; // disable interrupts
	IFS4bits.DMA2IF = 0; // clear flags
	IEC4bits.DMA3IE = 1; // enable interrupts
	IFS4bits.DMA3IF = 0; // clear flags
	IEC4bits.DMA4IE = 0; // disable interrupts
	IFS4bits.DMA4IF = 0; // clear flags
	IEC4bits.DMA5IE = 0; // disable interrupts
	IFS4bits.DMA5IF = 0; // clear flags
	IEC4bits.DMA6IE = 0; // disable interrupts
	IFS4bits.DMA6IF = 0; // clear flags
	IEC4bits.DMA7IE = 0; // disable interrupts
	IFS4bits.DMA7IF = 0; // clear flags
	IPC34bits.DMA3IP = 0x6; // interrupt priority level 6
	IPC34bits.DMA3IS = 0x0; // interrupt sub-priority level 0
	DMACONbits.ON = 1; // enable the DMA controller
	
	DCH0CONbits.CHEN = 0; // disable channel
	DCH0ECONbits.CHSIRQ = 12; // start on Output Compare 2 interrupt
	DCH0ECONbits.SIRQEN = 1; // enable start interrupt
	DCH0INT = 0x0000; // clear all interrupts
	DCH0INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH0INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH0CONbits.CHCHN = 0; // disallow chaining
	DCH0CONbits.CHAED = 1; // get next DMA ready for quick transition???
	DCH0CONbits.CHPRI = 0x3; // highest priority
	DCH0SSA = VirtToPhys(screen_zero); // transfer source physical address
	DCH0DSA = VirtToPhys(((unsigned char*)&PORTH + 1)); // transfer destination physical address
	//DCH0DSA = VirtToPhys(&PORTH); // transfer destination physical address
	DCH0SSIZ = 1; // source size
	DCH0DSIZ = 1; // dst size 
	DCH0CSIZ = 1; // 1 byte per event

	DCH1CONbits.CHEN = 0; // disable channel
	DCH1ECONbits.CHSIRQ = 14; // start on Timer 3 interrupt
	DCH1ECONbits.SIRQEN = 1; // enable start interrupt
	DCH1INT = 0x0000; // clear all interrupts
	DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH1INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH1CONbits.CHCHN = 1; // allow chaining
	DCH1CONbits.CHCHNS = 0; // chain from higher channel
	DCH1CONbits.CHAED = 1; // get next DMA ready for quick transition???
	DCH1CONbits.CHPRI = 0x3; // highest priority
	DCH1DSA = VirtToPhys(((unsigned char*)&PORTH + 1)); // transfer destination physical address
	//DCH1DSA = VirtToPhys(&PORTH); // transfer destination physical address
	DCH1SSIZ = SCREEN_X; // source size
	DCH1DSIZ = 1; // dst size 
	DCH1CSIZ = SCREEN_X; // X byte per event
	
	DCH2CONbits.CHEN = 0; // disable channel
	DCH2ECONbits.CHSIRQ = 19; // start on Timer 4 interrupt
	DCH2ECONbits.SIRQEN = 1; // enable start interrupt
	DCH2INT = 0x0000; // clear all interrupts
	DCH2INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH2INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH2CONbits.CHCHN = 1; // allow chaining
	DCH2CONbits.CHCHNS = 0; // chain from higher channel
	DCH2CONbits.CHAED = 1; // get next DMA ready for quick transition???
	DCH2CONbits.CHPRI = 0x3; // highest priority
	DCH2SSA = VirtToPhys(screen_zero); // transfer source physical address
	DCH2DSA = VirtToPhys(((unsigned char*)&PORTH + 1)); // transfer destination physical address
	//DCH2DSA = VirtToPhys(&PORTH); // transfer destination physical address
	DCH2SSIZ = 1; // source size
	DCH2DSIZ = 1; // dst size 
	DCH2CSIZ = 1; // 1 byte per event
	
	// for LCD
	DCH3CONbits.CHEN = 0; // disable channel
	DCH3ECONbits.CHSIRQ = 128; // start on PMP complete
	DCH3ECONbits.CHAIRQ = 129; // abort on PMP error
	DCH3ECONbits.SIRQEN = 1; // enable start interrupt
	DCH3INT = 0x0000; // clear all interrupts
	DCH3INTbits.CHSDIF = 0; // clear source flag
	DCH3INTbits.CHSDIE = 1; // source interrupt enable
	DCH3CONbits.CHCHN = 0; // disallow chaining
	DCH3CONbits.CHCHNS = 0; // chain from higher channel
	DCH3CONbits.CHAED = 0; // disable auto-renable?
	DCH3CONbits.CHPRI = 0x3; // highest priority
	DCH3DSA = VirtToPhys((unsigned char*)&PMDIN); // transfer destination physical address
	DCH3SSIZ = 480*128; // source size
	DCH3DSIZ = 1; // dst size 
	DCH3CSIZ = 1; // bytes per event
	
	// for flood fill function
	DCH4CONbits.CHEN = 0; // disable channel
	DCH4ECONbits.CHSIRQ = 19; // start on Timer 4?
	DCH4ECONbits.CHAIRQ = 0; // disable abort int
	DCH4ECONbits.SIRQEN = 1; // enable start interrupt
	DCH4INT = 0x0000; // clear all interrupts
	DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH1INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH4CONbits.CHCHN = 0; // disallow chaining
	DCH4CONbits.CHCHNS = 0; // chain from higher channel
	DCH4CONbits.CHAED = 1; // detect when disabled
	DCH4CONbits.CHPRI = 0x2; // somewhat high priority
	DCH4SSIZ = 1; // source size
	DCH4DSIZ = (SCREEN_X*SCREEN_Y >> 2); // dst size 
	DCH4CSIZ = (SCREEN_X*SCREEN_Y >> 2); // bytes per event
	
	DCH5CONbits.CHEN = 0; // disable channel
	DCH5ECONbits.CHSIRQ = 19; // start on Timer 4?
	DCH5ECONbits.CHAIRQ = 0; // disable abort int
	DCH5ECONbits.SIRQEN = 1; // enable start interrupt
	DCH5INT = 0x0000; // clear all interrupts
	DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH1INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH5CONbits.CHCHN = 1; // allow chaining
	DCH5CONbits.CHCHNS = 0; // chain from higher channel
	DCH5CONbits.CHAED = 1; // detect when disabled
	DCH5CONbits.CHPRI = 0x2; // somewhat high priority
	DCH5SSIZ = 1; // source size
	DCH5DSIZ = (SCREEN_X*SCREEN_Y >> 2); // dst size 
	DCH5CSIZ = (SCREEN_X*SCREEN_Y >> 2); // bytes per event
	
	DCH6CONbits.CHEN = 0; // disable channel
	DCH6ECONbits.CHSIRQ = 19; // start on Timer 4?
	DCH6ECONbits.CHAIRQ = 0; // disable abort int
	DCH6ECONbits.SIRQEN = 1; // enable start interrupt
	DCH6INT = 0x0000; // clear all interrupts
	DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH1INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH6CONbits.CHCHN = 1; // allow chaining
	DCH6CONbits.CHCHNS = 0; // chain from higher channel
	DCH6CONbits.CHAED = 1; // detect when disabled
	DCH6CONbits.CHPRI = 0x2; // somewhat high priority
	DCH6SSIZ = 1; // source size
	DCH6DSIZ = (SCREEN_X*SCREEN_Y >> 2); // dst size 
	DCH6CSIZ = (SCREEN_X*SCREEN_Y >> 2); // bytes per event
	
	DCH7CONbits.CHEN = 0; // disable channel
	DCH7ECONbits.CHSIRQ = 19; // start on Timer 4?
	DCH7ECONbits.CHAIRQ = 0; // disable abort int
	DCH7ECONbits.SIRQEN = 1; // enable start interrupt
	DCH7INT = 0x0000; // clear all interrupts
	DCH1INTbits.CHBCIF = 0; // clear transfer complete flag
	DCH1INTbits.CHBCIE = 1; // enable transfer complete interrupt
	DCH7CONbits.CHCHN = 1; // allow chaining
	DCH7CONbits.CHCHNS = 0; // chain from higher channel
	DCH7CONbits.CHAED = 1; // detect when disabled
	DCH7CONbits.CHPRI = 0x2; // somewhat high priority
	DCH7SSIZ = 1; // source size
	DCH7DSIZ = (SCREEN_X*SCREEN_Y >> 2); // dst size 
	DCH7CSIZ = (SCREEN_X*SCREEN_Y >> 2); // bytes per event
	
	// default to black screen
	PORTH = 0;
	
	for (unsigned short i=0; i<AUDIO_EXT; i++)
	{
		audio_buffer[i] = 0x00;
		audio_buffer2[i] = 0x00;
	}
	
	for (unsigned short y=0; y<SCREEN_Y2; y++)
	{
		for (unsigned short x=0; x<SCREEN_X; x++)
		{
			screen_buffer[y*SCREEN_X+x] = 0x00;
		}
	}
	
	for (unsigned short i=0; i<SCREEN_X; i++)
	{
		screen_line[i] = 0x00;
	}
	
	screen_frame = 0;
	screen_scanline = 771; //1025; // start of vertical sync
	
	// turn on video timers
	T5CONbits.ON = 1; // turn on TMR5 horizontal sync (cycle offset pre-calculated above)
	T3CONbits.ON = 1; // turn on TMR3 scanline start (cycle offset pre-calculated above)
	T4CONbits.ON = 1; // turn on TMR4 scanline end (independent of others)
	T6CONbits.ON = 1; // turn on TMR6 vertical sync (cycle offset pre-calculated above)
	
	T8CONbits.ON = 1; // turn on TMR8 for audio
	
	return;
}
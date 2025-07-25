

// wait until busy flag is clear
void sqi_wait()
{
	unsigned long busy = 1;
			
	while (busy > 0)
	{
		// read status
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00090001; // Read Status
		SQI1TXDATA = 0x05000000; // 0x00, 0x00, 0x00, then 0x05
		while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
		SQI1CON = 0x004A0004; // read 4 bytes
		while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
		busy = SQI1RXDATA; // read value in buffer		
		busy = (busy & 0x01); // get busy flag
	}
	
	return;
}

// get 4 bytes from chip but do not read from buffer
void sqi_prepare(unsigned long addr)
{
	//unsigned long val;
	
	// read
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00090003; // Fast Read and 2x Address
	SQI1TXDATA = (addr & 0x00FF0000) |
		((addr & 0x0000FF00) << 16) |
		0x00000B00; // 0x00, 0x0B, then Upper Address
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // Address and 3x Dummys
	SQI1TXDATA = (addr & 0x000000FF) | 0x00000000; // Lower Address, 0x00, 0x00, then 0x00
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x004A0004; // read 4 bytes (and stop)
	
	//while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	//val = SQI1RXDATA; // read value in buffer
	
	//return val;
}

// read 4 bytes from buffer
unsigned long sqi_read()
{
	unsigned long val;
	
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	return val;
}

// initialize chip (and can be used for debugging)
unsigned char sqi_initialize()
{
	unsigned long val = 0, loop = 0, count = 0;
	
	// initialize
	CFGCONbits.TROEN=0; // Disable trace outputs (SQI pins share trace)
	// REFCLKO2 is assumed to be SQI base clock
	if (!REFO2CONbits.ACTIVE) // Check if REFCLKO2 divider circuit is active
	{
		REFO2CONbits.RODIV=0x08; // Set the divider (SYSCLK/32)
		REFO2CONbits.ON=1; // Turn on the divider circuit
		while (REFO2CONbits.DIVSWEN) { } // Wait for divide to occur
		REFO2CONbits.OE=1; // Output enable
	}
	SQI1CFG = 0x00810000; // Enable and reset SQI
	SQI1CFG = 0x01A01019; // Configure SQI
	SQI1CLKCON |= (0x08 << 8); // Set divider to (TBC/32)
	SQI1CLKCON = 0x00000001; // Enable clock circuit
	while (!SQI1CLKCONbits.STABLE) { } // Wait for clock to be stable
	DelayMS(100); // short delay just in case
	SQI1THR = 0x00000004; // Set control buffer threshold to 4 bytes
	SQI1INTTHR = 0x00000404; // Set SQI TX/RX interrupt threshold to 4 bytes
	SQI1CMDTHR = 0x00000404; // Set SQI TX/RX command threshold to 4 bytes
	
	// delay
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	
	// read id
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00010001; // ID (single lane), IS25LPxx should yield 0x9D16609D
	//SQI1TXDATA = 0x9F000000; // 0x00, 0x00, 0x00, then 0x9F
	//while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	//SQI1CON = 0x00420004; // read 4 bytes (single lane)
	//while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	//val = SQI1RXDATA; // read value in buffer

	//SendLongHex(val);
	//SendChar('.');
	//DelayMS(10);
	
	loop = 1;
	count = 0;
	
	while (loop > 0)
	{		
		// switch to quad mode
		SQI1CON = 0x00410001; // NOP (single lane)
		SQI1CON = 0x00410001; // NOP (single lane)
		SQI1CON = 0x00410001; // NOP (single lane)
		SQI1CON = 0x00410001; // Enter Quad (single lane), can now use quad lane
		SQI1TXDATA = 0x35000000; // 0x00, 0x00, 0x00, then 0x35
		while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish

		// read id
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00490001; // NOP
		SQI1CON = 0x00090001; // ID
		SQI1TXDATA = 0xAF000000; // 0x00, 0x00, 0x00, then 0xAF
		while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
		SQI1CON = 0x004A0004; // read 4 bytes, IS25LPxx should yield 0x9D16609D
		while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
		val = SQI1RXDATA; // read value in buffer

		//SendLongHex(val);
		//SendChar('.');
		//DelayMS(10);
		
		if (val != 0x9D16609D)
		{
			// reset
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // Reset Enable
			SQI1TXDATA = 0x66000000; // 0x00, 0x00, 0x00, then 0x66
			while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
			DelayMS(100); // short delay needed!
			SQI1CON = 0x00490001; // Reset 
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1TXDATA = 0x00000099; // 0x99, 0x00, 0x00, then 0x00
			while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
			DelayMS(100); // short delay needed!

			// delay
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			DelayMS(100);
			
			count++;
			
			if (count >= 16)
			{
				return 0;
			}
		}
		else
		{
			loop = 0;
		}
	}
	
	sqi_wait();
	
	return 1;
}

// erase chip and write whole file
unsigned char sqi_write(const char *directory, const char *filename)
{	
	// Global variables
	FIL file; // File handle for the file we open
	DIR dir; // Directory information for the current directory
	FATFS fso; // File System Object for the file system we are reading from
	
	// Wait for the disk to initialise
	while(disk_initialize(0));
	// Mount the disk
	f_mount(&fso, "", 0);
	// Change dir to the root directory
	f_chdir(directory);
	// Open the directory
	f_opendir(&dir, ".");
 
	unsigned int bytes;
	unsigned char buffer[4][1];
	unsigned long word;
	unsigned int result;
	unsigned long addr, sector, page;
	unsigned long stop = 0;
	
	result = f_open(&file, filename, FA_READ);
	if (result == 0)
	{
		for (addr=0; addr<0x00400000; addr+=4096) // up to 4MB
		{	
			//SendLongHex(addr);
			//SendChar('.');
			//DelayMS(10);

			// erase
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // NOP
			SQI1CON = 0x00490001; // Write Enable
			SQI1TXDATA = 0x06000000; // 0x00, 0x00, 0x00, then 0x06
			while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
			SQI1CON = 0x00490004; // Sector Erase and Address
			SQI1TXDATA = (((addr & 0x00FF0000) >> 8) | 
				((addr & 0x0000FF00) << 8) | 
				((addr & 0x000000FF) << 16) | 
				0x00000020); // 0x20, then address
			while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish

			sqi_wait();
			
			for (sector=0; sector<0x00001000; sector+=256)
			{
				// write
				SQI1CON = 0x00490001; // NOP
				SQI1CON = 0x00490001; // NOP
				SQI1CON = 0x00490001; // NOP
				SQI1CON = 0x00490001; // Write Enable
				SQI1TXDATA = 0x06000000; // 0x00, 0x00, 0x00, then 0x06
				while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
				SQI1CON = 0x00090004; // Page Program and Address
				SQI1TXDATA = ((((addr+sector) & 0x00FF0000) >> 8) | 
					(((addr+sector) & 0x0000FF00) << 8) | 
					(((addr+sector) & 0x000000FF) << 16) | 
					0x00000002); // 0x02, then address
				while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish

				for (page=0; page<0x00000100; page+=4) // 4 bytes at a time
				{
					while (f_read(&file, &buffer[0], 1, &bytes) != 0) { } // MUST READ ONE BYTE AT A TIME!!!
					while (f_read(&file, &buffer[1], 1, &bytes) != 0) { } // MUST READ ONE BYTE AT A TIME!!!
					while (f_read(&file, &buffer[2], 1, &bytes) != 0) { } // MUST READ ONE BYTE AT A TIME!!!
					while (f_read(&file, &buffer[3], 1, &bytes) != 0) { } // MUST READ ONE BYTE AT A TIME!!!

					word = ((unsigned long)buffer[3][0] << 24) + 
						((unsigned long)buffer[2][0] << 16) + 
						((unsigned long)buffer[1][0] << 8) + 
						((unsigned long)buffer[0][0]);

					if (bytes > 0) 
					{
						if (page == 0x000000FC)
						{
							SQI1CON = 0x00490004; // write 4 bytes (and stop)
							SQI1TXDATA = word; // value written
							while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	
							sqi_wait();
						}
						else
						{
							SQI1CON = 0x00090004; // write 4 bytes
							SQI1TXDATA = word; // value written
							while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish							
						}
					}
					else
					{
						stop = addr;
						break;
					}
				}

				if (stop > 0)
				{
					// exit loop
					sector = 0x00FFFFFF;
					addr = 0x00FFFFFF;
				}
			}
		}

		while (f_sync(&file) != 0) { }
		while (f_close(&file) != 0) { }
		
		//SendString("Finished rom at \\");
		//SendLongHex(stop);
		//SendString("\r\n\\");
		
		// dummy read
		sqi_prepare(0);
		sqi_read(); 
		
		return 1;
	}

	//SendString("Could not find rom file\n\r\\");
		
	return 0;
}

// just to make sure the chip is working
void sqi_debug()
{
	unsigned long val = 0;
	
	// initialize
	CFGCONbits.TROEN=0; // Disable trace outputs (SQI pins share trace)
	// REFCLKO2 is assumed to be SQI base clock
	if (!REFO2CONbits.ACTIVE) // Check if REFCLKO2 divider circuit is active
	{
		REFO2CONbits.RODIV=0x20; // Set the divider (SYSCLK/128)
		REFO2CONbits.ON=1; // Turn on the divider circuit
		while (REFO2CONbits.DIVSWEN) { } // Wait for divide to occur
		REFO2CONbits.OE=1; // Output enable
	}
	SQI1CFG = 0x00810000; // Enable and reset SQI
	SQI1CFG = 0x01A01019; // Configure SQI
	SQI1CLKCON |= (0x20 << 8); // Set divider to (TBC/128)
	SQI1CLKCON = 0x00000001; // Enable clock circuit
	while (!SQI1CLKCONbits.STABLE) { } // Wait for clock to be stable
	DelayMS(100); // short delay just in case
	SQI1THR = 0x00000004; // Set control buffer threshold to 4 bytes
	SQI1INTTHR = 0x00000404; // Set SQI TX/RX interrupt threshold to 4 bytes
	SQI1CMDTHR = 0x00000404; // Set SQI TX/RX command threshold to 4 bytes
	
	// delay
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	DelayMS(100);
	
	// read id
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00410001; // NOP (single lane)
	//SQI1CON = 0x00010001; // ID (single lane), IS25LPxx should yield 0x9D16609D
	//SQI1TXDATA = 0x9F000000; // 0x00, 0x00, 0x00, then 0x9F
	//while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	//SQI1CON = 0x00420004; // read 4 bytes (single lane)
	//while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	//val = SQI1RXDATA; // read value in buffer

	//SendLongHex(val);
	//SendChar('.');
	//DelayMS(10);
	
	// switch to quad mode
	SQI1CON = 0x00410001; // NOP (single lane)
	SQI1CON = 0x00410001; // NOP (single lane)
	SQI1CON = 0x00410001; // NOP (single lane)
	SQI1CON = 0x00410001; // Enter Quad (single lane), can now use quad lane
	SQI1TXDATA = 0x35000000; // 0x00, 0x00, 0x00, then 0x35
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	
	// reset
	//SQI1CON = 0x00490001; // NOP
	//SQI1CON = 0x00490001; // NOP
	//SQI1CON = 0x00490001; // NOP
	//SQI1CON = 0x00490001; // Reset Enable
	//SQI1TXDATA = 0x66000000; // 0x00, 0x00, 0x00, then 0x66
	//while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	//DelayMS(100); // short delay needed!
	//SQI1CON = 0x00490001; // Reset 
	//SQI1CON = 0x00490001; // NOP
	//SQI1CON = 0x00490001; // NOP
	//SQI1CON = 0x00490001; // NOP
	//SQI1TXDATA = 0x00000099; // 0x99, 0x00, 0x00, then 0x00
	//while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	//DelayMS(100); // short delay needed!
	
	// delay
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	//DelayMS(100);
	
	// read id
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00090001; // ID
	SQI1TXDATA = 0xAF000000; // 0x00, 0x00, 0x00, then 0xAF
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x004A0004; // read 4 bytes, IS25LPxx should yield 0x9D16609D
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	
	// read status
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00090001; // Read Status
	SQI1TXDATA = 0x05000000; // 0x00, 0x00, 0x00, then 0x05
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x004A0004; // read 4 bytes
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	
	/*
	// erase
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // Write Enable
	SQI1CON = 0x00490001; // Erase Chip
	SQI1TXDATA = 0xC7060000; // 0x00, 0x00, 0x06, then 0xC7
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	
	for (unsigned char i=0; i<30; i++) // 30 second delay
	{
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
		DelayMS(100);
	}
	
	// write
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00490001; // Write Enable
	SQI1TXDATA = 0x06000000; // 0x00, 0x00, 0x00, then 0x06
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // Page Program and Address
	SQI1TXDATA = 0x00000002; // 0x02, 0x00, 0x00, then 0x00
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // write 4 bytes
	SQI1TXDATA = 0xFF005AA5; // random vales
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // write 4 bytes
	SQI1TXDATA = 0xDEADBEEF; // random values
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // write 4 bytes
	SQI1TXDATA = 0x03020100; // random values
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00490004; // write 4 bytes (and stop)
	SQI1TXDATA = 0x07060504; // random values
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	DelayMS(100); // short delay is needed here!
	
	// read
	SQI1CON = 0x00490001; // NOP
	SQI1CON = 0x00090003; // Fast Read and 2x Address
	SQI1TXDATA = 0x00000B00; // 0x00, 0x0B, 0x00, then 0x00
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x00090004; // Last Address, and 3x Dummys
	SQI1TXDATA = 0x00000000; // 0x00, 0x00, 0x00, then 0x00
	while ((SQI1STAT1 & 0x3F0000) < 0x200000) { } // wait for transmit to finish
	SQI1CON = 0x000A0004; // read 4 bytes
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	
	SQI1CON = 0x000A0004; // read 4 bytes
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	
	SQI1CON = 0x000A0004; // read 4 bytes
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	
	SQI1CON = 0x004A0004; // read 4 bytes (and stop)
	while ((SQI1STAT1 & 0x3F) < 4) { } // wait for receive to finish
	val = SQI1RXDATA; // read value in buffer
	
	SendLongHex(val);
	SendChar('.');
	DelayMS(10);
	*/
	 
	while (1) { } // infinite loop
}
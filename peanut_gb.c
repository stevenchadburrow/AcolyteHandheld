/**
 * MIT License
 * Copyright (c) 2018-2023 Mahyar Koshkouei
 *
 * An example of using the peanut_gb.h library. This example application uses
 * SDL2 to draw the screen and get input.
 */

#include <stdint.h>
#include <stdlib.h>

#include "minigb_apu.h"
#include "minigb_apu.c"

#include "peanut_gb.h"


struct priv_t
{
	uint8_t dummy;
};


volatile uint8_t __attribute__((coherent)) boot_rom[256];
volatile uint8_t selected_palette_vga[3][7];
volatile uint16_t selected_palette_lcd[3][7];

uint8_t frame_counter = 0;

unsigned char reset_check = 0;
unsigned char palette_num = 0;

// Returns a byte from the ROM file at the given address.
uint8_t gb_rom_read(struct gb_s *gb, const uint_fast32_t addr)
{
	return cart_rom[addr];
}

// Returns a byte from the cartridge RAM at the given address.
uint8_t gb_cart_ram_read(struct gb_s *gb, const uint_fast32_t addr)
{
	return cart_ram[addr];
}

// Writes a given byte to the cartridge RAM at the given address.
void gb_cart_ram_write(struct gb_s *gb, const uint_fast32_t addr,
		       const uint8_t val)
{
	cart_ram[addr] = val;
}

uint8_t gb_boot_rom_read(struct gb_s *gb, const uint_fast16_t addr)
{
	return boot_rom[addr];
}

unsigned char gb_write_cart_ram_file(char filename[16])
{	
	// Global variables
	FIL file; // File handle for the file we open
	DIR dir; // Directory information for the current directory
	FATFS fso; // File System Object for the file system we are reading from
	
	//SendString("Initializing disk\n\r\\");
	
	// Wait for the disk to initialise
    while(disk_initialize(0));
    // Mount the disk
    f_mount(&fso, "", 0);
    // Change dir to the root directory
    f_chdir("/");
    // Open the directory
    f_opendir(&dir, ".");
 
	unsigned char buffer[1];
	unsigned int bytes;
	unsigned int result;
	unsigned char flag;
	
	result = f_open(&file, filename, FA_CREATE_ALWAYS | FA_WRITE);
	if (result == 0)
	{		
		for (unsigned int i=0; i<32768; i++)
		{
			buffer[0] = cart_ram[i];
			
			while (f_write(&file, buffer, 1, &bytes) != 0) { }
		}
		
		while (f_sync(&file) != 0) { }
		while (f_close(&file) != 0) { }
		
		//SendString("Wrote all memory to file\n\r\\");
		
		flag = 1;
	}
	else
	{
		//SendString("Could not write all memory to file\n\r\\");
		
		flag = 0;
	}	
	
	return flag;
}

unsigned char gb_read_cart_ram_file(char filename[16])
{	
	// Global variables
	FIL file; // File handle for the file we open
	DIR dir; // Directory information for the current directory
	FATFS fso; // File System Object for the file system we are reading from
	
	//SendString("Initializing disk\n\r\\");
	
	// Wait for the disk to initialise
    while(disk_initialize(0));
    // Mount the disk
    f_mount(&fso, "", 0);
    // Change dir to the root directory
    f_chdir("/");
    // Open the directory
    f_opendir(&dir, ".");
 
	unsigned char buffer[1];
	unsigned int bytes;
	unsigned int result;
	unsigned char flag;
	
	result = f_open(&file, filename, FA_READ);
	if (result == 0)
	{		
		for (unsigned int i=0; i<32768; i++)
		{
			while (f_read(&file, &buffer[0], 1, &bytes) != 0) { } // MUST READ ONE BYTE AT A TIME!!!
			
			cart_ram[i] = buffer[0];
		}
		
		while (f_sync(&file) != 0) { }
		while (f_close(&file) != 0) { }
		
		//SendString("Read all memory from file\n\r\\");
		
		flag = 1;
	}
	else
	{		
		//SendString("Could not read all memory from file\n\r\\");
		
		flag = 0;
		
		nes_error(0x00);
	}	
	
	return flag;
}

/**
 * Handles an error reported by the emulator. The emulator context may be used
 * to better understand why the error given in gb_err was reported.
 */
void gb_error(struct gb_s *gb, const enum gb_error_e gb_err, const uint16_t addr)
{
	const char* gb_err_str[GB_INVALID_MAX] = {
		"UNKNOWN\\",
		"INVALID OPCODE\\",
		"INVALID READ\\",
		"INVALID WRITE\\",
		"HALT FOREVER\\",
		"INVALID_SIZE\\"
	};
	
	uint8_t instr_byte;

	instr_byte = __gb_read(gb, addr);
	
	unsigned long long_addr = addr;

	if(addr >= 0x4000 && addr < 0x8000)
	{
		long_addr = (uint32_t)addr * (uint32_t)gb->selected_rom_bank;
	}

	SendString("Error \\");
	SendString((char *)gb_err_str[gb_err]);
	SendChar(' ');
	SendLongHex(long_addr);
	SendChar(' ');
	SendHex(instr_byte);
	SendString("\r\n\\");

	while (1) { }
}

/**
 * Automatically assigns a colour palette to the game using a given game
 * checksum.
 * TODO: Not all checksums are programmed in yet because I'm lazy.
 */
const unsigned char preset_palette_single[16][21] = {
	//0xFF,0xB6,0x92,0x6D,0x49,0x24,0x00,
	{0xFF,0xB6,0x92,0x6D,0x49,0x24,0x00, 0xFF,0xB6,0x92,0x6D,0x49,0x24,0x00, 0xFF,0xB6,0x92,0x6D,0x49,0x24,0x00}, // greyscale
	{0x70,0x4C,0x4D,0x29,0x29,0x28,0x28, 0x70,0x4C,0x4D,0x29,0x29,0x28,0x28, 0x70,0x4C,0x4D,0x29,0x29,0x28,0x28}, // gb original
	{0xDA,0xB5,0x91,0x6C,0x48,0x24,0x00, 0xDA,0xB5,0x91,0x6C,0x48,0x24,0x00, 0xDA,0xB5,0x91,0x6C,0x48,0x24,0x00}, // gb pocket
	{0x16,0x11,0x11,0x0D,0x0D,0x08,0x08, 0x16,0x11,0x11,0x0D,0x0D,0x08,0x08, 0x16,0x11,0x11,0x0D,0x0D,0x08,0x08}, // gb light
	{0xFF,0x9D,0x5C,0x90,0xE8,0x64,0x00, 0xFF,0x9D,0x5C,0x90,0xE8,0x64,0x00, 0xFF,0x9D,0x5C,0x90,0xE8,0x64,0x00},
	{0xFF,0xFD,0xFC,0xEC,0xE0,0x60,0x00, 0xFF,0xFD,0xFC,0xEC,0xE0,0x60,0x00, 0xFF,0xFD,0xFC,0xEC,0xE0,0x60,0x00},
	{0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00, 0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00, 0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00},
	{0x00,0x09,0x12,0x75,0xF8,0xF9,0xFF, 0x00,0x09,0x12,0x75,0xF8,0xF9,0xFF, 0x00,0x09,0x12,0x75,0xF8,0xF9,0xFF},
	{0xFF,0xDA,0xB6,0x6D,0x49,0x24,0x00, 0xFF,0xDA,0xB6,0x6D,0x49,0x24,0x00, 0xFF,0xDA,0xB6,0x6D,0x49,0x24,0x00},
	{0xFE,0xF6,0xF2,0xB2,0x93,0x49,0x00, 0xFE,0xF6,0xF2,0xB2,0x93,0x49,0x00, 0xFE,0xF6,0xF2,0xB2,0x93,0x49,0x00},
	{0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00, 0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00, 0xFF,0xD6,0xD2,0xAD,0x8C,0x68,0x44},
	{0xFF,0xF6,0xF2,0xA9,0x84,0x40,0x00, 0xFF,0xF6,0xF2,0xA9,0x84,0x40,0x00, 0xFF,0xBD,0x7C,0x35,0x0F,0x05,0x00},
	{0xFF,0xF6,0xF2,0xA9,0x84,0x40,0x00, 0xFF,0xFA,0xF5,0xAC,0x84,0x40,0x00, 0xFF,0xB7,0x93,0x6E,0x4A,0x25,0x00},
	{0xFF,0xBD,0x7C,0x34,0x10,0x08,0x00, 0xFF,0xBB,0x77,0x2B,0x03,0x01,0x00, 0xFF,0xF6,0xF2,0xA9,0x84,0x40,0x00},
	{0xFF,0xF6,0xF2,0xA9,0x84,0x40,0x00, 0xFF,0xBD,0x7C,0x34,0x10,0x08,0x00, 0xFF,0xBB,0x77,0x2B,0x03,0x01,0x00},
	{0xFF,0xBB,0x77,0x2B,0x03,0x01,0x00, 0xFF,0xBD,0x7C,0x34,0x10,0x08,0x00, 0xFF,0xFD,0xFC,0xB0,0x68,0x24,0x00}
};

const unsigned short preset_palette_double[16][21] = {
	//0xC71C,0x8514,0x8410,0x4303,0x4208,0x0104,0x0000,
	{0xC71C,0x8514,0x8410,0x430C,0x4208,0x0104,0x0000, 0xC71C,0x8514,0x8410,0x430C,0x4208,0x0104,0x0000, 0xC71C,0x8514,0x8410,0x430C,0x4208,0x0104,0x0000}, // greyscale
	{0x040C,0x0308,0x4308,0x4204,0x4204,0x0204,0x0204, 0x040C,0x0308,0x4308,0x4204,0x4204,0x0204,0x0204, 0x040C,0x0308,0x4308,0x4204,0x4204,0x0204,0x0204}, // gb original
	{0x8618,0x4514,0x4410,0x030C,0x0208,0x0104,0x0000, 0x8618,0x4514,0x4410,0x030C,0x0208,0x0104,0x0000, 0x8618,0x4514,0x4410,0x030C,0x0208,0x0104,0x0000}, // gb pocket
	{0x8500,0x4400,0x4400,0x4300,0x4300,0x0200,0x0200, 0x8500,0x4400,0x4400,0x4300,0x4300,0x0200,0x0200, 0x8500,0x4400,0x4400,0x4300,0x4300,0x0200,0x0200}, // gb light
	{0xC71C,0x4710,0x0708,0x0410,0x021C,0x010C,0x0000, 0xC71C,0x4710,0x0708,0x0410,0x021C,0x010C,0x0000, 0xC71C,0x4710,0x0708,0x0410,0x021C,0x010C,0x0000},
	{0xC71C,0x471C,0x071C,0x031C,0x001C,0x000C,0x0000, 0xC71C,0x471C,0x071C,0x031C,0x001C,0x000C,0x0000, 0xC71C,0x471C,0x071C,0x031C,0x001C,0x000C,0x0000},
	{0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000, 0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000, 0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000},
	{0x0000,0x4200,0x8400,0x450C,0x061C,0x461C,0xC71C, 0x0000,0x4200,0x8400,0x450C,0x061C,0x461C,0xC71C, 0x0000,0x4200,0x8400,0x450C,0x061C,0x461C,0xC71C},
	{0xC71C,0x8618,0x8514,0x430C,0x4208,0x0104,0x0000, 0xC71C,0x8618,0x8514,0x430C,0x4208,0x0104,0x0000, 0xC71C,0x8618,0x8514,0x430C,0x4208,0x0104,0x0000},
	{0x871C,0x851C,0x841C,0x8414,0xC410,0x4208,0x0000, 0x871C,0x851C,0x841C,0x8414,0xC410,0x4208,0x0000, 0x871C,0x851C,0x841C,0x8414,0xC410,0x4208,0x0000},
	{0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000, 0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000, 0xC71C,0x8518,0x8418,0x4314,0x0310,0x020C,0x0108},
	{0xC71C,0x851C,0x841C,0x4214,0x0110,0x0008,0x0000, 0xC71C,0x851C,0x841C,0x4214,0x0110,0x0008,0x0000, 0xC71C,0x4714,0x070C,0x4504,0xC300,0x4100,0x0000},
	{0xC71C,0x851C,0x841C,0x4214,0x0110,0x0008,0x0000, 0xC71C,0x861C,0x451C,0x0314,0x0110,0x0008,0x0000, 0xC71C,0xC514,0xC410,0x830C,0x8208,0x4104,0x0000},
	{0xC71C,0x4714,0x070C,0x0504,0x0400,0x0200,0x0000, 0xC71C,0xC614,0xC50C,0xC204,0xC000,0x4000,0x0000, 0xC71C,0x851C,0x841C,0x4214,0x0110,0x0008,0x0000},
	{0xC71C,0x851C,0x841C,0x4214,0x0110,0x0008,0x0000, 0xC71C,0x4714,0x070C,0x0504,0x0400,0x0200,0x0000, 0xC71C,0xC614,0xC50C,0xC204,0xC000,0x4000,0x0000},
	{0xC71C,0xC614,0xC50C,0xC204,0xC000,0x4000,0x0000, 0xC71C,0x4714,0x070C,0x0504,0x0400,0x0200,0x0000, 0xC71C,0x471C,0x071C,0x0414,0x020C,0x0104,0x0000},
};

void auto_assign_palette(uint8_t game_checksum, unsigned char val)
{
	for (int i=0; i<3; i++)
	{
		for (int j=0; j<7; j++)
		{
			selected_palette_vga[i][j] = preset_palette_single[val][i*7+j];
		}
	}
	
	for (int i=0; i<3; i++)
	{
		for (int j=0; j<7; j++)
		{
			selected_palette_lcd[i][j] = preset_palette_double[val][i*7+j];
		}
	}
}


#if ENABLE_LCD
/**
 * Draws scanline into framebuffer.
 */
void lcd_draw_line(struct gb_s *gb, 
		//const uint8_t pixels1[160],
		//const uint8_t pixels2[160],
		const uint_fast8_t line
	)
{	
	if (scanline_scaled == 0)
	{
		if (screen_handheld == 0)
		{
			for(unsigned int x = 0; x < LCD_WIDTH; x++)
			{
				screen_pixel_vga_raw(x+48, line+48, selected_palette_vga[(scanline_pixels1[(x)] & LCD_PALETTE_ALL) >> 4][((scanline_pixels1[(x)] & 3)<<1)]);
			}
		}
		else
		{
			for(unsigned int x = 0; x < LCD_WIDTH; x++)
			{
				screen_pixel_lcd_raw(x+48, line+48, selected_palette_lcd[(scanline_pixels1[(x)] & LCD_PALETTE_ALL) >> 4][((scanline_pixels1[(x)] & 3)<<1)]);
			}
		}
	}
	else
	{
		unsigned char grid[3][3];

		unsigned short pos_x = 8;
		unsigned short pos_y = 12 + scanline_count;

		unsigned char pal[2][2];

		if (screen_handheld == 0)
		{
			for (unsigned int x = 0; x < LCD_WIDTH; x+=2)
			{
				grid[0][0] = ((scanline_pixels1[x] & 3) << 1);
				grid[2][0] = ((scanline_pixels1[x+1] & 3) << 1);
				grid[1][0] = ((grid[0][0] + grid[2][0]) >> 1);

				grid[0][2] = ((scanline_pixels2[x] & 3) << 1);
				grid[2][2] = ((scanline_pixels2[x+1] & 3) << 1);
				grid[1][2] = ((grid[0][2] + grid[2][2]) >> 1);

				grid[0][1] = ((grid[0][0] + grid[0][2]) >> 1);
				grid[1][1] = ((grid[1][0] + grid[1][2]) >> 1);
				grid[2][1] = ((grid[2][0] + grid[2][2]) >> 1);

				pal[0][0] = ((scanline_pixels1[x]&LCD_PALETTE_ALL) >> 4);
				pal[1][0] = ((scanline_pixels1[x+1]&LCD_PALETTE_ALL) >> 4);
				pal[0][1] = ((scanline_pixels2[x]&LCD_PALETTE_ALL) >> 4);
				pal[1][1] = ((scanline_pixels2[x+1]&LCD_PALETTE_ALL) >> 4);

				screen_pixel_vga_raw(pos_x, pos_y, selected_palette_vga[pal[0][0]][grid[0][0]]);
				screen_pixel_vga_raw(pos_x, pos_y+1, selected_palette_vga[pal[0][0]][grid[0][1]]);
				screen_pixel_vga_raw(pos_x, pos_y+2, selected_palette_vga[pal[1][0]][grid[0][2]]);

				pos_x++;

				screen_pixel_vga_raw(pos_x, pos_y, selected_palette_vga[pal[0][0]][grid[1][0]]);
				screen_pixel_vga_raw(pos_x, pos_y+1, selected_palette_vga[pal[0][0]][grid[1][1]]);
				screen_pixel_vga_raw(pos_x, pos_y+2, selected_palette_vga[pal[1][0]][grid[1][2]]);

				pos_x++;

				screen_pixel_vga_raw(pos_x, pos_y, selected_palette_vga[pal[0][1]][grid[2][0]]);
				screen_pixel_vga_raw(pos_x, pos_y+1, selected_palette_vga[pal[0][1]][grid[2][1]]);
				screen_pixel_vga_raw(pos_x, pos_y+2, selected_palette_vga[pal[1][1]][grid[2][2]]);

				pos_x++;
			}
		}
		else
		{
			for (unsigned int x = 0; x < LCD_WIDTH; x+=2)
			{
				grid[0][0] = ((scanline_pixels1[x] & 3) << 1);
				grid[2][0] = ((scanline_pixels1[x+1] & 3) << 1);
				grid[1][0] = ((grid[0][0] + grid[2][0]) >> 1);

				grid[0][2] = ((scanline_pixels2[x] & 3) << 1);
				grid[2][2] = ((scanline_pixels2[x+1] & 3) << 1);
				grid[1][2] = ((grid[0][2] + grid[2][2]) >> 1);

				grid[0][1] = ((grid[0][0] + grid[0][2]) >> 1);
				grid[1][1] = ((grid[1][0] + grid[1][2]) >> 1);
				grid[2][1] = ((grid[2][0] + grid[2][2]) >> 1);

				pal[0][0] = ((scanline_pixels1[x]&LCD_PALETTE_ALL) >> 4);
				pal[1][0] = ((scanline_pixels1[x+1]&LCD_PALETTE_ALL) >> 4);
				pal[0][1] = ((scanline_pixels2[x]&LCD_PALETTE_ALL) >> 4);
				pal[1][1] = ((scanline_pixels2[x+1]&LCD_PALETTE_ALL) >> 4);

				screen_pixel_lcd_raw(pos_x, pos_y, selected_palette_lcd[pal[0][0]][grid[0][0]]);
				screen_pixel_lcd_raw(pos_x, pos_y+1, selected_palette_lcd[pal[0][0]][grid[0][1]]);
				screen_pixel_lcd_raw(pos_x, pos_y+2, selected_palette_lcd[pal[1][0]][grid[0][2]]);

				pos_x++;

				screen_pixel_lcd_raw(pos_x, pos_y, selected_palette_lcd[pal[0][0]][grid[1][0]]);
				screen_pixel_lcd_raw(pos_x, pos_y+1, selected_palette_lcd[pal[0][0]][grid[1][1]]);
				screen_pixel_lcd_raw(pos_x, pos_y+2, selected_palette_lcd[pal[1][0]][grid[1][2]]);

				pos_x++;

				screen_pixel_lcd_raw(pos_x, pos_y, selected_palette_lcd[pal[0][1]][grid[2][0]]);
				screen_pixel_lcd_raw(pos_x, pos_y+1, selected_palette_lcd[pal[0][1]][grid[2][1]]);
				screen_pixel_lcd_raw(pos_x, pos_y+2, selected_palette_lcd[pal[1][1]][grid[2][2]]);

				pos_x++;
			}
		}
	}
}
#endif

int PeanutGB()
{
	screen_clear();
	audio_clear();
	
	struct gb_s gb;
	struct priv_t priv;

	enum gb_init_error_e gb_ret;
	int ret = EXIT_SUCCESS;
	
	/* TODO: Sanity check input GB file. */

	/* Initialise emulator context. */
	gb_ret = gb_init(&gb, &gb_rom_read, &gb_cart_ram_read, &gb_cart_ram_write,
			 &gb_error, &priv);

	switch(gb_ret)
	{
	case GB_INIT_NO_ERROR:
		break;

	case GB_INIT_CARTRIDGE_UNSUPPORTED:
		SendString("Unsupported cartridge.\r\n\\");
		ret = EXIT_FAILURE;
		while (1) { }

	case GB_INIT_INVALID_CHECKSUM:
		SendString("Invalid ROM: Checksum failure.\r\n\\");
		ret = EXIT_FAILURE;
		while (1) { }

	default:
		SendString("Unknown error \\");
		SendHex(gb_ret);
		SendString("\r\n\\");
		ret = EXIT_FAILURE;
		while (1) { }
	}
	
	// Global variables
	FIL file; // File handle for the file we open
	DIR dir; // Directory information for the current directory
	FATFS fso; // File System Object for the file system we are reading from
	//FILINFO fno;

	// Wait for the disk to initialise
	while(disk_initialize(0));
	// Mount the disk
	f_mount(&fso, "", 0);
	// Change dir to the root directory
	f_chdir("/");
	// Open the directory
	f_opendir(&dir, ".");
	
	unsigned int bytes;
	unsigned char buffer[1];
	unsigned int result;
	
	unsigned char choice = 0;
	unsigned char ps2_found = 0;

	result = f_open(&file, "/DMG-BOOT.BIN", FA_READ);
	if (result == 0)
	{	
		for (unsigned int i=0; i<256; i++)
		{
			while (f_read(&file, &buffer, 1, &bytes) != 0) { }

			boot_rom[i] = buffer[0];
		}
		
		while (f_sync(&file) != 0) { }
		while (f_close(&file) != 0) { }

		gb_set_boot_rom(&gb, gb_boot_rom_read);
		gb_reset(&gb);
	}
	else
	{
		SendString("Could not find DMG-BOOT.BIN file\n\r\\");
	}

	/* Set the RTC of the game cartridge. Only used by games that support it. */
	{
		time_t rawtime;
		time(&rawtime);
#ifdef _POSIX_C_SOURCE
		struct tm timeinfo;
		localtime_r(&rawtime, &timeinfo);
#else
		struct tm *timeinfo;
		timeinfo = localtime(&rawtime);
#endif

		/* You could potentially force the game to allow the player to
		 * reset the time by setting the RTC to invalid values.
		 *
		 * Using memset(&gb->cart_rtc, 0xFF, sizeof(gb->cart_rtc)) for
		 * example causes Pokemon Gold/Silver to say "TIME NOT SET",
		 * allowing the player to set the time without having some dumb
		 * password.
		 *
		 * The memset has to be done directly to gb->cart_rtc because
		 * gb_set_rtc() processes the input values, which may cause
		 * games to not detect invalid values.
		 */

		/* Set RTC. Only games that specify support for RTC will use
		 * these values. */
#ifdef _POSIX_C_SOURCE
		gb_set_rtc(&gb, &timeinfo);
#else
		gb_set_rtc(&gb, timeinfo);
#endif
	}

#if ENABLE_LCD
	gb_init_lcd(&gb, &lcd_draw_line);
#endif

	auto_assign_palette(gb_colour_hash(&gb), palette_num); // default
	
	frame_counter = 0;
	
#if ENABLE_SOUND
	T8CON = 0x0000; // reset
	T8CON = 0x0000; // prescale of 1:1, 16-bit
	TMR8 = 0x0000; // zero out counter
	PR8 = 0x207F; // approx twice per scanline (minus one)
	T8CONbits.ON = 1;
#endif
	
	while (1)
	{
		if (reset_check > 0)
		{
			reset_check = 0;
			
			screen_clear();
			audio_clear();
			
			gb_reset(&gb);
		}
		
		if (usb_mode != 0xFF)
		{
			USBHostTasks();
		}
		
		gb.direct.joypad |= JOYPAD_UP;
		gb.direct.joypad |= JOYPAD_DOWN;
		gb.direct.joypad |= JOYPAD_LEFT;
		gb.direct.joypad |= JOYPAD_RIGHT;
		gb.direct.joypad |= JOYPAD_A;
		gb.direct.joypad |= JOYPAD_B;
		gb.direct.joypad |= JOYPAD_SELECT;
		gb.direct.joypad |= JOYPAD_START;

		if ((controller_status_1 & 0x10) == 0x10) gb.direct.joypad &= ~JOYPAD_UP;
		if ((controller_status_1 & 0x20) == 0x20) gb.direct.joypad &= ~JOYPAD_DOWN;
		if ((controller_status_1 & 0x40) == 0x40) gb.direct.joypad &= ~JOYPAD_LEFT;
		if ((controller_status_1 & 0x80) == 0x80) gb.direct.joypad &= ~JOYPAD_RIGHT;
		if ((controller_status_1 & 0x01) == 0x01) gb.direct.joypad &= ~JOYPAD_A;
		if ((controller_status_1 & 0x02) == 0x02) gb.direct.joypad &= ~JOYPAD_B;
		if ((controller_status_1 & 0x04) == 0x04) gb.direct.joypad &= ~JOYPAD_SELECT;
		if ((controller_status_1 & 0x08) == 0x08) gb.direct.joypad &= ~JOYPAD_START;

		frame_counter++;
		
		if (frame_counter == 3) // only draw every three frames
		{
			scanline_draw = 1;
		}
		
		/* Execute CPU cycles until the screen has to be redrawn. */
		gb_run_frame(&gb);
		
		if (frame_counter == 3) // only draw every three frames
		{
			frame_counter = 0;
			screen_flip();
			
			scanline_draw = 0;
		}

#if ENABLE_SOUND
		if (audio_enable > 0)
		{
			if (audio_bank == 0) audio_init();
			
			// playing audio
			if (audio_bank == 1)
			{
				audio_callback(&gb, (uint8_t *)&audio_buffer2, AUDIO_NSAMPLES);
			}
			else if (audio_bank == 2)
			{
				audio_callback(&gb, (uint8_t *)&audio_buffer, AUDIO_NSAMPLES);
			}
		}
#endif
		// speed limiter for when occasionally the Gameboy is too fast
		while (screen_sync == 0) { }
		
		screen_sync = 0;
		
#if ENABLE_SOUND
		if (audio_enable > 0)
		{
			if (audio_bank == 1) audio_bank = 2;
			else audio_bank = 1;
			
			audio_read = 0;
		}
#endif
	}

	return ret;
}


				

/*
MIT License

Copyright (c) 2021 ITotalJustice

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


//#define SMS_DEBUG 1 // TEMPORARY
//#define SMS_SINGLE_FILE 1 // TEMPORARY

unsigned short frame_tally = 0; // counts how many frames between drawing

unsigned char button_disable = 0; // disables PAUSE and RESET buttons

// *** sms_types.h ***


#ifndef SMS_DEBUG
    #define SMS_DEBUG 0
#endif

#ifndef SMS_SINGLE_FILE
    #define SMS_SINGLE_FILE 0
#endif

// This sounds about right...
#define AUDIO_FREQ (690) // 690 = 3.58 MHz * 1000000 / 31250 samples per frame * 6 multiplier??


#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
//#include <string.h>
//#include <assert.h>

// fwd
struct SMS_Ports;
struct SMS_ApuSample;
struct SMS_MemoryControlRegister;
struct SMS_Core;


enum
{
    SMS_SCREEN_WIDTH = 256,
    SMS_SCREEN_HEIGHT = 192, // actually 224 (240 but nothing used it)

    GG_SCREEN_WIDTH = 160,
    GG_SCREEN_HEIGHT = 144,

    SMS_ROM_SIZE_MAX = 1024 * 512, // 512KiB
    SMS_SRAM_SIZE_MAX = 1024 * 16 * 2, // 2 banks of 16kib

    // this value was taken for sms power docs
    SMS_CPU_CLOCK = 3579545,

    SMS_CYCLES_PER_FRAME = SMS_CPU_CLOCK / 60,
};

enum SMS_System
{
    SMS_System_SMS,
    SMS_System_GG,
    SMS_System_SG1000,
};

// this is currently unused!
// enum SMS_SystemMode
// {
//     SMS_SystemMode_SMS1,
//     SMS_SystemMode_SMS2,

//     SMS_SystemMode_GG_SMS, // GG sys, in sms mode
//     SMS_SystemMode_GG, // GG sys in GG mode
// };

struct Z80_GeneralRegisterSet
{
    uint8_t B;
    uint8_t C;
    uint8_t D;
    uint8_t E;
    uint8_t H;
    uint8_t L;
    uint8_t A;

    struct
    {
        bool C : 1;
        bool N : 1;
        bool P : 1;
        bool B3 : 1; // bit3
        bool H : 1;
        bool B5 : 1; // bit5
        bool Z : 1;
        bool S : 1;
    } flags;
};

struct Z80
{
    uint16_t cycles;

    // [special purpose registers]
    uint16_t PC; // program counter
    uint16_t SP; // stack pointer

    // these are actually mainly 16-bit registers, however, some instructions
    // split these into lo / hi bytes, similar to the general_reg_set.
    uint8_t IXL;
    uint8_t IXH;
    uint8_t IYL;
    uint8_t IYH;

    uint8_t I; // interrupt vector
    uint8_t R; // memory refresh

    // [general purpose registers]
    // theres 2-sets, main and alt
    struct Z80_GeneralRegisterSet main;
    struct Z80_GeneralRegisterSet alt;

    // interrupt flipflops
    bool IFF1 : 1;
    bool IFF2 : 1;
    bool ei_delay : 1; // like the gb, ei is delayed by 1 instructions
    bool halt : 1;

    bool interrupt_requested : 1;
};

enum SMS_MapperType
{
    MAPPER_TYPE_SEGA, // nomal sega mapper (can have sram)
    MAPPER_TYPE_CODEMASTERS, // todo:
    MAPPER_TYPE_NONE, // 8K - 48K
    // https://segaretro.org/8kB_RAM_Adapter
    // https://www.smspower.org/forums/13579-Taiwan8KBRAMAdapterForPlayingMSXPortsOnSG1000II
    // thanks to bock for the info about the 2 types of mappings
    // thanks to calindro for letting me know of dahjee adapter :)
    MAPPER_TYPE_DAHJEE_A, // extra 8K ram 0x2000-0x3FFF, normal 1K at 0xC000-0xFFFF
    MAPPER_TYPE_DAHJEE_B, // extra 8K ram 0xC000-0xFFFF

    // special case mappers
    MAPPER_TYPE_THE_CASTLE,
    MAPPER_TYPE_OTHELLO,
};

struct SMS_SegaMapper
{
    struct // control
    {
        bool rom_write_enable;
        bool ram_enable_c0000;
        bool ram_enable_80000;
        bool ram_bank_select;
        uint8_t bank_shift;
    } fffc;

    uint8_t fffd;
    uint8_t fffe;
    uint8_t ffff;
};

struct SMS_CodemastersMapper
{
    uint8_t slot[3];
    bool ram_mapped; // ernie els golf features 8k on cart ram
};

struct SMS_Cart
{
    enum SMS_MapperType mapper_type;

    union
    {
        struct SMS_SegaMapper sega;
        struct SMS_CodemastersMapper codemasters;
    } mappers;

    uint8_t max_bank_mask;
    bool sram_used; // set when game uses sram at any point
};

struct SMS_RomHeader
{
    uint8_t magic[0x8];
    uint16_t checksum;
    uint32_t prod_code;
    uint8_t version;
    uint8_t region_code;
    uint8_t rom_size;
};

enum VDP_Code
{
    VDP_CODE_VRAM_WRITE_LOAD,
    VDP_CODE_VRAM_WRITE,
    VDP_CODE_REG_WRITE,
    VDP_CODE_CRAM_WRITE,
};

struct CachedTile
{
    uint16_t palette_index : 9;
    bool priority : 1;
    bool palette_select : 1;
    bool vertical_flip : 1;
    bool horizontal_flip : 1;
    bool dirty : 1;
};

// basically packed u32, easier for me to understand
struct CachedPalette
{
    uint32_t flipped;
    uint32_t normal;
};

struct SMS_Vdp
{
    // this is used for vram r/w and cram writes.
    volatile uint16_t addr;
    volatile enum VDP_Code code;

	volatile uint8_t *vram; 
    volatile uint8_t *dirty_vram; 
	
	// gross amount of memory, find a place for it hopefully!
    volatile struct CachedPalette cached_palette[(1024 * 16) / 4];
	
    // bg can use either palette, while sprites can only use
    // the second half of the cram.
    volatile uint8_t cram[64];

    // writes to even addresses are latched!
    volatile uint8_t cram_gg_latch;

    // set when cram value changes, the colour callback is then called
    // during rendering of the line.
    volatile bool dirty_cram[64];
    // indicates where the loop should start and end
    volatile uint8_t dirty_cram_min;
    volatile uint8_t dirty_cram_max;

    // the actual colour set to the pixels
    volatile uint32_t colour[32];

    // 16 registers, not all are useable
    volatile uint8_t registers[0x10];

    // vertical scroll is updated when the display is not active,
    // not when the register is updated!
    volatile uint8_t vertical_scroll;

    volatile int16_t cycles;
    volatile uint16_t hcount;
    volatile uint16_t vcount;

    // this differ from above in that this is what will be read on the port.
    // due to the fact that scanlines can be 262 or 312, the value would
    // would eventually overflow, so scanline 256 would read as 0!
    // internally this does not actually wrap around, instead, at set
    // values (differes between ntsc and pal), it will jump back to a
    // previous value, for example, on ntsc, it'll jump from value
    // 218 back to 213, though i am unsure if it keeps jumping...
    volatile uint8_t vcount_port;

    // used for interrupts, reloaded at 0
    volatile uint8_t line_counter;

    // reads are buffered
    volatile uint8_t buffer_read_data;

    // it takes 2 writes to control port to form the control word
    // this can be used to set the addr, vdp reg or set writes
    // to be made to cram
    volatile uint16_t control_word;

    // set if already have lo byte
    volatile bool control_latch : 1;

    // (all of below is cleared upon reading stat)
    // set on vblank
    volatile bool frame_interrupt_pending : 1;
    // set on line counter underflow
    volatile bool line_interrupt_pending : 1;
    // set when there's more than 8(sms)/4(sg) sprites on a line
    volatile bool sprite_overflow : 1;
    // set when a sprite collides
    volatile bool sprite_collision : 1;
    // 5th sprite number sg-1000
    volatile uint8_t fifth_sprite_num : 5;
};

enum SMS_Button
{
    SMS_Button_JOY1_UP      = 1 << 0,
    SMS_Button_JOY1_DOWN    = 1 << 1,
    SMS_Button_JOY1_LEFT    = 1 << 2,
    SMS_Button_JOY1_RIGHT   = 1 << 3,
    SMS_Button_JOY1_A       = 1 << 4,
    SMS_Button_JOY1_B       = 1 << 5,
    SMS_Button_JOY2_UP      = 1 << 6,
    SMS_Button_JOY2_DOWN    = 1 << 7,

    SMS_Button_JOY2_LEFT    = 1 << 8,
    SMS_Button_JOY2_RIGHT   = 1 << 9,
    SMS_Button_JOY2_A       = 1 << 10,
    SMS_Button_JOY2_B       = 1 << 11,
    SMS_Button_RESET        = 1 << 12,
    SMS_Button_PAUSE        = 1 << 13,
};

enum SMS_PortA
{
    JOY1_UP_BUTTON      = 1 << 0,
    JOY1_DOWN_BUTTON    = 1 << 1,
    JOY1_LEFT_BUTTON    = 1 << 2,
    JOY1_RIGHT_BUTTON   = 1 << 3,
    JOY1_A_BUTTON       = 1 << 4,
    JOY1_B_BUTTON       = 1 << 5,
    JOY2_UP_BUTTON      = 1 << 6,
    JOY2_DOWN_BUTTON    = 1 << 7,
};

enum SMS_PortB
{
    JOY2_LEFT_BUTTON    = 1 << 0,
    JOY2_RIGHT_BUTTON   = 1 << 1,
    JOY2_A_BUTTON       = 1 << 2,
    JOY2_B_BUTTON       = 1 << 3,
    RESET_BUTTON        = 1 << 4,
    PAUSE_BUTTON        = 1 << 5,
};

struct SMS_Ports
{
    uint8_t gg_regs[7];
    uint8_t a;
    uint8_t b;
};

struct SMS_ApuSample
{
    uint8_t tone0[2];
    uint8_t tone1[2];
    uint8_t tone2[2];
    uint8_t noise[2];
};

struct SMS_Psg
{
    uint32_t cycles; // elapsed cycles since last psg_sync()

    struct
    {
        int16_t counter; // 10-bits
        uint16_t tone; // 10-bits
    } tone[3];

    struct
    {
        int16_t counter; // 10-bits
        uint16_t lfsr; // can be either 16-bit or 15-bit...
        uint8_t mode; // 1-bits
        uint8_t shift_rate; // 2-bits
        bool flip_flop;
    } noise;

    uint8_t volume[4];
    uint8_t polarity[4];

    // which of the 4 channels are latched.
    uint8_t latched_channel;
    // vol or tone (or mode + shift instead of tone for noise).
    uint8_t latched_type;

    // GG has stereo switches for each channel
    bool channel_enable[4][2];
};

struct SMS_MemoryControlRegister
{
    bool exp_slot_disable : 1;
    bool cart_slot_disable : 1;
    bool card_slot_disable : 1;
    bool work_ram_disable : 1;
    bool bios_rom_disable : 1;
    bool io_chip_disable : 1;
};

struct SMS_Core
{
    // mapped every 0x400 due to how sega mapper works with the first
    // page being fixed (0x400 in size).
    volatile uint8_t* rmap[64]; // 64
    volatile uint8_t* wmap[64]; // 64

    struct Z80 cpu;
    struct SMS_Vdp vdp;
    struct SMS_Psg psg;
    struct SMS_Cart cart;
    struct SMS_Ports port;
    struct SMS_MemoryControlRegister memory_control;

    volatile uint32_t crc;
    volatile enum SMS_System system;

    volatile uint8_t* rom;
    volatile size_t rom_size;
    volatile uint32_t rom_mask;

    volatile uint8_t* bios;
    volatile size_t bios_size;

    volatile uint16_t pitch;
    volatile uint8_t bpp;
    volatile bool skip_frame;

    uint32_t apu_callback_freq; // sample rate
    uint32_t apu_callback_counter; // how many cpu cycles until sample

    // enable to have better sounding drums in most games!
    bool better_drums;
} sms;


// *** sms.h ***

bool SMS_init(struct SMS_Core* sms);
bool SMS_loadbios(struct SMS_Core* sms, volatile uint8_t* bios, size_t size);
bool SMS_loadrom(struct SMS_Core* sms, volatile uint8_t* rom, size_t size, int system_hint);
void SMS_run(struct SMS_Core* sms, size_t cycles);

bool SMS_loadsave(struct SMS_Core* sms, volatile uint8_t* data, size_t size);
bool SMS_used_sram(const struct SMS_Core* sms);

void SMS_skip_frame(struct SMS_Core* sms, bool enable);

// i had a bug in the noise channel which made the drums in all games sound *much*
// better, so much so, that i assumed other emulators emulated the noise channel wrong!
// however, after listening to real hw, the drums were in fact always that bad sounding.
// setting this to true will re-enable better drums!
void SMS_set_better_drums(struct SMS_Core* sms, bool enable);

/**
 * @brief mixes samples into s16 stereo format
 *
 * @param samples the sample buffer
 * @param output number of entires must be count*2
 * @param count number of samples
 */
void SMS_apu_mixer_s16(const struct SMS_ApuSample* samples, int8_t* output, uint32_t count);

void SMS_set_system_type(struct SMS_Core* sms, enum SMS_System system);
enum SMS_System SMS_get_system_type(const struct SMS_Core* sms);
bool SMS_is_system_type_sms(const struct SMS_Core* sms);
bool SMS_is_system_type_gg(const struct SMS_Core* sms);
bool SMS_is_system_type_sg(const struct SMS_Core* sms);

void SMS_get_pixel_region(const struct SMS_Core* sms, int* x, int* y, int* w, int* h);

// [INPUT]
void SMS_set_port_a(struct SMS_Core* sms, enum SMS_PortA pin, bool down);
void SMS_set_port_b(struct SMS_Core* sms, enum SMS_PortB pin, bool down);

uint32_t SMS_crc32(volatile uint8_t *data, size_t size);

// *** sms_internal.h ***

// if neither set, check compiler, else, default to little
#if !defined(SMS_LITTLE_ENDIAN) && !defined(SMS_BIG_ENDIAN)
    #if defined(__BYTE_ORDER)
        #define SMS_LITTLE_ENDIAN (__BYTE_ORDER == __LITTLE_ENDIAN)
        #define SMS_BIG_ENDIAN (__BYTE_ORDER == __BIG_ENDIAN)
    #elif defined(__BYTE_ORDER__)
        #define SMS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        #define SMS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    #else
        #define SMS_LITTLE_ENDIAN (1)
        #define SMS_BIG_ENDIAN (0)
    #endif
#elif defined(SMS_LITTLE_ENDIAN) && !defined(SMS_BIG_ENDIAN)
    #define SMS_BIG_ENDIAN (!SMS_LITTLE_ENDIAN)
#elif !defined(SMS_LITTLE_ENDIAN) && defined(SMS_BIG_ENDIAN)
    #define SMS_LITTLE_ENDIAN (!SMS_BIG_ENDIAN)
#endif

#define SMS_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define SMS_MAX(x, y) (((x) > (y)) ? (x) : (y))


#ifndef SMS_ENABLE_FORCE_INLINE
    #define SMS_ENABLE_FORCE_INLINE 1
#endif

#ifndef SMS_DISABLE_AUDIO
    #define SMS_DISABLE_AUDIO 0
#endif

#if SMS_ENABLE_FORCE_INLINE
    #if defined(_MSC_VER)
        #define FORCE_INLINE inline __forceinline
    #elif defined(__GNUC__)
        #define FORCE_INLINE inline __attribute__((always_inline))
    #elif defined(__clang__)
        #define FORCE_INLINE inline __attribute__((always_inline))
    #else
        #define FORCE_INLINE inline
    #endif
#else
    #define FORCE_INLINE inline
#endif

#if SMS_SINGLE_FILE
    #define SMS_STATIC static
    #define SMS_INLINE static inline
    #define SMS_FORCE_INLINE static FORCE_INLINE
#else
    #define SMS_STATIC
    #define SMS_INLINE
    #define SMS_FORCE_INLINE
#endif // SMS_SINGLE_FILE

#if defined __has_builtin
    #define HAS_BUILTIN(x) __has_builtin(x)
#else
    #if defined(__GNUC__)
        #define HAS_BUILTIN(x) (1)
    #else
        #define HAS_BUILTIN(x) (0)
    #endif
#endif // __has_builtin

#if HAS_BUILTIN(__builtin_expect)
    #define LIKELY(c) (__builtin_expect(c,1))
    #define UNLIKELY(c) (__builtin_expect(c,0))
#else
    #define LIKELY(c) ((c))
    #define UNLIKELY(c) ((c))
#endif // __has_builtin(__builtin_expect)

#if HAS_BUILTIN(__builtin_unreachable)
    #define UNREACHABLE(ret) __builtin_unreachable()
#else
    #define UNREACHABLE(ret) return ret
#endif // __has_builtin(__builtin_unreachable)

// used mainly in debugging when i want to quickly silence
// the compiler about unsed vars.
#define UNUSED(var) ((void)(var))

// ONLY use this for C-arrays, not pointers, not structs
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

//#if SMS_DEBUG
    //#include <stdio.h>
    //#include <assert.h>
    //#define SMS_log(...) fprintf(stdout, __VA_ARGS__)
    //#define SMS_log_err(...) fprintf(stderr, __VA_ARGS__)
    //#define SMS_log_fatal(...) do { fprintf(stderr, __VA_ARGS__); assert(0); } while(0)
//#else
    #define SMS_log(...)
    #define SMS_log_err(...)
    #define SMS_log_fatal(...)
//#endif // SMS_DEBUG

// returns 1 OR 0
#define IS_BIT_SET(v, bit) (!!((v) & (1 << (bit))))

// [CPU]
SMS_STATIC void z80_init(struct SMS_Core* sms);
SMS_FORCE_INLINE void z80_run(struct SMS_Core* sms);
SMS_STATIC void z80_nmi(struct SMS_Core* sms);
SMS_STATIC void z80_irq(struct SMS_Core* sms);

// [BUS]
SMS_FORCE_INLINE uint8_t SMS_read8(struct SMS_Core* sms, uint16_t addr);
SMS_FORCE_INLINE void SMS_write8(struct SMS_Core* sms, uint16_t addr, uint8_t value);
SMS_FORCE_INLINE uint16_t SMS_read16(struct SMS_Core* sms, uint16_t addr);
SMS_FORCE_INLINE void SMS_write16(struct SMS_Core* sms, uint16_t addr, uint16_t value);

SMS_STATIC uint8_t SMS_read_io(struct SMS_Core* sms, uint8_t addr);
SMS_STATIC void SMS_write_io(struct SMS_Core* sms, uint8_t addr, uint8_t value);

SMS_STATIC void mapper_init(struct SMS_Core* sms);
SMS_STATIC void mapper_update(struct SMS_Core* sms);

// [APU]
SMS_INLINE void psg_reg_write(struct SMS_Core* sms, uint8_t value);
SMS_STATIC void psg_sync(struct SMS_Core* sms);
SMS_FORCE_INLINE void psg_run(struct SMS_Core* sms, uint8_t cycles);
SMS_STATIC void psg_init(struct SMS_Core* sms);

SMS_STATIC void vdp_init(struct SMS_Core* sms);
SMS_INLINE uint8_t vdp_status_flag_read(struct SMS_Core* sms);
SMS_INLINE void vdp_io_write(struct SMS_Core* sms, uint8_t addr, uint8_t value);
SMS_FORCE_INLINE bool vdp_has_interrupt(const struct SMS_Core* sms);
SMS_FORCE_INLINE void vdp_run(struct SMS_Core* sms, uint8_t cycles);

// [MISC]
SMS_STATIC bool SMS_has_bios(const struct SMS_Core* sms);
SMS_FORCE_INLINE bool SMS_parity16(uint16_t value);
SMS_FORCE_INLINE bool SMS_parity8(uint8_t value);
SMS_STATIC bool SMS_is_spiderman_int_hack_enabled(const struct SMS_Core* sms);
SMS_STATIC void vdp_mark_palette_dirty(struct SMS_Core* sms);


// *** sms_rom_database.h ***

struct RomEntry
{
    uint32_t crc; // crc32
    uint32_t rom; // size
    uint16_t ram; // size
    uint8_t map; // size
    uint8_t sys; // size
};

bool rom_database_find_entry(struct RomEntry* entry, uint32_t crc);



// *** sms_bus.c ***

// this is mapped to any address space which is unmapped
// and *not* mirror, such as sega mapper writing to 0x0-0x8000 range
static uint8_t UNUSED_BANK[0x400];

static void sega_mapper_update_slot0(struct SMS_Core* sms)
{
    // if we have bios and is loaded, do not map rom here!
    if (SMS_has_bios(sms) && !sms->memory_control.bios_rom_disable)
    {
        return;
    }

    const size_t offset = 0x4000 * sms->cart.mappers.sega.fffd;

    // this is fixed, never updated!
    sms->rmap[0x00] = sms->rom;

    for (size_t i = 1; i < 0x10; ++i)
    {
        // only the first 15 banks are saved
        sms->rmap[i] = sms->rom + offset + (0x400 * i);
    }
}

static void sega_mapper_update_slot1(struct SMS_Core* sms)
{
    const size_t offset = 0x4000 * sms->cart.mappers.sega.fffe;

    for (size_t i = 0; i < 0x10; ++i)
    {
        sms->rmap[i + 0x10] = sms->rom + offset + (0x400 * i);
    }
}

static void sega_mapper_update_slot2(struct SMS_Core* sms)
{
    const size_t offset = 0x4000 * sms->cart.mappers.sega.ffff;

    for (size_t i = 0; i < 0x10; ++i)
    {
        sms->rmap[i + 0x20] = sms->rom + offset + (0x400 * i);
        sms->wmap[i + 0x20] = UNUSED_BANK;
    }
}

static void sega_mapper_update_ram0(struct SMS_Core* sms)
{
    sms->cart.sram_used = true;

    for (size_t i = 0; i < 0x10; ++i)
    {
        sms->rmap[i + 0x20] = cart_ram + (0x4000 * sms->cart.mappers.sega.fffc.ram_bank_select) + (0x400 * i);
        sms->wmap[i + 0x20] = cart_ram + (0x4000 * sms->cart.mappers.sega.fffc.ram_bank_select) + (0x400 * i);
    }
}

static void setup_mapper_unused_ram(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x40; i++)
    {
        sms->rmap[i] = UNUSED_BANK;
        sms->wmap[i] = UNUSED_BANK;
    }
}

static void setup_mapper_none(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x30; i++)
    {
        sms->rmap[i] = sms->rom + (0x400 * (i % sms->rom_mask));
    }

    // sg only has 1k of ram
    // todo: maybe have different variants of MAPPER_NONE for each system?
    // such as sms, sg, msx etc
    const uint8_t ram_mask = SMS_is_system_type_sg(sms) ? 0x0 : 0x7;

    for (int i = 0; i < 0x10; ++i)
    {
        sms->rmap[0x30 + i] = sys_ram + (0x400 * (i & ram_mask));
        sms->wmap[0x30 + i] = sys_ram + (0x400 * (i & ram_mask));
    }
}

static void init_mapper_sega(struct SMS_Core* sms)
{
    // control is reset to zero
	sms->cart.mappers.sega.fffc.rom_write_enable = 0;
    sms->cart.mappers.sega.fffc.ram_enable_c0000 = 0;
    sms->cart.mappers.sega.fffc.ram_enable_80000 = 0;
    sms->cart.mappers.sega.fffc.ram_bank_select = 0;
    sms->cart.mappers.sega.fffc.bank_shift = 0;

    // default banks
    sms->cart.mappers.sega.fffd = 0;
    sms->cart.mappers.sega.fffe = 1;
    sms->cart.mappers.sega.ffff = 2;
}

static void setup_mapper_sega(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x10; ++i)
    {
        sms->rmap[0x30 + i] = sys_ram + (0x400 * (i & 0x7));
        sms->wmap[0x30 + i] = sys_ram + (0x400 * (i & 0x7));
    }

    sega_mapper_update_slot0(sms);
    sega_mapper_update_slot1(sms);
    sega_mapper_update_slot2(sms);
}

static void codemaster_mapper_update_slot(struct SMS_Core* sms, const uint8_t slot, const uint8_t value)
{
    sms->cart.mappers.codemasters.slot[slot] = value & sms->cart.max_bank_mask;
    const size_t offset = 0x4000 * sms->cart.mappers.codemasters.slot[slot];

    for (size_t i = 0; i < 0x10; ++i)
    {
        sms->rmap[i + (0x10 * slot)] = sms->rom + offset + (0x400 * i);
    }
}

static void codemaster_mapper_update_slot0(struct SMS_Core* sms, const uint8_t value)
{
    codemaster_mapper_update_slot(sms, 0, value);
}

static void codemaster_mapper_update_slot1(struct SMS_Core* sms, const uint8_t value)
{
    // for codemaster games that feature on-cart ram
    // writing to ctrl-1 with bit7 set maps ram 0xA000-0xC000
    if (IS_BIT_SET(value, 7))
    {
        for (size_t i = 0; i < 0x8; ++i)
        {
            sms->rmap[i + 0x28] = cart_ram + (0x400 * i);
            sms->wmap[i + 0x28] = cart_ram + (0x400 * i);
        }

        sms->cart.mappers.codemasters.ram_mapped = true;
        return;
    }
    // was ram previously mapped?
    else if (sms->cart.mappers.codemasters.ram_mapped)
    {
        sms->cart.mappers.codemasters.ram_mapped = false;

        // unmap ram and re-map the rom
        for (size_t i = 0; i < 0x8; ++i)
        {
            sms->wmap[i + 0x28] = UNUSED_BANK;
        }

        codemaster_mapper_update_slot(sms, 2, sms->cart.mappers.codemasters.slot[2]);
    }

    codemaster_mapper_update_slot(sms, 1, value);
}

static void codemaster_mapper_update_slot2(struct SMS_Core* sms, const uint8_t value)
{
    // NOTE: if ram is mapped (ernie elfs golf), then does writes to this
    // reg remap rom? or are they ignored?
    if (sms->cart.mappers.codemasters.ram_mapped)
    {
        sms->cart.mappers.codemasters.slot[2] = value & sms->cart.max_bank_mask;
        const size_t offset = 0x4000 * sms->cart.mappers.codemasters.slot[2];

        for (size_t i = 0; i < 0x8; ++i)
        {
            sms->rmap[i + 0x20] = sms->rom + offset + (0x400 * i);
        }
    }
    else
    {
        codemaster_mapper_update_slot(sms, 2, value);
    }
}

static void init_mapper_codemaster(struct SMS_Core* sms)
{
    sms->cart.mappers.codemasters.slot[0] = 0;
    sms->cart.mappers.codemasters.slot[1] = 1;
    sms->cart.mappers.codemasters.slot[2] = 2;
    sms->cart.mappers.codemasters.ram_mapped = false;
}

static void setup_mapper_codemaster(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x10; ++i)
    {
        sms->rmap[0x30 + i] = sys_ram + (0x400 * (i & 0x7));
        sms->wmap[0x30 + i] = sys_ram + (0x400 * (i & 0x7));
    }

    codemaster_mapper_update_slot0(sms, sms->cart.mappers.codemasters.slot[0]);
    codemaster_mapper_update_slot1(sms, sms->cart.mappers.codemasters.slot[1]);
    codemaster_mapper_update_slot2(sms, sms->cart.mappers.codemasters.slot[2]);
}

static void setup_mapper_dahjee_a(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x30; i++)
    {
        sms->rmap[i] = sms->rom + (0x400 * (i % sms->rom_mask));
    }

    // has 8K mapped at 0x2000-0x3FFF
    for (int i = 0; i < 0x8; i++)
    {
        sms->rmap[0x8 + i] = cart_ram + (0x400 * i);
        sms->wmap[0x8 + i] = cart_ram + (0x400 * i);
    }

    // normal 1K mapping 0xC000-0xFFFF
    for (int i = 0; i < 0x10; ++i)
    {
        sms->rmap[0x30 + i] = sys_ram;
        sms->wmap[0x30 + i] = sys_ram;
    }
}

static void setup_mapper_dahjee_b(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x30; i++)
    {
        sms->rmap[i] = sms->rom + (0x400 * (i % sms->rom_mask));
    }

    // 8K ram mapped 0xC000-0xFFFF
    for (int i = 0; i < 0x10; i++)
    {
        sms->rmap[0x30 + i] = cart_ram + (0x400 * (i&7));
        sms->wmap[0x30 + i] = cart_ram + (0x400 * (i&7));
    }
}

static void setup_mapper_castle(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x20; i++)
    {
        sms->rmap[i] = sms->rom + (0x400 * (i % sms->rom_mask));
    }

    // 8k ram mapping 0x8000-0xBFFF
    for (int i = 0; i < 0x10; i++)
    {
        sms->rmap[0x20 + i] = cart_ram + (0x400 * (i&7));
        sms->wmap[0x20 + i] = cart_ram + (0x400 * (i&7));
    }

    // normal 1k mapping 0xC000-0xFFFF
    for (int i = 0; i < 0x10; ++i)
    {
        sms->rmap[0x30 + i] = sys_ram;
        sms->wmap[0x30 + i] = sys_ram;
    }
}

static void setup_mapper_othello(struct SMS_Core* sms)
{
    for (int i = 0; i < 0x20; i++)
    {
        sms->rmap[i] = sms->rom + (0x400 * (i % sms->rom_mask));
    }

    // 2K ram mapped 0x8000-0xFFFF
    for (int i = 0; i < 0x20; i++)
    {
        sms->rmap[0x20 + i] = cart_ram + (0x400 * (i&1));
        sms->wmap[0x20 + i] = cart_ram + (0x400 * (i&1));
    }
}

void mapper_update(struct SMS_Core* sms)
{
    // sets all banks to point to unused_bank[0x400]
    setup_mapper_unused_ram(sms);

    switch (sms->cart.mapper_type)
    {
        case MAPPER_TYPE_SEGA:
            setup_mapper_sega(sms);
            break;

        case MAPPER_TYPE_CODEMASTERS:
            setup_mapper_codemaster(sms);
            break;

        case MAPPER_TYPE_NONE:
            setup_mapper_none(sms);
            break;

        case MAPPER_TYPE_DAHJEE_A:
            setup_mapper_dahjee_a(sms);
            break;

        case MAPPER_TYPE_DAHJEE_B:
            setup_mapper_dahjee_b(sms);
            break;

        case MAPPER_TYPE_THE_CASTLE:
            setup_mapper_castle(sms);
            break;

        case MAPPER_TYPE_OTHELLO:
            setup_mapper_othello(sms);
            break;
    }

    // map the bios is enabled (if we have it)
    if (!sms->memory_control.bios_rom_disable)
    {
        //assert(SMS_has_bios(sms) && "bios was mapped, but we dont have it!");

        if (SMS_has_bios(sms))
        {
            // usually 8 (8kib)
            const size_t map_max = sms->bios_size / 0x400;

            for (size_t i = 0; i < map_max; ++i)
            {
                sms->rmap[i] = sms->bios + (0x400 * i);
            }

            // not really needed, but just in case
            for (size_t i = map_max; i < 0x10; ++i)
            {
                sms->rmap[i] = sms->rom + (0x400 * i);
            }
        }
    }
}

void mapper_init(struct SMS_Core* sms)
{
    switch (sms->cart.mapper_type)
    {
        case MAPPER_TYPE_SEGA:
            init_mapper_sega(sms);
            break;

        case MAPPER_TYPE_CODEMASTERS:
            init_mapper_codemaster(sms);
            break;

        case MAPPER_TYPE_NONE:
        case MAPPER_TYPE_DAHJEE_A:
        case MAPPER_TYPE_DAHJEE_B:
        case MAPPER_TYPE_THE_CASTLE:
        case MAPPER_TYPE_OTHELLO:
            break;
    }

    mapper_update(sms);
}

static void sega_mapper_fffx_write(struct SMS_Core* sms, const uint16_t addr, const uint8_t value)
{
    //assert(sms->cart.mapper_type == MAPPER_TYPE_SEGA && "wrong mapper, how did we get here?!?");

    switch (addr)
    {
        case 0xFFFC: // Cartridge RAM mapper control
            // TODO: mapping at 0xC000
            sms->cart.mappers.sega.fffc.rom_write_enable = IS_BIT_SET(value, 7);
            sms->cart.mappers.sega.fffc.ram_enable_c0000 = IS_BIT_SET(value, 4);
            sms->cart.mappers.sega.fffc.ram_enable_80000 = IS_BIT_SET(value, 3);
            sms->cart.mappers.sega.fffc.ram_bank_select = IS_BIT_SET(value, 2);
            sms->cart.mappers.sega.fffc.bank_shift = value & 0x3;

            // //assert(!sms->cart.mappers.sega.fffc.rom_write_enable && "rom write enable!");
            //assert(!sms->cart.mappers.sega.fffc.ram_enable_c0000 && "unimp ram_enable_c0000");
            //assert(!sms->cart.mappers.sega.fffc.bank_shift && "unimp bank_shift");

            if (sms->cart.mappers.sega.fffc.ram_enable_80000)
            {
                sega_mapper_update_ram0(sms);
                //SMS_log("game is mapping sram to rom region 0x8000!\n");
            }
            else
            {
                // unamp ram
                sega_mapper_update_slot2(sms);
            }
            break;

        case 0xFFFD: // Mapper slot 0 control
            sms->cart.mappers.sega.fffd = value & sms->cart.max_bank_mask;
            sega_mapper_update_slot0(sms);
            break;

        case 0xFFFE: // Mapper slot 1 control
            sms->cart.mappers.sega.fffe = value & sms->cart.max_bank_mask;
            sega_mapper_update_slot1(sms);
            break;

        case 0xFFFF: // Mapper slot 2 control
            sms->cart.mappers.sega.ffff = value & sms->cart.max_bank_mask;
            if (sms->cart.mappers.sega.fffc.ram_enable_80000 == false)
            {
                sega_mapper_update_slot2(sms);
            }
            break;
    }
}

uint8_t SMS_read8(struct SMS_Core* sms, const uint16_t addr)
{	
    //assert(sms->rmap[addr >> 10] && "NULL ptr in rmap!");
    return sms->rmap[addr >> 10][addr & 0x3FF];
}

void SMS_write8(struct SMS_Core* sms, const uint16_t addr, const uint8_t value)
{
    // specific mapper writes
    switch (sms->cart.mapper_type)
    {
        case MAPPER_TYPE_SEGA:
            if (addr >= 0xFFFC)
            {
                sega_mapper_fffx_write(sms, addr, value);
            }
            break;

        // control regs are not mirrored to ram (written values cannot be read back)
        case MAPPER_TYPE_CODEMASTERS:
            if (addr <= 0x3FFF)
            {
                //assert(!(addr & 0xFFF) && "codemaster ctrl mirror used!");
                codemaster_mapper_update_slot0(sms, value);
                return;
            }
            else if (addr >= 0x4000 && addr <= 0x7FFF)
            {
                //assert(!(addr & 0xFFF) && "codemaster ctrl mirror used!");
                codemaster_mapper_update_slot1(sms, value);
                return;
            }
            else if (addr >= 0x8000 && addr <= 0xBFFF)
            {
                if (!sms->cart.mappers.codemasters.ram_mapped || addr <= 0x9FFF)
                {
                    //assert(!(addr & 0xFFF) && "codemaster ctrl mirror used!");
                    codemaster_mapper_update_slot2(sms, value);
                    return;
                }
            }
            break;

        case MAPPER_TYPE_NONE:
        case MAPPER_TYPE_DAHJEE_A:
        case MAPPER_TYPE_DAHJEE_B:
        case MAPPER_TYPE_THE_CASTLE:
        case MAPPER_TYPE_OTHELLO:
            break;
    }

    //assert(sms->wmap[addr >> 10] && "NULL ptr in wmap!");
    sms->wmap[addr >> 10][addr & 0x3FF] = value;
}

uint16_t SMS_read16(struct SMS_Core* sms, const uint16_t addr)
{
    const uint16_t lo = SMS_read8(sms, addr + 0);
    const uint16_t hi = SMS_read8(sms, addr + 1);

    return (hi << 8) | lo;
}

void SMS_write16(struct SMS_Core* sms, const uint16_t addr, const uint16_t value)
{
    SMS_write8(sms, addr + 0, value & 0xFF);
    SMS_write8(sms, addr + 1, value >> 8);
}

// https://www.smspower.org/Development/Port3E
static void IO_memory_control_write(struct SMS_Core* sms, const uint8_t value)
{
    const struct SMS_MemoryControlRegister old = sms->memory_control;

    sms->memory_control.exp_slot_disable  = IS_BIT_SET(value, 7);
    sms->memory_control.cart_slot_disable = IS_BIT_SET(value, 6);
    sms->memory_control.card_slot_disable = IS_BIT_SET(value, 5);
    sms->memory_control.work_ram_disable  = IS_BIT_SET(value, 4);
    sms->memory_control.bios_rom_disable  = IS_BIT_SET(value, 3);
    sms->memory_control.io_chip_disable   = IS_BIT_SET(value, 2);

    // bios either unmapped itself (likely) or got re-mapped (impossible)
    if (SMS_has_bios(sms) && old.bios_rom_disable != sms->memory_control.bios_rom_disable)
    {
        mapper_update(sms);
    }

    //SMS_log("[memory_control]\n");
    //SMS_log("\tmemory_control.exp_slot_disable: %u\n", sms->memory_control.exp_slot_disable);
    //SMS_log("\tmemory_control.cart_slot_disable: %u\n", sms->memory_control.cart_slot_disable);
    //SMS_log("\tmemory_control.card_slot_disable: %u\n", sms->memory_control.card_slot_disable);
    //SMS_log("\tmemory_control.work_ram_disable: %u\n", sms->memory_control.work_ram_disable);
    //SMS_log("\tmemory_control.bios_rom_disable: %u\n", sms->memory_control.bios_rom_disable);
    //SMS_log("\tmemory_control.io_chip_disable: %u\n", sms->memory_control.io_chip_disable);
    // //assert(sms->memory_control.work_ram_disable == 0 && "wram disabled!");
}

// todo: PORT 0x3F
static void IO_control_write(struct SMS_Core* sms, const uint8_t value)
{
    /*
    bit 0: controller 1 button 2 direction (1=input 0=output)
    bit 1: controller 1 TH direction (1=input 0=output)
    bit 2: controller 2 button 2 direction (1=input 0=output)
    bit 3: controller 2 TH direction (1=input 0=output)

    bit 4: controller 1 button 2 output level (1=high 0=low)
    bit 5: controller 1 TH output level (1=high 0=low*)
    bit 6: controller 2 button 2 output level (1=high 0=low)
    bit 7: controller 2 TH output level (1=high 0=low*)
    */
    // 0, 2, 4, 5, 6, 7
    // 0, 2, 4, 6
    (void)sms; (void)value;
    //SMS_log("IO_control_write: 0x%02X\n", value);
}

static uint8_t IO_read_vcounter(const struct SMS_Core* sms)
{
    return sms->vdp.vcount_port;
}

static uint8_t IO_read_hcounter(const struct SMS_Core* sms)
{
    // docs say that this is a 9-bit counter, but only upper 8-bits read
    return (uint16_t)((sms->vdp.cycles * 3) / 2) >> 1;
}

static uint8_t IO_vdp_status_read(struct SMS_Core* sms)
{
    sms->vdp.control_latch = false;

    return vdp_status_flag_read(sms);
}

static uint8_t IO_vdp_data_read(struct SMS_Core* sms)
{
    sms->vdp.control_latch = false;

    const uint8_t data = sms->vdp.buffer_read_data;

    sms->vdp.buffer_read_data = sms->vdp.vram[sms->vdp.addr];
    sms->vdp.addr = (sms->vdp.addr + 1) & 0x3FFF;

    return data;
}

static void IO_vdp_cram_gg_write(struct SMS_Core* sms, const uint8_t value)
{
    // even addr stores byte to latch, odd writes 2 bytes
    if (sms->vdp.addr & 1)
    {
        const uint8_t rg_index = (sms->vdp.addr - 1) & 0x3F;
        const uint8_t b_index = sms->vdp.addr & 0x3F;

        // check is the colour has changed, if so, set dirty
        if (sms->vdp.cram[rg_index] != sms->vdp.cram_gg_latch || sms->vdp.cram[b_index] != value)
        {
            sms->vdp.dirty_cram_min = SMS_MIN(sms->vdp.dirty_cram_min, rg_index);
            sms->vdp.dirty_cram_max = SMS_MAX(sms->vdp.dirty_cram_max, rg_index+1);
            sms->vdp.dirty_cram[rg_index] = true;
        }

        sms->vdp.cram[rg_index] = sms->vdp.cram_gg_latch;
        sms->vdp.cram[b_index] = value;
    }
    else
    {
        // latches the r,g values
        sms->vdp.cram_gg_latch = value;
    }
}

static void IO_vdp_cram_sms_write(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t index = sms->vdp.addr & 0x1F;

    if (sms->vdp.cram[index] != value)
    {
        sms->vdp.cram[index] = value;
        sms->vdp.dirty_cram[index] = true;
        sms->vdp.dirty_cram_min = SMS_MIN(sms->vdp.dirty_cram_min, index);
        sms->vdp.dirty_cram_max = SMS_MAX(sms->vdp.dirty_cram_max, index+1);
    }
}

static void IO_vdp_data_write(struct SMS_Core* sms, const uint8_t value)
{
    sms->vdp.control_latch = false;
    // writes store the new value in the buffered_data
    sms->vdp.buffer_read_data = value;

    switch (sms->vdp.code)
    {
        case VDP_CODE_VRAM_WRITE_LOAD:
        case VDP_CODE_VRAM_WRITE:
        case VDP_CODE_REG_WRITE:
            // mark entry as dirty if modified
            sms->vdp.dirty_vram[sms->vdp.addr >> 2] |= sms->vdp.vram[sms->vdp.addr] != value;
            sms->vdp.vram[sms->vdp.addr] = value;
            sms->vdp.addr = (sms->vdp.addr + 1) & 0x3FFF;
            break;

        case VDP_CODE_CRAM_WRITE:
            switch (SMS_get_system_type(sms))
            {
                case SMS_System_SMS:
                    IO_vdp_cram_sms_write(sms, value);
                    break;

                case SMS_System_GG:
                    IO_vdp_cram_gg_write(sms, value);
                    break;

                case SMS_System_SG1000:
                    //assert(!"sg1000 cram write, check what should happen here!");
                    break;
            }

            sms->vdp.addr = (sms->vdp.addr + 1) & 0x3FFF;
            break;
    }
}

static void IO_vdp_control_write(struct SMS_Core* sms, const uint8_t value)
{
    if (sms->vdp.control_latch)
    {
        sms->vdp.control_word = (sms->vdp.control_word & 0xFF) | (value << 8);
        sms->vdp.code = (value >> 6) & 3;
        sms->vdp.control_latch = false;

        switch (sms->vdp.code)
        {
            // code0 immediatley loads a byte from vram into buffer
            case VDP_CODE_VRAM_WRITE_LOAD:
                sms->vdp.addr = sms->vdp.control_word & 0x3FFF;
                sms->vdp.buffer_read_data = sms->vdp.vram[sms->vdp.addr];
                sms->vdp.addr = (sms->vdp.addr + 1) & 0x3FFF;
                break;

            case VDP_CODE_VRAM_WRITE:
                sms->vdp.addr = sms->vdp.control_word & 0x3FFF;
                break;

            case VDP_CODE_REG_WRITE:
                vdp_io_write(sms, value & 0xF, sms->vdp.control_word & 0xFF);
                break;

            case VDP_CODE_CRAM_WRITE:
                sms->vdp.addr = sms->vdp.control_word & 0x3FFF;
                break;
        }
    }
    else
    {
        sms->vdp.addr = (sms->vdp.addr & 0x3F00) | value;
        sms->vdp.control_word = value;
        sms->vdp.control_latch = true;
    }
}

static uint8_t IO_gamegear_read(const struct SMS_Core* sms, const uint8_t addr)
{
    switch (addr & 0x7)
    {
        case 0x0: return sms->port.gg_regs[0x0] | 0x1F;
        case 0x1: return /* 0x7F; */ sms->port.gg_regs[0x1];
        case 0x2: return /* 0xFF; */ sms->port.gg_regs[0x2];
        case 0x3: return /* 0x00; */ sms->port.gg_regs[0x3];
        case 0x4: return /* 0xFF; */ sms->port.gg_regs[0x4];
        case 0x5: return /* 0x00; */ sms->port.gg_regs[0x5];
    }

    UNREACHABLE(0xFF);
}

static void IO_gamegear_write(struct SMS_Core* sms, const uint8_t addr, const uint8_t value)
{
    switch (addr & 0x7)
    {
        case 0x1: sms->port.gg_regs[0x1] = value; break;
        case 0x2: sms->port.gg_regs[0x2] = value; break;
        case 0x3: sms->port.gg_regs[0x3] = value; break;
        case 0x4: break;
        case 0x5: sms->port.gg_regs[0x5] = value; break;

        case 0x6:
            // right side of channels
            sms->psg.channel_enable[0][1] = IS_BIT_SET(value, 0);
            sms->psg.channel_enable[1][1] = IS_BIT_SET(value, 1);
            sms->psg.channel_enable[2][1] = IS_BIT_SET(value, 2);
            sms->psg.channel_enable[3][1] = IS_BIT_SET(value, 3);
            // left side of channels
            sms->psg.channel_enable[0][0] = IS_BIT_SET(value, 4);
            sms->psg.channel_enable[1][0] = IS_BIT_SET(value, 5);
            sms->psg.channel_enable[2][0] = IS_BIT_SET(value, 6);
            sms->psg.channel_enable[3][0] = IS_BIT_SET(value, 7);
            break;
    }
}

uint8_t SMS_read_io(struct SMS_Core* sms, const uint8_t addr)
{
    switch (addr)
    {
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05:
            if (SMS_is_system_type_gg(sms))
            {
                return IO_gamegear_read(sms, addr);
            }
            else
            {
                //assert(!"reading from gg port in non gg mode, what should happen here?");
                return 0xFF;
            }

        case 0x06: case 0x07:
        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
        case 0x18: case 0x19: case 0x1A: case 0x1B:
        case 0x1C: case 0x1D: case 0x1E: case 0x1F:
        case 0x20: case 0x21: case 0x22: case 0x23:
        case 0x24: case 0x25: case 0x26: case 0x27:
        case 0x28: case 0x29: case 0x2A: case 0x2B:
        case 0x2C: case 0x2D: case 0x2E: case 0x2F:
        case 0x30: case 0x31: case 0x32: case 0x33:
        case 0x34: case 0x35: case 0x36: case 0x37:
        case 0x38: case 0x39: case 0x3A: case 0x3B:
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            // note: ristar (GG) reads from port $22 for some reason...
            //SMS_log("[PORT-READ] 0x%02X last byte of the instruction\n", addr);
            return 0xFF; // todo:

        case 0x40: case 0x42: case 0x44: case 0x46:
        case 0x48: case 0x4A: case 0x4C: case 0x4E:
        case 0x50: case 0x52: case 0x54: case 0x56:
        case 0x58: case 0x5A: case 0x5C: case 0x5E:
        case 0x60: case 0x62: case 0x64: case 0x66:
        case 0x68: case 0x6A: case 0x6C: case 0x6E:
        case 0x70: case 0x72: case 0x74: case 0x76:
        case 0x78: case 0x7A: case 0x7C: case 0x7E:
            return IO_read_vcounter(sms);

        case 0x41: case 0x43: case 0x45: case 0x47:
        case 0x49: case 0x4B: case 0x4D: case 0x4F:
        case 0x51: case 0x53: case 0x55: case 0x57:
        case 0x59: case 0x5B: case 0x5D: case 0x5F:
        case 0x61: case 0x63: case 0x65: case 0x67:
        case 0x69: case 0x6B: case 0x6D: case 0x6F:
        case 0x71: case 0x73: case 0x75: case 0x77:
        case 0x79: case 0x7B: case 0x7D: case 0x7F:
            return IO_read_hcounter(sms);

        case 0x80: case 0x82: case 0x84: case 0x86:
        case 0x88: case 0x8A: case 0x8C: case 0x8E:
        case 0x90: case 0x92: case 0x94: case 0x96:
        case 0x98: case 0x9A: case 0x9C: case 0x9E:
        case 0xA0: case 0xA2: case 0xA4: case 0xA6:
        case 0xA8: case 0xAA: case 0xAC: case 0xAE:
        case 0xB0: case 0xB2: case 0xB4: case 0xB6:
        case 0xB8: case 0xBA: case 0xBC: case 0xBE:
            return IO_vdp_data_read(sms);

        case 0x81: case 0x83: case 0x85: case 0x87:
        case 0x89: case 0x8B: case 0x8D: case 0x8F:
        case 0x91: case 0x93: case 0x95: case 0x97:
        case 0x99: case 0x9B: case 0x9D: case 0x9F:
        case 0xA1: case 0xA3: case 0xA5: case 0xA7:
        case 0xA9: case 0xAB: case 0xAD: case 0xAF:
        case 0xB1: case 0xB3: case 0xB5: case 0xB7:
        case 0xB9: case 0xBB: case 0xBD: case 0xBF:
            return IO_vdp_status_read(sms);

        case 0xC0: case 0xC2: case 0xC4: case 0xC6:
        case 0xC8: case 0xCA: case 0xCC: case 0xCE:
        case 0xD0: case 0xD2: case 0xD4: case 0xD6:
        case 0xD8: case 0xDA: case 0xDC: case 0xDE:
        case 0xE0: case 0xE2: case 0xE4: case 0xE6:
        case 0xE8: case 0xEA: case 0xEC: case 0xEE:
        case 0xF0: case 0xF2: case 0xF4: case 0xF6:
        case 0xF8: case 0xFA: case 0xFC: case 0xFE:
            return sms->port.a;

        case 0xC1: case 0xC3: case 0xC5: case 0xC7:
        case 0xC9: case 0xCB: case 0xCD: case 0xCF:
        case 0xD1: case 0xD3: case 0xD5: case 0xD7:
        case 0xD9: case 0xDB: case 0xDD: case 0xDF:
        case 0xE1: case 0xE3: case 0xE5: case 0xE7:
        case 0xE9: case 0xEB: case 0xED: case 0xEF:
        case 0xF1: case 0xF3: case 0xF5: case 0xF7:
        case 0xF9: case 0xFB: case 0xFD: case 0xFF:
            return sms->port.b;
    }

    UNREACHABLE(0xFF);
}

void SMS_write_io(struct SMS_Core* sms, const uint8_t addr, const uint8_t value)
{
    switch (addr)
    {
        // GG regs
        case 0x00: case 0x01: case 0x02: case 0x03:
        case 0x04: case 0x05: case 0x06:
            if (SMS_is_system_type_gg(sms))
            {
                IO_gamegear_write(sms, addr, value);
            }
            else
            {
                if (addr & 1) // odd / even split
                {
                    IO_control_write(sms, value);
                }
                else
                {
                    IO_memory_control_write(sms, value);
                }
            }
            break;

        case 0x08: case 0x0A: case 0x0C: case 0x0E:
        case 0x10: case 0x12: case 0x14: case 0x16:
        case 0x18: case 0x1A: case 0x1C: case 0x1E:
        case 0x20: case 0x22: case 0x24: case 0x26:
        case 0x28: case 0x2A: case 0x2C: case 0x2E:
        case 0x30: case 0x32: case 0x34: case 0x36:
        case 0x38: case 0x3A: case 0x3C: case 0x3E:
            IO_memory_control_write(sms, value);
            break;

        case 0x07:
        case 0x09: case 0x0B: case 0x0D: case 0x0F:
        case 0x11: case 0x13: case 0x15: case 0x17:
        case 0x19: case 0x1B: case 0x1D: case 0x1F:
        case 0x21: case 0x23: case 0x25: case 0x27:
        case 0x29: case 0x2B: case 0x2D: case 0x2F:
        case 0x31: case 0x33: case 0x35: case 0x37:
        case 0x39: case 0x3B: case 0x3D: case 0x3F:
            IO_control_write(sms, value);
            break;

        case 0x40: case 0x41: case 0x42: case 0x43:
        case 0x44: case 0x45: case 0x46: case 0x47:
        case 0x48: case 0x49: case 0x4A: case 0x4B:
        case 0x4C: case 0x4D: case 0x4E: case 0x4F:
        case 0x50: case 0x51: case 0x52: case 0x53:
        case 0x54: case 0x55: case 0x56: case 0x57:
        case 0x58: case 0x59: case 0x5A: case 0x5B:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        case 0x60: case 0x61: case 0x62: case 0x63:
        case 0x64: case 0x65: case 0x66: case 0x67:
        case 0x68: case 0x69: case 0x6A: case 0x6B:
        case 0x6C: case 0x6D: case 0x6E: case 0x6F:
        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x76: case 0x77:
        case 0x78: case 0x79: case 0x7A: case 0x7B:
        case 0x7C: case 0x7D: case 0x7E: case 0x7F:
            psg_reg_write(sms, value);
            break;

        case 0x80: case 0x82: case 0x84: case 0x86:
        case 0x88: case 0x8A: case 0x8C: case 0x8E:
        case 0x90: case 0x92: case 0x94: case 0x96:
        case 0x98: case 0x9A: case 0x9C: case 0x9E:
        case 0xA0: case 0xA2: case 0xA4: case 0xA6:
        case 0xA8: case 0xAA: case 0xAC: case 0xAE:
        case 0xB0: case 0xB2: case 0xB4: case 0xB6:
        case 0xB8: case 0xBA: case 0xBC: case 0xBE:
            IO_vdp_data_write(sms, value);
            break;

        case 0x81: case 0x83: case 0x85: case 0x87:
        case 0x89: case 0x8B: case 0x8D: case 0x8F:
        case 0x91: case 0x93: case 0x95: case 0x97:
        case 0x99: case 0x9B: case 0x9D: case 0x9F:
        case 0xA1: case 0xA3: case 0xA5: case 0xA7:
        case 0xA9: case 0xAB: case 0xAD: case 0xAF:
        case 0xB1: case 0xB3: case 0xB5: case 0xB7:
        case 0xB9: case 0xBB: case 0xBD: case 0xBF:
            IO_vdp_control_write(sms, value);
            break;
    }
}


// *** sms_z80.c ***


// cycles tables taken from the code linked below (thanks!)
// SOURCE: https://github.com/superzazu/z80
static const uint8_t CYC_00[0x100] =
{
    0x04, 0x0A, 0x07, 0x06, 0x04, 0x04, 0x07, 0x04, 0x04, 0x0B, 0x07, 0x06, 0x04, 0x04, 0x07, 0x04,
    0x08, 0x0A, 0x07, 0x06, 0x04, 0x04, 0x07, 0x04, 0x0C, 0x0B, 0x07, 0x06, 0x04, 0x04, 0x07, 0x04,
    0x07, 0x0A, 0x10, 0x06, 0x04, 0x04, 0x07, 0x04, 0x07, 0x0B, 0x10, 0x06, 0x04, 0x04, 0x07, 0x04,
    0x07, 0x0A, 0x0D, 0x06, 0x0B, 0x0B, 0x0A, 0x04, 0x07, 0x0B, 0x0D, 0x06, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x07, 0x04,
    0x05, 0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x07, 0x0B, 0x05, 0x0A, 0x0A, 0x00, 0x0A, 0x11, 0x07, 0x0B,
    0x05, 0x0A, 0x0A, 0x0B, 0x0A, 0x0B, 0x07, 0x0B, 0x05, 0x04, 0x0A, 0x0B, 0x0A, 0x00, 0x07, 0x0B,
    0x05, 0x0A, 0x0A, 0x13, 0x0A, 0x0B, 0x07, 0x0B, 0x05, 0x04, 0x0A, 0x04, 0x0A, 0x00, 0x07, 0x0B,
    0x05, 0x0A, 0x0A, 0x04, 0x0A, 0x0B, 0x07, 0x0B, 0x05, 0x06, 0x0A, 0x04, 0x0A, 0x00, 0x07, 0x0B,
};

static const uint8_t CYC_ED[0x100] =
{
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x09, 0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x09,
    0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x09, 0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x09,
    0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x12, 0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x12,
    0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x08, 0x0C, 0x0C, 0x0F, 0x14, 0x08, 0x0E, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08,
    0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08, 0x10, 0x10, 0x10, 0x10, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08,
};

static const uint8_t CYC_DDFD[0x100] =
{
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x0E, 0x14, 0x0A, 0x08, 0x08, 0x0B, 0x04, 0x04, 0x0F, 0x14, 0x0A, 0x08, 0x08, 0x0B, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x17, 0x17, 0x13, 0x04, 0x04, 0x0F, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x13, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x13, 0x08,
    0x13, 0x13, 0x13, 0x13, 0x13, 0x13, 0x04, 0x13, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04, 0x04, 0x04, 0x04, 0x04, 0x08, 0x08, 0x13, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x00, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x0E, 0x04, 0x17, 0x04, 0x0F, 0x04, 0x04, 0x04, 0x08, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x0A, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
};

static const uint8_t CYC_CB[0x100] =
{
    8,8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,12,8,8,8,8,8,8,8,12,8,8,
    8,8,8,8,8,12,8,8,8,8,8,8,8,12,8,8,
    8,8,8,8,8,12,8,8,8,8,8,8,8,12,8,8,
    8,8,8,8,8,12,8,8,8,8,8,8,8,12,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,8,
    8,8,8,8,8,15,8,8,8,8,8,8,8,15,8,
};

enum
{
    FLAG_C_MASK = 1 << 0, // set if result is > 255
    FLAG_N_MASK = 1 << 1, // idk, ued for daa
    FLAG_P_MASK = 1 << 2, // parity
    FLAG_V_MASK = 1 << 2, // overflow
    FLAG_B3_MASK = 1 << 3, //
    FLAG_H_MASK = 1 << 4, // set if carry from bit-3 and bit-4
    FLAG_B5_MASK = 1 << 5, //
    FLAG_Z_MASK = 1 << 6, // set if the result is 0
    FLAG_S_MASK = 1 << 7, // set to bit-7 of a result
};

#define REG_B sms->cpu.main.B
#define REG_C sms->cpu.main.C
#define REG_D sms->cpu.main.D
#define REG_E sms->cpu.main.E
#define REG_H sms->cpu.main.H
#define REG_L sms->cpu.main.L
#define REG_A sms->cpu.main.A

#define REG_B_ALT sms->cpu.alt.B
#define REG_C_ALT sms->cpu.alt.C
#define REG_D_ALT sms->cpu.alt.D
#define REG_E_ALT sms->cpu.alt.E
#define REG_H_ALT sms->cpu.alt.H
#define REG_L_ALT sms->cpu.alt.L
#define REG_A_ALT sms->cpu.alt.A

#define REG_PC sms->cpu.PC
#define REG_SP sms->cpu.SP

#define REG_IXL sms->cpu.IXL
#define REG_IXH sms->cpu.IXH
#define REG_IYL sms->cpu.IYL
#define REG_IYH sms->cpu.IYH

#define REG_I sms->cpu.I
#define REG_R sms->cpu.R

#define FLAG_C sms->cpu.main.flags.C
#define FLAG_N sms->cpu.main.flags.N
#define FLAG_P sms->cpu.main.flags.P // P/V
#define FLAG_V sms->cpu.main.flags.P // P/V
#define FLAG_B3 sms->cpu.main.flags.B3
#define FLAG_H sms->cpu.main.flags.H
#define FLAG_B5 sms->cpu.main.flags.B5
#define FLAG_Z sms->cpu.main.flags.Z
#define FLAG_S sms->cpu.main.flags.S

#define FLAG_C_ALT sms->cpu.alt.flags.C
#define FLAG_N_ALT sms->cpu.alt.flags.N
// #define FLAG_P_ALT sms->cpu.alt.flags.P // P/V
#define FLAG_V_ALT sms->cpu.alt.flags.P // P/V
#define FLAG_B3_ALT sms->cpu.alt.flags.B3
#define FLAG_H_ALT sms->cpu.alt.flags.H
#define FLAG_B5_ALT sms->cpu.alt.flags.B5
#define FLAG_Z_ALT sms->cpu.alt.flags.Z
#define FLAG_S_ALT sms->cpu.alt.flags.S

// REG_F getter
#define REG_F_GET() \
    ((FLAG_S << 7) | (FLAG_Z << 6) | (FLAG_B5 << 5) | (FLAG_H << 4) | \
    (FLAG_B3 << 3) | (FLAG_V << 2) | (FLAG_N << 1) | (FLAG_C << 0))

#define REG_F_GET_ALT() \
    ((FLAG_S_ALT << 7) | (FLAG_Z_ALT << 6) | (FLAG_B5_ALT << 5) | (FLAG_H_ALT << 4) | \
    (FLAG_B3_ALT << 3) | (FLAG_V_ALT << 2) | (FLAG_N_ALT << 1) | (FLAG_C_ALT << 0))

// REG_F setter
#define REG_F_SET(v) \
    FLAG_S = (v) & FLAG_S_MASK; \
    FLAG_Z = (v) & FLAG_Z_MASK; \
    FLAG_B5 = (v) & FLAG_B5_MASK; \
    FLAG_H = (v) & FLAG_H_MASK; \
    FLAG_B3 = (v) & FLAG_B3_MASK; \
    FLAG_V = (v) & FLAG_V_MASK; \
    FLAG_N = (v) & FLAG_N_MASK; \
    FLAG_C = (v) & FLAG_C_MASK

// REG_F setter
#define REG_F_SET_ALT(v) \
    FLAG_S_ALT = (v) & FLAG_S_MASK; \
    FLAG_Z_ALT = (v) & FLAG_Z_MASK; \
    FLAG_B5_ALT = (v) & FLAG_B5_MASK; \
    FLAG_H_ALT = (v) & FLAG_H_MASK; \
    FLAG_B3_ALT = (v) & FLAG_B3_MASK; \
    FLAG_V_ALT = (v) & FLAG_V_MASK; \
    FLAG_N_ALT = (v) & FLAG_N_MASK; \
    FLAG_C_ALT = (v) & FLAG_C_MASK

#define PAIR(hi, lo) ((hi << 8) | (lo))
#define SET_PAIR(hi, lo, v) hi = ((v) >> 8) & 0xFF; lo = (v) & 0xFF

// getters
#define REG_BC PAIR(REG_B, REG_C)
#define REG_DE PAIR(REG_D, REG_E)
#define REG_HL PAIR(REG_H, REG_L)
#define REG_AF PAIR(REG_A, REG_F_GET())
// #define REG_IX PAIR(REG_IXH, REG_IXL)
// #define REG_IY PAIR(REG_IYH, REG_IYL)

// todo: remove
#define REG_BC_ALT PAIR(REG_B_ALT, REG_C_ALT)
#define REG_DE_ALT PAIR(REG_D_ALT, REG_E_ALT)
#define REG_HL_ALT PAIR(REG_H_ALT, REG_L_ALT)
#define REG_AF_ALT PAIR(REG_A_ALT, REG_F_GET_ALT())

// setters
#define SET_REG_BC(v) SET_PAIR(REG_B, REG_C, v)
#define SET_REG_DE(v) SET_PAIR(REG_D, REG_E, v)
#define SET_REG_HL(v) SET_PAIR(REG_H, REG_L, v)
#define SET_REG_AF(v) REG_A = (((v) >> 8) & 0xFF); REG_F_SET(v)

// TODO: remove
#define SET_REG_BC_ALT(v) SET_PAIR(REG_B_ALT, REG_C_ALT, v)
#define SET_REG_DE_ALT(v) SET_PAIR(REG_D_ALT, REG_E_ALT, v)
#define SET_REG_HL_ALT(v) SET_PAIR(REG_H_ALT, REG_L_ALT, v)
#define SET_REG_AF_ALT(v) REG_A_ALT = (((v) >> 8) & 0xFF); REG_F_SET_ALT(v)

#define read8(addr) SMS_read8(sms, addr)
#define read16(addr) SMS_read16(sms, addr)

#define write8(addr,value) SMS_write8(sms, addr, value)
#define write16(addr,value) SMS_write16(sms, addr, value)

#define readIO(addr) SMS_read_io(sms, addr)
#define writeIO(addr,value) SMS_write_io(sms, addr, value)

static FORCE_INLINE uint8_t get_r8(struct SMS_Core* sms, const uint8_t idx)
{
    switch (idx & 0x7)
    {
        case 0x0: return REG_B;
        case 0x1: return REG_C;
        case 0x2: return REG_D;
        case 0x3: return REG_E;
        case 0x4: return REG_H;
        case 0x5: return REG_L;
        case 0x6: return read8(REG_HL);
        case 0x7: return REG_A;
    }

    UNREACHABLE(0xFF);
}

static FORCE_INLINE void set_r8(struct SMS_Core* sms, const uint8_t value, const uint8_t idx)
{
    switch (idx & 0x7)
    {
        case 0x0: REG_B = value; break;
        case 0x1: REG_C = value; break;
        case 0x2: REG_D = value; break;
        case 0x3: REG_E = value; break;
        case 0x4: REG_H = value; break;
        case 0x5: REG_L = value; break;
        case 0x6: write8(REG_HL, value); break;
        case 0x7: REG_A = value; break;
    }
}

static FORCE_INLINE uint16_t get_r16(struct SMS_Core* sms, const uint8_t idx)
{
    switch (idx & 0x3)
    {
        case 0x0: return REG_BC;
        case 0x1: return REG_DE;
        case 0x2: return REG_HL;
        case 0x3: return REG_SP;
    }

    UNREACHABLE(0xFF);
}

static FORCE_INLINE void set_r16(struct SMS_Core* sms, uint16_t value, const uint8_t idx)
{
    switch (idx & 0x3)
    {
        case 0x0: SET_REG_BC(value); break;
        case 0x1: SET_REG_DE(value); break;
        case 0x2: SET_REG_HL(value); break;
        case 0x3: REG_SP = value; break;
    }
}

static FORCE_INLINE bool calc_vflag_8(const uint8_t a, const uint8_t b, const uint8_t r)
{
    return ((a & 0x80) == (b & 0x80)) && ((a & 0x80) != (r & 0x80));
}

static FORCE_INLINE bool calc_vflag_16(const uint8_t a, const uint8_t b, const uint8_t r)
{
    return ((a & 0x8000) == (b & 0x8000)) && ((a & 0x8000) != (r & 0x8000));
}

static FORCE_INLINE void _ADD(struct SMS_Core* sms, uint16_t value)
{
    const uint8_t result = REG_A + value;

    FLAG_C = (REG_A + value) > 0xFF;
    FLAG_N = false;
    FLAG_V = calc_vflag_8(REG_A, value, result);
    FLAG_H = ((REG_A & 0xF) + (value & 0xF)) > 0xF;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    REG_A = result;
}

static FORCE_INLINE void _SUB(struct SMS_Core* sms, uint16_t value)
{
    const uint8_t result = REG_A - value;

    FLAG_C = value > REG_A;
    FLAG_N = true;
    FLAG_V = calc_vflag_8(REG_A, ~value + 1, result);
    FLAG_H = (REG_A & 0xF) < (value & 0xF);
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    REG_A = result;
}

static FORCE_INLINE void _AND(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = REG_A & value;

    FLAG_C = false;
    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = true;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    REG_A = result;
}

static FORCE_INLINE void _XOR(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = REG_A ^ value;

    FLAG_C = false;
    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    REG_A = result;
}

static FORCE_INLINE void _OR(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = REG_A | value;

    FLAG_C = false;
    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    REG_A = result;
}

static FORCE_INLINE void _CP(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = REG_A - value;

    FLAG_C = value > REG_A;
    FLAG_N = true;
    FLAG_V = calc_vflag_8(REG_A, ~value + 1, result);
    FLAG_H = (REG_A & 0xF) < (value & 0xF);
    FLAG_B3 = IS_BIT_SET(value, 3);
    FLAG_B5 = IS_BIT_SET(value, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;
}

static FORCE_INLINE void _ADC(struct SMS_Core* sms, const uint8_t value)
{
    _ADD(sms, value + FLAG_C);
}

static FORCE_INLINE void _SBC(struct SMS_Core* sms, const uint8_t value)
{
    _SUB(sms, value + FLAG_C);
}

static FORCE_INLINE void AND_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _AND(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void XOR_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _XOR(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void OR_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _OR(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void CP_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _CP(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void ADD_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _ADD(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void SUB_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _SUB(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void ADC_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _ADC(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void SBC_r(struct SMS_Core* sms, const uint8_t opcode)
{
    _SBC(sms, get_r8(sms, opcode));
}

static FORCE_INLINE void AND_imm(struct SMS_Core* sms)
{
    _AND(sms, read8(REG_PC++));
}

static FORCE_INLINE void XOR_imm(struct SMS_Core* sms)
{
    _XOR(sms, read8(REG_PC++));
}

static FORCE_INLINE void OR_imm(struct SMS_Core* sms)
{
    _OR(sms, read8(REG_PC++));
}

static FORCE_INLINE void CP_imm(struct SMS_Core* sms)
{
    const uint8_t b = read8(REG_PC++);
    _CP(sms, b);
}

static FORCE_INLINE void ADD_imm(struct SMS_Core* sms)
{
    _ADD(sms, read8(REG_PC++));
}

static FORCE_INLINE void SUB_imm(struct SMS_Core* sms)
{
    _SUB(sms, read8(REG_PC++));
}

static FORCE_INLINE void ADC_imm(struct SMS_Core* sms)
{
    _ADC(sms, read8(REG_PC++));
}

static FORCE_INLINE void SBC_imm(struct SMS_Core* sms)
{
    _SBC(sms, read8(REG_PC++));
}

static FORCE_INLINE void NEG(struct SMS_Core* sms)
{
    _SUB(sms, REG_A * 2); // 1 - (1*2) = 0xFF
}

static FORCE_INLINE uint16_t ADD_DD_16(struct SMS_Core* sms, uint16_t a, uint16_t b)
{
    const uint16_t result = a + b;

    FLAG_C = (a + b) > 0xFFFF;
    FLAG_N = false;
    FLAG_H = ((a & 0xFFF) + (b & 0xFFF)) > 0xFFF;
    FLAG_B3 = IS_BIT_SET(result >> 8, 3);
    FLAG_B5 = IS_BIT_SET(result >> 8, 5);

    return result;
}

static FORCE_INLINE void ADC_hl(struct SMS_Core* sms, uint32_t value)
{
    value += FLAG_C;

    const uint16_t hl = REG_HL;
    const uint16_t result = hl + value;

    FLAG_C = (value + hl) > 0xFFFF;
    FLAG_N = false;
    FLAG_V = calc_vflag_16(hl, value, result);
    FLAG_H = ((hl & 0xFFF) + (value & 0xFFF)) > 0xFFF;
    FLAG_B3 = IS_BIT_SET(result >> 8, 3);
    FLAG_B5 = IS_BIT_SET(result >> 8, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 15;

    SET_REG_HL(result);
}

static FORCE_INLINE void SBC_hl(struct SMS_Core* sms, uint32_t value)
{
    value += FLAG_C;

    const uint16_t hl = REG_HL;
    const uint16_t result = hl - value;

    FLAG_C = value > hl;
    FLAG_N = true;
    FLAG_V = calc_vflag_16(hl, ~value + 1, result);
    FLAG_H = (hl & 0xFFF) < (value & 0xFFF);
    FLAG_B3 = IS_BIT_SET(result >> 8, 3);
    FLAG_B5 = IS_BIT_SET(result >> 8, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 15;


    SET_REG_HL(result);
}

static FORCE_INLINE void PUSH(struct SMS_Core* sms, uint16_t value)
{
    write8(--REG_SP, (value >> 8) & 0xFF);
    write8(--REG_SP, value & 0xFF);
}

static FORCE_INLINE uint16_t POP(struct SMS_Core* sms)
{
    const uint16_t result = read16(REG_SP);
    REG_SP += 2;
    return result;
}

static FORCE_INLINE void POP_BC(struct SMS_Core* sms)
{
    const uint16_t r = POP(sms);
    SET_REG_BC(r);
}

static FORCE_INLINE void POP_DE(struct SMS_Core* sms)
{
    const uint16_t r = POP(sms);
    SET_REG_DE(r);
}

static FORCE_INLINE void POP_HL(struct SMS_Core* sms)
{
    const uint16_t r = POP(sms);
    SET_REG_HL(r);
}

static FORCE_INLINE void POP_AF(struct SMS_Core* sms)
{
    const uint16_t r = POP(sms);
    SET_REG_AF(r);
}

static FORCE_INLINE void shift_left_flags(struct SMS_Core* sms, const uint8_t result, const uint8_t value)
{
    FLAG_C = value >> 7;
    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;
}

static FORCE_INLINE void shift_right_flags(struct SMS_Core* sms, const uint8_t result, const uint8_t value)
{
    FLAG_C = value & 0x1;
    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;
}

static FORCE_INLINE void ADD_HL(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint16_t value = get_r16(sms, opcode >> 4);
    const uint16_t HL = REG_HL;
    const uint16_t result = HL + value;

    FLAG_C = HL + value > 0xFFFF;
    FLAG_H = (HL & 0xFFF) + (value & 0xFFF) > 0xFFF;
    FLAG_N = false;
    FLAG_B3 = IS_BIT_SET(result >> 8, 3);
    FLAG_B5 = IS_BIT_SET(result >> 8, 5);

    SET_REG_HL(result);
}

static FORCE_INLINE uint8_t _INC(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = value + 1;

    FLAG_N = false;
    FLAG_V = value == 0x7F;
    FLAG_H = (value & 0xF) == 0xF;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    return result;
}

static FORCE_INLINE uint8_t _DEC(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = value - 1;

    FLAG_N = true;
    FLAG_V = value == 0x80;
    FLAG_H = (value & 0xF) == 0x0;
    FLAG_B3 = IS_BIT_SET(result, 3);
    FLAG_B5 = IS_BIT_SET(result, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;

    return result;
}

static FORCE_INLINE void INC_r8(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint8_t result = _INC(sms, get_r8(sms, opcode >> 3));
    set_r8(sms, result, opcode >> 3);
}

static FORCE_INLINE void DEC_r8(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint8_t result = _DEC(sms, get_r8(sms, opcode >> 3));
    set_r8(sms, result, opcode >> 3);
}

static FORCE_INLINE void INC_r16(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint8_t idx = opcode >> 4;
    set_r16(sms, get_r16(sms, idx) + 1, idx);
}

static FORCE_INLINE void DEC_r16(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint8_t idx = opcode >> 4;
    set_r16(sms, get_r16(sms, idx) - 1, idx);
}

static FORCE_INLINE void LD_imm_a(struct SMS_Core* sms)
{
    const uint16_t addr = read16(REG_PC);
    REG_PC += 2;

    write8(addr, REG_A);
}

static FORCE_INLINE void LD_a_imm(struct SMS_Core* sms)
{
    const uint16_t addr = read16(REG_PC);
    REG_PC += 2;

    REG_A = read8(addr);
}

static FORCE_INLINE void LD_imm_r16(struct SMS_Core* sms, uint16_t r16)
{
    const uint16_t addr = read16(REG_PC);
    REG_PC += 2;

    write16(addr, r16);
}

static FORCE_INLINE void LD_hl_imm(struct SMS_Core* sms)
{
    const uint16_t addr = read16(REG_PC);
    REG_PC += 2;

    const uint16_t r = read16(addr);
    SET_REG_HL(r);
}

static FORCE_INLINE void LD_16(struct SMS_Core* sms, const uint8_t opcode)
{
    const uint16_t value = read16(REG_PC);
    REG_PC += 2;
    set_r16(sms, value, opcode >> 4);
}

static FORCE_INLINE void LD_r_u8(struct SMS_Core* sms, const uint8_t opcode)
{
    set_r8(sms, read8(REG_PC++), opcode >> 3);
}

static FORCE_INLINE void LD_rr(struct SMS_Core* sms, const uint8_t opcode)
{
    set_r8(sms, get_r8(sms, opcode), opcode >> 3);
}

static FORCE_INLINE void LD_sp_hl(struct SMS_Core* sms)
{
    REG_SP = REG_HL;
}

static FORCE_INLINE void LD_r16_a(struct SMS_Core* sms, uint16_t r16)
{
    write8(r16, REG_A);
}

static FORCE_INLINE void LD_a_r16(struct SMS_Core* sms, uint16_t r16)
{
    REG_A = read8(r16);
}

static FORCE_INLINE void RST(struct SMS_Core* sms, uint16_t value)
{
    PUSH(sms, REG_PC);
    REG_PC = value;
}

static FORCE_INLINE void CALL(struct SMS_Core* sms)
{
    const uint16_t value = read16(REG_PC);
    PUSH(sms, REG_PC + 2);
    REG_PC = value;
}

static FORCE_INLINE void CALL_cc(struct SMS_Core* sms, bool cond)
{
    if (cond)
    {
        CALL(sms);
        sms->cpu.cycles += 7;
    }
    else
    {
        REG_PC += 2;
    }
}

static FORCE_INLINE void RET(struct SMS_Core* sms)
{
    REG_PC = POP(sms);
}

static FORCE_INLINE void RET_cc(struct SMS_Core* sms, bool cond)
{
    if (cond)
    {
        RET(sms);
        sms->cpu.cycles += 6;
    }
}

static FORCE_INLINE void RETI(struct SMS_Core* sms)
{
    REG_PC = POP(sms);
    sms->cpu.IFF1 = true;
}

static FORCE_INLINE void RETN(struct SMS_Core* sms)
{
    REG_PC = POP(sms);
    sms->cpu.IFF1 = sms->cpu.IFF2;
}

static FORCE_INLINE void JR(struct SMS_Core* sms)
{
    REG_PC += ((int8_t)read8(REG_PC)) + 1;
}

static FORCE_INLINE void DJNZ(struct SMS_Core* sms)
{
    --REG_B;

    if (REG_B)
    {
        JR(sms);
        sms->cpu.cycles += 5;
    }
    else
    {
        REG_PC += 1;
    }
}

static FORCE_INLINE void JR_cc(struct SMS_Core* sms, bool cond)
{
    if (cond)
    {
        JR(sms);
        sms->cpu.cycles += 5;
    }
    else
    {
        REG_PC += 1;
    }
}

static FORCE_INLINE void JP(struct SMS_Core* sms)
{
    REG_PC = read16(REG_PC);
}

static FORCE_INLINE void JP_cc(struct SMS_Core* sms, bool cond)
{
    if (cond)
    {
        JP(sms);
    }
    else
    {
        REG_PC += 2;
    }
}

static FORCE_INLINE void EI(struct SMS_Core* sms)
{
    sms->cpu.ei_delay = true;
}

static FORCE_INLINE void DI(struct SMS_Core* sms)
{
    sms->cpu.IFF1 = false;
    sms->cpu.IFF2 = false;
}

static FORCE_INLINE uint8_t _RL(struct SMS_Core* sms, const uint8_t value, bool carry)
{
    const uint8_t result = (value << 1) | carry;

    shift_left_flags(sms, result, value);

    return result;
}

static FORCE_INLINE uint8_t _RR(struct SMS_Core* sms, const uint8_t value, bool carry)
{
    const uint8_t result = (value >> 1) | (carry << 7);

    shift_right_flags(sms, result, value);

    return result;
}

// the accumulator shifts do not affect the [p, z, s] flags
static FORCE_INLINE void RLA(struct SMS_Core* sms)
{
    const bool p = FLAG_P;
    const bool z = FLAG_Z;
    const bool s = FLAG_S;

    REG_A = _RL(sms, REG_A, FLAG_C);

    FLAG_P = p; FLAG_Z = z; FLAG_S = s;
}

static FORCE_INLINE void RRA(struct SMS_Core* sms)
{
    const bool p = FLAG_P;
    const bool z = FLAG_Z;
    const bool s = FLAG_S;

    REG_A = _RR(sms, REG_A, FLAG_C);

    FLAG_P = p; FLAG_Z = z; FLAG_S = s;
}

static FORCE_INLINE void RLCA(struct SMS_Core* sms)
{
    const bool p = FLAG_P;
    const bool z = FLAG_Z;
    const bool s = FLAG_S;

    REG_A = _RL(sms, REG_A, REG_A >> 7);

    FLAG_P = p; FLAG_Z = z; FLAG_S = s;
}

static FORCE_INLINE void RRCA(struct SMS_Core* sms)
{
    const bool p = FLAG_P;
    const bool z = FLAG_Z;
    const bool s = FLAG_S;

    REG_A = _RR(sms, REG_A, REG_A & 1);

    FLAG_P = p; FLAG_Z = z; FLAG_S = s;
}

static FORCE_INLINE uint8_t RL(struct SMS_Core* sms, const uint8_t value)
{
    return _RL(sms, value, FLAG_C);
}

static FORCE_INLINE uint8_t RLC(struct SMS_Core* sms, const uint8_t value)
{
    return _RL(sms, value, value >> 7);
}

static FORCE_INLINE uint8_t RR(struct SMS_Core* sms, const uint8_t value)
{
    return _RR(sms, value, FLAG_C);
}

static FORCE_INLINE uint8_t RRC(struct SMS_Core* sms, const uint8_t value)
{
    return _RR(sms, value, value & 1);
}

static FORCE_INLINE uint8_t SLA(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = value << 1;

    shift_left_flags(sms, result, value);

    return result;
}

static FORCE_INLINE uint8_t SRA(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = (value >> 1) | (value & 0x80);

    shift_right_flags(sms, result, value);

    return result;
}

static FORCE_INLINE uint8_t SLL(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = (value << 1) | 0x1;

    shift_left_flags(sms, result, value);

    return result;
}

static FORCE_INLINE uint8_t SRL(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t result = value >> 1;

    shift_right_flags(sms, result, value);

    return result;
}

static FORCE_INLINE void BIT(struct SMS_Core* sms, const uint8_t value, const uint8_t bit)
{
    const uint8_t result = value & bit;

    FLAG_N = false;
    FLAG_P = result == 0; // copy of zero flag
    FLAG_H = true;
    FLAG_B3 = IS_BIT_SET(value, 3);
    FLAG_B5 = IS_BIT_SET(value, 5);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;
}

static FORCE_INLINE uint8_t RES(struct SMS_Core* sms, const uint8_t value, const uint8_t bit)
{
    UNUSED(sms);

    const uint8_t result = value & ~bit;

    return result;
}

static FORCE_INLINE uint8_t SET(struct SMS_Core* sms, const uint8_t value, const uint8_t bit)
{
    UNUSED(sms);

    const uint8_t result = value | bit;

    return result;
}

static FORCE_INLINE void IMM_set(struct SMS_Core* sms, const uint8_t mode)
{
    //assert(mode == 1 && "invalid mode set for SMS");
    UNUSED(sms); UNUSED(mode);
}

static FORCE_INLINE void IN_imm(struct SMS_Core* sms)
{
    const uint8_t port = read8(REG_PC++);
    REG_A = readIO(port);
}

static FORCE_INLINE void OUT_imm(struct SMS_Core* sms)
{
    const uint8_t port = read8(REG_PC++);
    writeIO(port, REG_A);
}

static FORCE_INLINE void EX_af_af(struct SMS_Core* sms)
{
    const uint16_t temp = REG_AF_ALT;
    SET_REG_AF_ALT(REG_AF);
    SET_REG_AF(temp);
}

static FORCE_INLINE void EXX(struct SMS_Core* sms)
{
    // BC
    uint16_t temp = REG_BC_ALT;
    SET_REG_BC_ALT(REG_BC);
    SET_REG_BC(temp);
    // DE
    temp = REG_DE_ALT;
    SET_REG_DE_ALT(REG_DE);
    SET_REG_DE(temp);
    // HL
    temp = REG_HL_ALT;
    SET_REG_HL_ALT(REG_HL);
    SET_REG_HL(temp);
}

static FORCE_INLINE void EX_de_hl(struct SMS_Core* sms)
{
    const uint16_t temp = REG_DE;
    SET_REG_DE(REG_HL);
    SET_REG_HL(temp);
}

static FORCE_INLINE void EX_sp_hl(struct SMS_Core* sms)
{
    // this swaps the value at (SP), not SP!
    const uint16_t value = read16(REG_SP);
    write16(REG_SP, REG_HL);
    SET_REG_HL(value);
}

static FORCE_INLINE void CPI_CPD(struct SMS_Core* sms, int increment)
{
    uint16_t hl = REG_HL;
    uint16_t bc = REG_BC;

    const uint8_t value = read8(hl);
    const uint8_t result = REG_A - value;

    hl += increment;
    bc--;

    FLAG_N = true;
    FLAG_H = (REG_A & 0xF) < (value & 0xF);
    FLAG_Z = result == 0;
    FLAG_S = result >> 7;
    FLAG_P = bc != 0;

    const uint8_t b35_value = REG_A - value - FLAG_H;

    FLAG_B3 = IS_BIT_SET(b35_value, 3);
    FLAG_B5 = IS_BIT_SET(b35_value, 1);

    SET_REG_BC(bc);
    SET_REG_HL(hl);
}

static FORCE_INLINE void CPI(struct SMS_Core* sms)
{
    CPI_CPD(sms, +1);
}

static FORCE_INLINE void CPIR(struct SMS_Core* sms)
{
    CPI(sms);

    if (REG_BC != 0 && FLAG_Z == false)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void CPD(struct SMS_Core* sms)
{
    CPI_CPD(sms, -1);
}

static FORCE_INLINE void CPDR(struct SMS_Core* sms)
{
    CPD(sms);

    if (REG_BC != 0 && FLAG_Z == false)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void LDI_LDD(struct SMS_Core* sms, int increment)
{
    uint16_t hl = REG_HL;
    uint16_t de = REG_DE;
    uint16_t bc = REG_BC;

    const uint8_t value = read8(hl);
    write8(de, value);

    hl += increment;
    de += increment;
    bc--;

    FLAG_H = false;
    FLAG_P = bc != 0;
    FLAG_N = false;
    FLAG_B3 = IS_BIT_SET(REG_A + value, 3);
    FLAG_B5 = IS_BIT_SET(REG_A + value, 1);

    SET_REG_BC(bc);
    SET_REG_DE(de);
    SET_REG_HL(hl);
}

static FORCE_INLINE void LDI(struct SMS_Core* sms)
{
    LDI_LDD(sms, +1);
}

static FORCE_INLINE void LDIR(struct SMS_Core* sms)
{
    LDI(sms);

    if (REG_BC != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void LDD(struct SMS_Core* sms)
{
    LDI_LDD(sms, -1);
}

static FORCE_INLINE void LDDR(struct SMS_Core* sms)
{
    LDD(sms);

    if (REG_BC != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void INI_IND(struct SMS_Core* sms, int increment)
{
    uint16_t hl = REG_HL;

    const uint8_t value = readIO(REG_C);
    write8(hl, value);

    hl += increment;
    REG_B--;

    FLAG_Z = REG_B == 0;
    FLAG_N = true;

    SET_REG_HL(hl);
}

static FORCE_INLINE void INI(struct SMS_Core* sms)
{
    INI_IND(sms, +1);
}

static FORCE_INLINE void INIR(struct SMS_Core* sms)
{
    INI(sms);

    if (REG_B != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void IND(struct SMS_Core* sms)
{
    INI_IND(sms, -1);
}

static FORCE_INLINE void INDR(struct SMS_Core* sms)
{
    IND(sms);

    if (REG_B != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void OUTI_OUTD(struct SMS_Core* sms, int increment)
{
    uint16_t hl = REG_HL;

    const uint8_t value = read8(hl);
    writeIO(REG_C, value);

    hl += increment;
    REG_B--;

    FLAG_Z = REG_B == 0;
    FLAG_N = true;

    SET_REG_HL(hl);
}

static FORCE_INLINE void OUTI(struct SMS_Core* sms)
{
    OUTI_OUTD(sms, +1);
}

static FORCE_INLINE void OTIR(struct SMS_Core* sms)
{
    OUTI(sms);

    if (REG_B != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE void OUTD(struct SMS_Core* sms)
{
    OUTI_OUTD(sms, -1);
}

static FORCE_INLINE void OTDR(struct SMS_Core* sms)
{
    OUTD(sms);

    if (REG_B != 0)
    {
        REG_PC -= 2;
        sms->cpu.cycles += 5;
    }
}

static FORCE_INLINE uint8_t IN(struct SMS_Core* sms)
{
    const uint8_t result = readIO(REG_C);

    FLAG_N = false;
    FLAG_P = SMS_parity8(result);
    FLAG_H = false;

    return result;
}

static FORCE_INLINE void OUT(struct SMS_Core* sms, const uint8_t value)
{
    writeIO(REG_C, value);
}

static FORCE_INLINE void HALT(struct SMS_Core* sms)
{
    //assert((sms->cpu.ei_delay || sms->cpu.IFF1) && "halt with interrupts disabled!");
    sms->cpu.halt = true;
}

static FORCE_INLINE void DAA(struct SMS_Core* sms)
{
    if (FLAG_N)
    {
        if (FLAG_C)
        {
            REG_A -= 0x60;
            FLAG_C = true;
        }
        if (FLAG_H)
        {
            REG_A -= 0x6;
        }
    }
    else
    {
        if (FLAG_C || REG_A > 0x99)
        {
            REG_A += 0x60;
            FLAG_C = true;
        }
        if (FLAG_H || (REG_A & 0x0F) > 0x09)
        {
            REG_A += 0x6;
        }
    }

    // TODO: check how half flag is set!!!
    FLAG_P = SMS_parity8(REG_A);
    FLAG_Z = REG_A == 0;
    FLAG_S = REG_A >> 7;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
}

static FORCE_INLINE void CCF(struct SMS_Core* sms)
{
    FLAG_H = FLAG_C;
    FLAG_C = !FLAG_C;
    FLAG_N = false;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
}

static FORCE_INLINE void SCF(struct SMS_Core* sms)
{
    FLAG_C = true;
    FLAG_H = false;
    FLAG_N = false;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
}

static FORCE_INLINE void CPL(struct SMS_Core* sms)
{
    REG_A = ~REG_A;
    FLAG_H = true;
    FLAG_N = true;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
}

static FORCE_INLINE void RRD(struct SMS_Core* sms)
{
    const uint16_t hl = REG_HL;
    const uint8_t a = REG_A;
    const uint8_t value = read8(hl);

    write8(hl, (a << 4) | (value >> 4));
    REG_A = (a & 0xF0) | (value & 0x0F);

    FLAG_N = false;
    FLAG_P = SMS_parity8(REG_A);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
    FLAG_Z = REG_A == 0;
    FLAG_S = REG_A >> 7;
}

static FORCE_INLINE void RLD(struct SMS_Core* sms)
{
    const uint16_t hl = REG_HL;
    const uint8_t a = REG_A;
    const uint8_t value = read8(hl);

    write8(hl, (a & 0x0F) | (value << 4));
    REG_A = (a & 0xF0) | (value >> 4);

    FLAG_N = false;
    FLAG_P = SMS_parity8(REG_A);
    FLAG_H = false;
    FLAG_B3 = IS_BIT_SET(REG_A, 3);
    FLAG_B5 = IS_BIT_SET(REG_A, 5);
    FLAG_Z = REG_A == 0;
    FLAG_S = REG_A >> 7;
}

static FORCE_INLINE void LD_I_A(struct SMS_Core* sms)
{
    REG_I = REG_A;
}

static FORCE_INLINE void LD_A_I(struct SMS_Core* sms)
{
    REG_A = REG_I;

    FLAG_N = false;
    FLAG_P = sms->cpu.IFF2;
    FLAG_H = false;
    FLAG_Z = REG_A == 0;
    FLAG_S = REG_A >> 7;
}

static FORCE_INLINE void LD_R_A(struct SMS_Core* sms)
{
    REG_R = REG_A;
}

static FORCE_INLINE void LD_A_R(struct SMS_Core* sms)
{
    // the refresh reg is ticked on every mem access.
    // to avoid this slight overhead, just increment the value
    // on read. this will work find as R is used as psuedo RNG anyway.
    REG_R += REG_H + REG_A + REG_C;
    REG_A = REG_R;

    FLAG_N = false;
    FLAG_P = sms->cpu.IFF2;
    FLAG_H = false;
    FLAG_Z = REG_A == 0;
    FLAG_S = REG_A >> 7;
}

static FORCE_INLINE void isr(struct SMS_Core* sms)
{
    if (sms->cpu.ei_delay)
    {
        sms->cpu.ei_delay = false;
        sms->cpu.IFF1 = true;
        sms->cpu.IFF2 = true;
        return;
    }

    if (sms->cpu.IFF1 && (sms->cpu.interrupt_requested || vdp_has_interrupt(sms)))
    {
        sms->cpu.IFF1 = false;
        sms->cpu.IFF2 = false;
        sms->cpu.interrupt_requested = false;
        sms->cpu.halt = false;
        sms->cpu.cycles += 13;

        RST(sms, 0x38);
    }
}

void z80_nmi(struct SMS_Core* sms)
{
    sms->cpu.IFF1 = false;
    sms->cpu.IFF2 = false;
    sms->cpu.halt = false;
    sms->cpu.ei_delay = false;
    sms->cpu.cycles += 11;

    RST(sms, 0x66);
}

void z80_irq(struct SMS_Core* sms)
{
    sms->cpu.interrupt_requested = true;
}

// NOTE: templates would be much nicer here
// returns true if the result needs to be written back (all except BIT)
static FORCE_INLINE bool _CB(struct SMS_Core* sms, const uint8_t opcode, const uint8_t value, uint8_t* result)
{
    switch ((opcode >> 3) & 0x1F)
    {
        case 0x00: *result = RLC(sms, value); return true;
        case 0x01: *result = RRC(sms, value); return true;
        case 0x02: *result = RL(sms, value);  return true;
        case 0x03: *result = RR(sms, value);  return true;
        case 0x04: *result = SLA(sms, value); return true;
        case 0x05: *result = SRA(sms, value); return true;
        case 0x06: *result = SLL(sms, value); return true;
        case 0x07: *result = SRL(sms, value); return true;

        case 0x08: case 0x09: case 0x0A: case 0x0B:
        case 0x0C: case 0x0D: case 0x0E: case 0x0F:
            BIT(sms, value, 1 << ((opcode >> 3) & 0x7));
            return false;

        case 0x10: case 0x11: case 0x12: case 0x13:
        case 0x14: case 0x15: case 0x16: case 0x17:
            *result = RES(sms, value, 1 << ((opcode >> 3) & 0x7));
            return true;

        case 0x18: case 0x19: case 0x1A: case 0x1B:
        case 0x1C: case 0x1D: case 0x1E: case 0x1F:
            *result = SET(sms, value, 1 << ((opcode >> 3) & 0x7));
            return true;
    }

    UNREACHABLE(false);
}

static FORCE_INLINE void execute_CB(struct SMS_Core* sms)
{
    const uint8_t opcode = read8(REG_PC++);
    const uint8_t value = get_r8(sms, opcode);

    uint8_t result = 0;

    if (_CB(sms, opcode, value, &result))
    {
        set_r8(sms, result, opcode);
    }

    sms->cpu.cycles += CYC_CB[opcode]; // pretty sure this isn't accurate
}

static FORCE_INLINE void execute_CB_IXIY(struct SMS_Core* sms, uint16_t ixy)
{
    // the address is actually fectched before the opcode
    const uint16_t addr = ixy + (int8_t)read8(REG_PC++);
    const uint8_t value = read8(addr);
    const uint8_t opcode = read8(REG_PC++);

    uint8_t result = 0;

    if (_CB(sms, opcode, value, &result))
    {
        switch (opcode & 0x7)
        {
            case 0x0: REG_B = result; break;
            case 0x1: REG_C = result; break;
            case 0x2: REG_D = result; break;
            case 0x3: REG_E = result; break;
            case 0x4: REG_H = result; break;
            case 0x5: REG_L = result; break;
            case 0x6: write8(addr, result); break;
            case 0x7: REG_A = result; break;
        }

        sms->cpu.cycles += 23;
    }
    // bit is the only instr that doesn't set the result,
    // so this this 'else' will be for bit!
    else
    {
        // for IXIY BIT() instr only, the B3 and B5 flags are set differently
        FLAG_B3 = IS_BIT_SET(addr >> 8, 3);
        FLAG_B5 = IS_BIT_SET(addr >> 8, 5);
        sms->cpu.cycles += 20;
    }
}

static FORCE_INLINE void execute_IXIY(struct SMS_Core* sms, uint8_t* ixy_hi, uint8_t* ixy_lo)
{
    const uint8_t opcode = read8(REG_PC++);
    const uint16_t pair = PAIR(*ixy_hi, *ixy_lo);

    sms->cpu.cycles += CYC_DDFD[opcode];

    #define DISP() (int8_t)read8(REG_PC++)

    switch (opcode)
    {
        case 0x09:
        {
            const uint16_t result = ADD_DD_16(sms, pair, REG_BC);
            SET_PAIR(*ixy_hi, *ixy_lo, result);
        }   break;

        case 0x19:
        {
            const uint16_t result = ADD_DD_16(sms, pair, REG_DE);
            SET_PAIR(*ixy_hi, *ixy_lo, result);
        }   break;

        case 0x29:
        {
            const uint16_t result = ADD_DD_16(sms, pair, pair);
            SET_PAIR(*ixy_hi, *ixy_lo, result);
        }   break;

        case 0x39:
        {
            const uint16_t result = ADD_DD_16(sms, pair, REG_SP);
            SET_PAIR(*ixy_hi, *ixy_lo, result);
        }   break;

        case 0x21:
            *ixy_lo = read8(REG_PC++);
            *ixy_hi = read8(REG_PC++);
            break;

        case 0x22: LD_imm_r16(sms, pair); break;

        case 0x23: ++*ixy_lo; if (*ixy_lo == 0x00) { ++*ixy_hi; } break;
        case 0x2B: --*ixy_lo; if (*ixy_lo == 0xFF) { --*ixy_hi; } break;

        case 0x2A:
        {
            const uint16_t addr = read16(REG_PC);
            REG_PC += 2;

            const uint16_t r = read16(addr);

            *ixy_hi = r >> 8;
            *ixy_lo = r & 0xFF;
        }   break;

        case 0x34:
        {
            const uint16_t addr = pair + DISP();
            write8(addr, _INC(sms, read8(addr)));
        }   break;

        case 0x35:
        {
            const uint16_t addr = pair + DISP();
            write8(addr, _DEC(sms, read8(addr)));
        }   break;

        case 0x36: // addr disp is stored first, then imm value
        {
            const uint16_t addr = pair + DISP();
            const uint8_t value = read8(REG_PC++);
            write8(addr, value);
        }   break;

        case 0x64: /* *ixy_hi = *ixy_hi; */ break;
        case 0x65: *ixy_hi = *ixy_lo; break;
        case 0x6C: *ixy_lo = *ixy_hi; break;
        case 0x6D: /* *ixy_lo = *ixy_lo; */ break;

        case 0x60: case 0x61: case 0x62: case 0x63: case 0x67:
            *ixy_hi = get_r8(sms, opcode);
            break;

        case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6F:
            *ixy_lo = get_r8(sms, opcode);
            break;

        case 0x8E:
        {
            const uint8_t value = read8(pair + DISP());
            _ADC(sms, value);
        }   break;

        case 0x26: *ixy_hi = read8(REG_PC++); break;
        case 0x2E: *ixy_lo = read8(REG_PC++); break;
        case 0x24: *ixy_hi = _INC(sms, *ixy_hi); break;
        case 0x2C: *ixy_lo = _INC(sms, *ixy_lo); break;
        case 0x25: *ixy_hi = _DEC(sms, *ixy_hi); break;
        case 0x2D: *ixy_lo = _DEC(sms, *ixy_lo); break;
        case 0x84: _ADD(sms, *ixy_hi); break; // untested
        case 0x85: _ADD(sms, *ixy_lo); break; // untested
        case 0x86: _ADD(sms, read8(pair + DISP())); break;
        case 0x94: _SUB(sms, *ixy_hi); break; // untested
        case 0x95: _SUB(sms, *ixy_lo); break; // untested
        case 0x96: _SUB(sms, read8(pair + DISP())); break;
        case 0x9E: _SBC(sms, read8(pair + DISP())); break;
        case 0xA4: _AND(sms, *ixy_hi); break; // untested
        case 0xA5: _AND(sms, *ixy_lo); break; // untested
        case 0xA6: _AND(sms, read8(pair + DISP())); break;
        case 0xAC: _XOR(sms, *ixy_hi); break; // untested
        case 0xAD: _XOR(sms, *ixy_lo); break; // untested
        case 0xAE: _XOR(sms, read8(pair + DISP())); break;
        case 0xB4: _OR(sms, *ixy_hi); break; // untested
        case 0xB5: _OR(sms, *ixy_lo); break; // untested
        case 0xB6: _OR(sms, read8(pair + DISP())); break;
        case 0xBC: _CP(sms, *ixy_hi); break; // untested
        case 0xBD: _CP(sms, *ixy_lo); break; // untested
        case 0xBE: _CP(sms, read8(pair + DISP())); break;
        case 0xE9: REG_PC = pair; break;
        case 0xF9: REG_SP = pair; break;

        case 0x44:
        case 0x4C:
        case 0x54:
        case 0x5C:
        case 0x7C:
            set_r8(sms, *ixy_hi, opcode >> 3);
            break;

        case 0x45:
        case 0x4D:
        case 0x55:
        case 0x5D:
        case 0x7D:
            set_r8(sms, *ixy_lo, opcode >> 3);
            break;

        case 0x46: case 0x4E: case 0x56: case 0x5E:
        case 0x66: case 0x6E: case 0x7E:
            set_r8(sms, read8(pair + DISP()), opcode >> 3);
            break;

        case 0x70: case 0x71: case 0x72: case 0x73:
        case 0x74: case 0x75: case 0x77:
            write8(pair + DISP(), get_r8(sms, opcode));
            break;

        case 0xE3:
        {
            const uint16_t value = read16(REG_SP);
            write16(REG_SP, pair);
            *ixy_hi = value >> 8;
            *ixy_lo = value & 0xFF;
        }   break;

        case 0xE5: PUSH(sms, pair); break;
        case 0xE1:
        {
            const uint16_t r = POP(sms);
            *ixy_hi = r >> 8;
            *ixy_lo = r & 0xFF;
        }   break;

        case 0xCB: execute_CB_IXIY(sms, pair); break;

        default:
            //SMS_log_fatal("UNK OP: 0xFD%02X\n", opcode);
            break;
    }

    #undef DISP
}

static FORCE_INLINE void execute_ED(struct SMS_Core* sms)
{
    const uint8_t opcode = read8(REG_PC++);
    sms->cpu.cycles += CYC_ED[opcode];

    switch (opcode)
    {
        case 0x40: REG_B = IN(sms); break;
        case 0x48: REG_C = IN(sms); break;
        case 0x50: REG_D = IN(sms); break;
        case 0x58: REG_E = IN(sms); break;
        case 0x60: REG_H = IN(sms); break;
        case 0x68: REG_L = IN(sms); break;
        case 0x70: /*non*/ IN(sms); break;
        case 0x78: REG_A = IN(sms); break;

        case 0x41: OUT(sms, REG_B); break;
        case 0x49: OUT(sms, REG_C); break;
        case 0x51: OUT(sms, REG_D); break;
        case 0x59: OUT(sms, REG_E); break;
        case 0x61: OUT(sms, REG_H); break;
        case 0x69: OUT(sms, REG_L); break;
        case 0x71: OUT(sms, 0); break;
        case 0x79: OUT(sms, REG_A); break;

        case 0x45: case 0x55: case 0x5D: case 0x65:
        case 0x6D: case 0x75: case 0x7D:
            RETN(sms);
            break;

        case 0x4D: RETI(sms); break;

        case 0x43: LD_imm_r16(sms, REG_BC); break;
        case 0x53: LD_imm_r16(sms, REG_DE); break;
        case 0x63: LD_imm_r16(sms, REG_HL); break;
        case 0x73: LD_imm_r16(sms, REG_SP); break;

        case 0x4B:
        {
            const uint16_t addr = read16(REG_PC);
            REG_PC += 2;

            const uint16_t value = read16(addr);

            SET_REG_BC(value);
        }   break;

        case 0x5B:
        {
            const uint16_t addr = read16(REG_PC);
            REG_PC += 2;

            const uint16_t value = read16(addr);

            SET_REG_DE(value);
        }   break;

        case 0x6B:
        {
            const uint16_t addr = read16(REG_PC);
            REG_PC += 2;

            const uint16_t value = read16(addr);

            SET_REG_HL(value);
        }   break;

        case 0x7B:
        {
            const uint16_t addr = read16(REG_PC);
            REG_PC += 2;

            const uint16_t value = read16(addr);

            REG_SP = value;
        }   break;


        case 0x42: SBC_hl(sms, REG_BC); break;
        case 0x52: SBC_hl(sms, REG_DE); break;
        case 0x62: SBC_hl(sms, REG_HL); break;
        case 0x72: SBC_hl(sms, REG_SP); break;

        case 0x4A: ADC_hl(sms, REG_BC); break;
        case 0x5A: ADC_hl(sms, REG_DE); break;
        case 0x6A: ADC_hl(sms, REG_HL); break;
        case 0x7A: ADC_hl(sms, REG_SP); break;

        case 0x44: case 0x54: case 0x64: case 0x74:
        case 0x4C: case 0x5C: case 0x6C: case 0x7C:
            NEG(sms);
            break;

        case 0x46: case 0x66:
            IMM_set(sms, 0);
            break;

        case 0x56: case 0x76:
            IMM_set(sms, 1);
            break;

        case 0x5E: case 0x7E:
            IMM_set(sms, 2);
            break;

        case 0x47: LD_I_A(sms); break;
        case 0x4F: LD_R_A(sms); break;
        case 0x57: LD_A_I(sms); break;
        case 0x5F: LD_A_R(sms); break;

        case 0x67: RRD(sms); break;
        case 0x6F: RLD(sms); break;
        case 0xA0: LDI(sms); break;
        case 0xB0: LDIR(sms); break;
        case 0xA8: LDD(sms); break;
        case 0xB8: LDDR(sms); break;
        case 0xA1: CPI(sms); break;
        case 0xB1: CPIR(sms); break;
        case 0xA9: CPD(sms); break;
        case 0xB9: CPDR(sms); break;
        case 0xA2: INI(sms); break;
        case 0xB2: INIR(sms); break;
        case 0xAA: IND(sms); break;
        case 0xBA: INDR(sms); break;
        case 0xA3: OUTI(sms); break;
        case 0xB3: OTIR(sms); break;
        case 0xAB: OUTD(sms); break;
        case 0xBB: OTDR(sms); break;

        default:
            //SMS_log_fatal("UNK OP: 0xED%02X\n", opcode);
            break;
    }
}

static FORCE_INLINE void execute(struct SMS_Core* sms)
{
    const uint8_t opcode = read8(REG_PC++);
    sms->cpu.cycles += CYC_00[opcode];
	
    switch (opcode)
    {
        case 0x00: break; // nop

        case 0x07: RLCA(sms); break;
        case 0x0F: RRCA(sms); break;
        case 0x17: RLA(sms); break;
        case 0x1F: RRA(sms); break;

        case 0x2F: CPL(sms); break;
        case 0x27: DAA(sms); break;
        case 0x37: SCF(sms); break;
        case 0x3F: CCF(sms); break;

        case 0x04: case 0x0C: case 0x14: case 0x1C:
        case 0x24: case 0x2C: case 0x34: case 0x3C:
            INC_r8(sms, opcode);
            break;

        case 0x05: case 0x0D: case 0x15: case 0x1D:
        case 0x25: case 0x2D: case 0x35: case 0x3D:
            DEC_r8(sms, opcode);
            break;

        case 0x03: case 0x13: case 0x23: case 0x33:
            INC_r16(sms, opcode);
            break;

        case 0x0B: case 0x1B: case 0x2B: case 0x3B:
            DEC_r16(sms, opcode);
            break;

        case 0x09: case 0x19: case 0x29: case 0x39:
            ADD_HL(sms, opcode);
            break;

        case 0x01: case 0x11: case 0x21: case 0x31:
            LD_16(sms, opcode);
            break;

        case 0x02: LD_r16_a(sms, REG_BC); break;
        case 0x12: LD_r16_a(sms, REG_DE); break;
        case 0x0A: LD_a_r16(sms, REG_BC); break;
        case 0x1A: LD_a_r16(sms, REG_DE); break;
        case 0x22: LD_imm_r16(sms, REG_HL); break;
        case 0x2A: LD_hl_imm(sms); break;
        case 0x32: LD_imm_a(sms); break;
        case 0x3A: LD_a_imm(sms); break;
        case 0xF9: LD_sp_hl(sms); break;

        case 0x06: case 0x0E: case 0x16: case 0x1E:
        case 0x26: case 0x2E: case 0x36: case 0x3E:
            LD_r_u8(sms, opcode);
            break;

        case 0x41: case 0x42: case 0x43: case 0x44:
        case 0x45: case 0x46: case 0x47: case 0x48:
        case 0x4A: case 0x4B: case 0x4C: case 0x4D:
        case 0x4E: case 0x4F: case 0x50: case 0x51:
        case 0x53: case 0x54: case 0x55: case 0x56:
        case 0x57: case 0x58: case 0x59: case 0x5A:
        case 0x5C: case 0x5D: case 0x5E: case 0x5F:
        case 0x60: case 0x61: case 0x62: case 0x63:
        case 0x65: case 0x66: case 0x67: case 0x68:
        case 0x69: case 0x6A: case 0x6B: case 0x6C:
        case 0x6E: case 0x6F: case 0x70: case 0x71:
        case 0x72: case 0x73: case 0x74: case 0x75:
        case 0x77: case 0x78: case 0x79: case 0x7A:
        case 0x7B: case 0x7C: case 0x7D: case 0x7E:
            LD_rr(sms, opcode);
            break;

        case 0x40: break; // nop LD b,b
        case 0x49: break; // nop LD c,c
        case 0x52: break; // nop LD d,d
        case 0x5B: break; // nop LD e,e
        case 0x64: break; // nop LD h,h
        case 0x6D: break; // nop LD l,l
        case 0x7F: break; // nop LD a,a

        case 0x76: HALT(sms); break;

        case 0x80: case 0x81: case 0x82: case 0x83:
        case 0x84: case 0x85: case 0x86: case 0x87:
            ADD_r(sms, opcode);
            break;

        case 0x88: case 0x89: case 0x8A: case 0x8B:
        case 0x8C: case 0x8D: case 0x8E: case 0x8F:
            ADC_r(sms, opcode);
            break;

        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97:
            SUB_r(sms, opcode);
            break;

        case 0x98: case 0x99: case 0x9A: case 0x9B:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F:
            SBC_r(sms, opcode);
            break;

        case 0xA0: case 0xA1: case 0xA2: case 0xA3:
        case 0xA4: case 0xA5: case 0xA6: case 0xA7:
            AND_r(sms, opcode);
            break;

        case 0xA8: case 0xA9: case 0xAA: case 0xAB:
        case 0xAC: case 0xAD: case 0xAE: case 0xAF:
            XOR_r(sms, opcode);
            break;

        case 0xB0: case 0xB1: case 0xB2: case 0xB3:
        case 0xB4: case 0xB5: case 0xB6: case 0xB7:
            OR_r(sms, opcode);
            break;

        case 0xB8: case 0xB9: case 0xBA: case 0xBB:
        case 0xBC: case 0xBD: case 0xBE: case 0xBF:
            CP_r(sms, opcode);
            break;

        case 0xC6: ADD_imm(sms); break;
        case 0xCE: ADC_imm(sms); break;
        case 0xD6: SUB_imm(sms); break;
        case 0xDE: SBC_imm(sms); break;
        case 0xE6: AND_imm(sms); break;
        case 0xEE: XOR_imm(sms); break;
        case 0xF6: OR_imm(sms); break;
        case 0xFE: CP_imm(sms); break;

        case 0x10: DJNZ(sms); break;
        case 0x18: JR(sms); break;
        case 0xE9: REG_PC = REG_HL; break;
        case 0xC3: JP(sms); break;
        case 0xC9: RET(sms); break;
        case 0xCD: CALL(sms); break;

        case 0xC1: POP_BC(sms); break;
        case 0xD1: POP_DE(sms); break;
        case 0xE1: POP_HL(sms); break;
        case 0xF1: POP_AF(sms); break;

        case 0xC5: PUSH(sms, REG_BC); break;
        case 0xD5: PUSH(sms, REG_DE); break;
        case 0xE5: PUSH(sms, REG_HL); break;
        case 0xF5: PUSH(sms, REG_AF); break;

        // i couldnt figure out a way to decode the cond flag from
        // the opcode, so ive hardcoded the flags (for now)...
        case 0x20: JR_cc(sms, FLAG_Z == false); break;
        case 0x28: JR_cc(sms, FLAG_Z == true); break;
        case 0x30: JR_cc(sms, FLAG_C == false); break;
        case 0x38: JR_cc(sms, FLAG_C == true); break;

        case 0xC0: RET_cc(sms, FLAG_Z == false); break;
        case 0xC8: RET_cc(sms, FLAG_Z == true); break;
        case 0xD0: RET_cc(sms, FLAG_C == false); break;
        case 0xD8: RET_cc(sms, FLAG_C == true); break;
        case 0xE0: RET_cc(sms, FLAG_P == false); break;
        case 0xE8: RET_cc(sms, FLAG_P == true); break;
        case 0xF0: RET_cc(sms, FLAG_S == false); break;
        case 0xF8: RET_cc(sms, FLAG_S == true); break;

        case 0xC2: JP_cc(sms, FLAG_Z == false); break;
        case 0xCA: JP_cc(sms, FLAG_Z == true); break;
        case 0xD2: JP_cc(sms, FLAG_C == false); break;
        case 0xDA: JP_cc(sms, FLAG_C == true); break;
        case 0xE2: JP_cc(sms, FLAG_P == false); break;
        case 0xEA: JP_cc(sms, FLAG_P == true); break;
        case 0xF2: JP_cc(sms, FLAG_S == false); break;
        case 0xFA: JP_cc(sms, FLAG_S == true); break;

        case 0xC4: CALL_cc(sms, FLAG_Z == false); break;
        case 0xCC: CALL_cc(sms, FLAG_Z == true); break;
        case 0xD4: CALL_cc(sms, FLAG_C == false); break;
        case 0xDC: CALL_cc(sms, FLAG_C == true); break;
        case 0xE4: CALL_cc(sms, FLAG_P == false); break;
        case 0xEC: CALL_cc(sms, FLAG_P == true); break;
        case 0xF4: CALL_cc(sms, FLAG_S == false); break;
        case 0xFC: CALL_cc(sms, FLAG_S == true); break;

        case 0xC7: case 0xCF: case 0xD7: case 0xDF:
        case 0xE7: case 0xEF: case 0xF7: case 0xFF:
            RST(sms, opcode & 0x38);
            break;

        case 0x08: EX_af_af(sms); break;
        case 0xD9: EXX(sms); break;
        case 0xE3: EX_sp_hl(sms); break;
        case 0xEB: EX_de_hl(sms); break;

        case 0xD3: OUT_imm(sms); break;
        case 0xDB: IN_imm(sms); break;

        case 0xF3: DI(sms); break;
        case 0xFB: EI(sms); break;

        case 0xCB: execute_CB(sms); break;
        case 0xDD: execute_IXIY(sms, &REG_IXH, &REG_IXL); break;
        case 0xED: execute_ED(sms); break;
        case 0xFD: execute_IXIY(sms, &REG_IYH, &REG_IYL); break;

        default:
            //SMS_log_fatal("UNK OP: 0x%02X\n", opcode);
            break;
    }
}

void z80_run(struct SMS_Core* sms)
{
    sms->cpu.cycles = 0;

    if (!sms->cpu.halt)
    {
        execute(sms);
    }
    else
    {
        sms->cpu.cycles = 4;
    }

    isr(sms);
}

void z80_init(struct SMS_Core* sms)
{
    // setup cpu regs, initial values from Sean Young docs.
    sms->cpu.PC = 0x0000;
    sms->cpu.SP = 0xDFF0; // fixes ace of aces and shadow dancer
    sms->cpu.main.A = 0xFF;
    sms->cpu.main.flags.C = true;
    sms->cpu.main.flags.N = true;
    sms->cpu.main.flags.P = true;
    sms->cpu.main.flags.H = true;
    sms->cpu.main.flags.B3 = true;
    sms->cpu.main.flags.B5 = true;
    sms->cpu.main.flags.Z = true;
    sms->cpu.main.flags.S = true;
}


// *** sms_joypad.c ***


// [API]
void SMS_set_port_a(struct SMS_Core* sms, const enum SMS_PortA pin, const bool down)
{
    if (down)
    {
        sms->port.a &= ~pin;

        // if (is_down && (buttons & GB_BUTTON_DIRECTIONAL))
        // can't have opposing directions pressed at the same time
        {
            if (pin & JOY1_RIGHT_BUTTON)
            {
                sms->port.a |= JOY1_LEFT_BUTTON;
            }
            if (pin & JOY1_LEFT_BUTTON)
            {
                sms->port.a |= JOY1_RIGHT_BUTTON;
            }
            if (pin & JOY1_UP_BUTTON)
            {
                sms->port.a |= JOY1_DOWN_BUTTON;
            }
            if (pin & JOY1_DOWN_BUTTON)
            {
                sms->port.a |= JOY1_UP_BUTTON;
            }
        }
    }
    else
    {
        sms->port.a |= pin;
    }
}

void SMS_set_port_b(struct SMS_Core* sms, const enum SMS_PortB pin, const bool down)
{
    if (pin == PAUSE_BUTTON)
    {
        if (SMS_is_system_type_gg(sms))
        {
            sms->port.gg_regs[0x0] &= ~0x80;

            if (!down)
            {
                sms->port.gg_regs[0x0] |= 0x80;
            }
        }
        else
        {
            if (down)
            {
                z80_nmi(sms);
            }
        }
    }
    else
    {
        if (down)
        {
            sms->port.b &= ~pin;
        }
        else
        {
            sms->port.b |= pin;
        }

    }

    // bit 5 is unused on SMS and GG (used on MD)
    sms->port.b |= 1 << 5;

    if (SMS_is_system_type_gg(sms))
    {
        // GG (and MD) have no reset button so always return 1
        sms->port.b |= 1 << 4;
    }
}


// *** sms.c ***


// not all values are listed here because the other
// values are not used by official software and
// the checksum is broken on those sizes.
static const bool valid_rom_size_values[0x10] =
{
    [0xC] = true, // 32KiB
    [0xE] = true, // 64KiB
    [0xF] = true, // 128KiB
    [0x0] = true, // 256KiB
    [0x1] = true, // 512KiB
};

static const char* const valid_rom_size_string[0x10] =
{
    [0xC] = "32KiB",
    [0xE] = "64KiB",
    [0xF] = "128KiB",
    [0x0] = "256KiB",
    [0x1] = "512KiB",
};

static const char* const region_code_string[0x10] =
{
    [0x3] = "SMS Japan",
    [0x4] = "SMS Export",
    [0x5] = "GG Japan",
    [0x6] = "GG Export",
    [0x7] = "GG International",
};

static uint16_t find_rom_header_offset(volatile uint8_t* data)
{
    // loop until we find the magic num
    // the rom header can start at 1 of 3 offsets
    const uint16_t offsets[] =
    {
        // the bios checks in reverse order
        0x7FF0,
        0x3FF0,
        0x1FF0,
    };

    for (size_t i = 0; i < ARRAY_SIZE(offsets); ++i)
    {
        const uint8_t* d = data + offsets[i];
        const char* magic = "TMR SEGA";

        if (d[0] == magic[0] && d[1] == magic[1] &&
            d[2] == magic[2] && d[3] == magic[3] &&
            d[4] == magic[4] && d[5] == magic[5] &&
            d[6] == magic[6] && d[7] == magic[7])
        {
            return offsets[i];
        }
    }

    // invalid offset, this zero needs to be checked by the caller!
    return 0;
}

/* SOURCE: https://web.archive.org/web/20190108202303/http://www.hackersdelight.org/hdcodetxt/crc.c.txt */
uint32_t SMS_crc32(volatile uint8_t *data, size_t size)
{
    int crc;
    unsigned int byte, c;
    const unsigned int g0 = 0xEDB88320,    g1 = g0>>1,
        g2 = g0>>2, g3 = g0>>3, g4 = g0>>4, g5 = g0>>5,
        g6 = (g0>>6)^g0, g7 = ((g0>>6)^g0)>>1;

    crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        byte = ((const uint8_t*)data)[i];
        crc = crc ^ byte;
        c = ((crc<<31>>31) & g7) ^ ((crc<<30>>31) & g6) ^
            ((crc<<29>>31) & g5) ^ ((crc<<28>>31) & g4) ^
            ((crc<<27>>31) & g3) ^ ((crc<<26>>31) & g2) ^
            ((crc<<25>>31) & g1) ^ ((crc<<24>>31) & g0);
        crc = ((unsigned)crc >> 8) ^ c;
    }
    return ~crc;
}

void SMS_skip_frame(struct SMS_Core* sms, bool enable)
{
    sms->skip_frame = enable;
}

void SMS_set_system_type(struct SMS_Core* sms, enum SMS_System system)
{
    sms->system = system;
}

enum SMS_System SMS_get_system_type(const struct SMS_Core* sms)
{
    return sms->system;
}

bool SMS_is_system_type_sms(const struct SMS_Core* sms)
{
    return SMS_get_system_type(sms) == SMS_System_SMS;
}

bool SMS_is_system_type_gg(const struct SMS_Core* sms)
{
    return SMS_get_system_type(sms) == SMS_System_GG;
}

bool SMS_is_system_type_sg(const struct SMS_Core* sms)
{
    return SMS_get_system_type(sms) == SMS_System_SG1000;
}

bool SMS_is_spiderman_int_hack_enabled(const struct SMS_Core* sms)
{
    return sms->crc == 0xEBE45388;
}

struct SMS_RomHeader SMS_parse_rom_header(volatile uint8_t* data, uint16_t offset)
{

    struct SMS_RomHeader header = {0};

	for (int i=0; i<8; i++)
	{
		header.magic[i] = data[offset + i];
	}

	offset += 8 + 2; // skip 2 padding bytes as well

	header.checksum = (uint16_t)((data[offset] << 8) + (data[offset+1]));

	offset += 2;

    // the next part depends on if the host is LE or BE.
    // due to needing to read half nibble.
    // for now, assume LE, as it likely will be...
    uint32_t last_4 = (uint32_t)(data[offset]);

    #if SMS_LITTLE_ENDIAN
        header.prod_code = 0; // this isn't correct atm
        header.version = (last_4 >> 16) & 0xF;
        header.rom_size = (last_4 >> 24) & 0xF;
        header.region_code = (last_4 >> 28) & 0xF;
    #else
        header.rom_size = last_4 & 0xF;
        // todo: the rest
    #endif

    return header;
}

static void log_header(const struct SMS_RomHeader* header)
{
    // silence warnings if logging is disabled
    UNUSED(header); UNUSED(region_code_string); UNUSED(valid_rom_size_string);

    //SMS_log("version: [0x%X]\n", header->version);
    //SMS_log("region_code: [0x%X] [%s]\n", header->region_code, region_code_string[header->region_code]);
    //SMS_log("rom_size: [0x%X] [%s]\n", header->rom_size, valid_rom_size_string[header->rom_size]);
}

bool SMS_init(struct SMS_Core* sms)
{
	sms->vdp.vram = ext_ram + 0x0000;
    sms->vdp.dirty_vram = ext_ram + 0x4000;
	
	sms->pitch = SMS_SCREEN_WIDTH;
    sms->bpp = 1;
}

static void SMS_reset(struct SMS_Core* sms)
{
    // do NOT reset cart!
	for (int i=0; i<64; i++)
	{
		sms->rmap[i] = 0;
		sms->wmap[i] = 0;
	}

    z80_init(sms);
    psg_init(sms);
    vdp_init(sms);

    // enable everything in control
	sms->memory_control.exp_slot_disable = 0;
   	sms->memory_control.cart_slot_disable = 0;
   	sms->memory_control.card_slot_disable = 0;
    sms->memory_control.work_ram_disable = 0;
    sms->memory_control.bios_rom_disable = 0;
    sms->memory_control.io_chip_disable = 0;

    // if we don't have bios, disable it in control
    if (!SMS_has_bios(sms))
    {
        sms->memory_control.bios_rom_disable = true;
    }

    // port A/B are hi when a button is NOT pressed
    sms->port.a = 0xFF;
    sms->port.b = 0xFF;

    if (SMS_is_system_type_gg(sms))
    {
        sms->port.gg_regs[0x0] = 0xC0;
        sms->port.gg_regs[0x1] = 0x7F;
        sms->port.gg_regs[0x2] = 0xFF;
        sms->port.gg_regs[0x3] = 0x00;
        sms->port.gg_regs[0x4] = 0xFF;
        sms->port.gg_regs[0x5] = 0x00;
        sms->port.gg_regs[0x6] = 0xFF;
    }
}

bool SMS_has_bios(const struct SMS_Core* sms)
{
    // bios should be at least 1-page size in size
    return sms->bios && sms->bios_size >= 1024 && sms->bios_size <= 1024*32;
}

bool SMS_loadbios(struct SMS_Core* sms, volatile uint8_t* bios, size_t size)
{
    sms->bios = bios;
    sms->bios_size = size;

    // todo: hash all known bios to know exactly what bios is being loaded
    return SMS_has_bios(sms);
}

static bool sg_loadrom(struct SMS_Core* sms, volatile uint8_t* rom, size_t size, int system_hint)
{
    //assert(system_hint == SMS_System_SG1000);

    //SMS_log("[INFO] trying to load sg rom\n");

    // save the rom, setup the size and mask
    sms->rom = rom;
    sms->rom_size = size;
    sms->rom_mask = size / 0x400; // this works because size is always pow2
    sms->cart.max_bank_mask = (size / 0x4000) - 1;
    sms->crc = SMS_crc32(rom, size);

    //SMS_log("crc32 0x%08X\n", sms->crc);

    SMS_set_system_type(sms, system_hint);
    sms->cart.mapper_type = MAPPER_TYPE_NONE;
    SMS_reset(sms);

    // this assumes the game is always sega mapper
    // which (for testing at least), it always will be
    mapper_init(sms);

    return true;
}

static bool loadrom2(struct SMS_Core* sms, struct RomEntry* entry, volatile uint8_t* rom, size_t size)
{
    // save the rom, setup the size and mask
    sms->rom = rom;
    sms->rom_size = size;
    sms->rom_mask = size / 0x400; // this works because size is always pow2
    sms->cart.max_bank_mask = (size / 0x4000) - 1;
    sms->crc = entry->crc;

    SMS_set_system_type(sms, entry->sys);
    SMS_reset(sms);

    // this assumes the game is always sega mapper
    // which (for testing at least), it always will be
    sms->cart.mapper_type = entry->map;
    mapper_init(sms);

    return true;
}

bool SMS_loadrom(struct SMS_Core* sms, volatile uint8_t* rom, size_t size, int system_hint)
{
    //assert(sms);
    //assert(rom);
    //assert(size);
    //assert(sms && rom && size);

    //SMS_log("[INFO] loadrom called with rom size: 0x%zX\n", size);

    struct RomEntry entry = {0};
    const uint32_t crc = SMS_crc32(rom, size);
    //SMS_log("crc32 0x%08X\n", crc);

    //SMS_log("couldn't find rom in database, checking system hint\n");

    if (system_hint == SMS_System_SG1000)
    {
        //SMS_log("system hint is SG1000, trying to load...\n");
        return sg_loadrom(sms, rom, size, system_hint);
    }
    else
    {

    }

    // try to find the header offset
    const uint16_t header_offset = find_rom_header_offset(rom);

    // no header found!
    if (header_offset == 0)
    {
        //SMS_log_fatal("[ERROR] unable to find rom header!\n");
        return false;
    }

    //SMS_log("[INFO] found header offset at: 0x%X\n", header_offset);

    struct SMS_RomHeader header = SMS_parse_rom_header(rom, header_offset);
    log_header(&header);

    // check if the size is valid
    if (!valid_rom_size_values[header.rom_size])
    {
        //SMS_log_fatal("[ERROR] invalid rom size in header! 0x%X\n", header.rom_size);
        return false;
    }

    // save the rom, setup the size and mask
    sms->rom = rom;
    sms->rom_size = size;
    sms->rom_mask = size / 0x400; // this works because size is always pow2
    sms->cart.max_bank_mask = (size / 0x4000) - 1;
    sms->crc = crc;

    //SMS_log("crc32 0x%08X\n", sms->crc);

    if (system_hint != -1)
    {
        SMS_set_system_type(sms, system_hint);
    }
    else if (header.region_code == 0x5 || header.region_code == 0x6 || header.region_code == 0x7)
    {
        SMS_set_system_type(sms, SMS_System_GG);
    }
    else
    {
        SMS_set_system_type(sms, SMS_System_SMS);
    }

    SMS_reset(sms);

    // this assumes the game is always sega mapper
    // which (for testing at least), it always will be
    sms->cart.mapper_type = MAPPER_TYPE_SEGA;
    mapper_init(sms);

    return true;
}


bool SMS_used_sram(const struct SMS_Core* sms)
{
    return sms->cart.sram_used;
}

void SMS_set_better_drums(struct SMS_Core* sms, bool enable)
{
    sms->better_drums = enable;
}

enum { STATE_MAGIC = 0x5E6A };
enum { STATE_VERSION = 2 };


bool SMS_parity16(uint16_t value)
{
    #if HAS_BUILTIN(__builtin_parity) && !defined(N64)
        return !__builtin_parity(value);
    #else
        // SOURCE: https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
        value ^= value >> 8; // 16-bit
        value ^= value >> 4; // 8-bit
        value &= 0xF;
        return !((0x6996 >> value) & 0x1);
    #endif
}

bool SMS_parity8(uint8_t value)
{
    #if HAS_BUILTIN(__builtin_parity) && !defined(N64)
        return !__builtin_parity(value);
    #else
        // SOURCE: https://graphics.stanford.edu/~seander/bithacks.html#ParityParallel
        value ^= value >> 4; // 8-bit
        value &= 0xF;
        return !((0x6996 >> value) & 0x1);
    #endif
}

void SMS_run(struct SMS_Core* sms, size_t cycles)
{
    for (size_t i = 0; i < cycles; i += sms->cpu.cycles)
    {		
        z80_run(sms);

        vdp_run(sms, sms->cpu.cycles);

        psg_run(sms, sms->cpu.cycles);

        //assert(sms->cpu.cycles != 0);
    }

    psg_sync(sms);
}


// *** sms_psg.c ***


#define PSG sms->psg

enum
{
    LATCH_TYPE_TONE = 0,
    LATCH_TYPE_NOISE = 0,
    LATCH_TYPE_VOL = 1,

    WHITE_NOISE = 1,
    PERIDOIC_NOISE = 0,

    // this depends on the system.
    // this should be a var instead, but ill add that when needed!
    TAPPED_BITS = 0x0009,

    LFSR_RESET_VALUE = 0x8000,
};

#if SMS_DISABLE_AUDIO

void psg_reg_write(struct SMS_Core* sms, const uint8_t value) {}
void psg_sync(struct SMS_Core* sms) {}
void psg_run(struct SMS_Core* sms, const uint8_t cycles) {}

#else

static FORCE_INLINE void latch_reg_write(struct SMS_Core* sms, const uint8_t value)
{
    PSG.latched_channel = (value >> 5) & 0x3;
    PSG.latched_type = (value >> 4) & 0x1;

    const uint8_t data = value & 0xF;

    if (PSG.latched_type == LATCH_TYPE_VOL)
    {
        PSG.volume[PSG.latched_channel] = data & 0xF;
    }
    else
    {
        switch (PSG.latched_channel)
        {
            case 0: case 1: case 2:
                PSG.tone[PSG.latched_channel].tone &= 0x3F0;
                PSG.tone[PSG.latched_channel].tone |= data;
                break;

            case 3:
                PSG.noise.lfsr = LFSR_RESET_VALUE;
                PSG.noise.shift_rate = data & 0x3;
                PSG.noise.mode = (data >> 2) & 0x1;
                break;
        }
    }
}

static FORCE_INLINE void data_reg_write(struct SMS_Core* sms, const uint8_t value)
{
    const uint8_t data = value & 0x3F;

    if (PSG.latched_type == LATCH_TYPE_VOL)
    {
        PSG.volume[PSG.latched_channel] = data & 0xF;
    }
    else
    {
        switch (PSG.latched_channel)
        {
            case 0: case 1: case 2:
                PSG.tone[PSG.latched_channel].tone &= 0xF;
                PSG.tone[PSG.latched_channel].tone |= data << 4;
                break;

            case 3:
                PSG.noise.lfsr = LFSR_RESET_VALUE;
                PSG.noise.shift_rate = data & 0x3;
                PSG.noise.mode = (data >> 2) & 0x1;
                break;
        }
    }
}

void psg_reg_write(struct SMS_Core* sms, const uint8_t value)
{
    psg_sync(sms);

    // if MSB is set, then this is a latched write, else its a normal data write
    if (value & 0x80)
    {
        latch_reg_write(sms, value);
    }
    else
    {
        data_reg_write(sms, value);
    }
}

static FORCE_INLINE void tick_tone(struct SMS_Core* sms, const uint8_t index, const uint8_t cycles)
{
    enum { MAX_SAMPLE_RATE = 8 }; // fixes shinobi tone2 (set to less than 8 to hear bug)

    // we don't want to keep change the polarity if the counter is already zero,
    // especially if the volume is still not off!
    // otherwise this will cause a hi-pitch screech, can be heard in golden-axe
    // to fix this, i check if the counter > 0 || if we have a value to reload
    // the counter with that's >= MAX_SAMPLE_RATE.
    if (PSG.tone[index].counter > 0 || PSG.tone[index].tone >= MAX_SAMPLE_RATE)
    {
        if (PSG.tone[index].counter > 0)
        {
            PSG.tone[index].counter -= cycles;
        }

        if (PSG.tone[index].counter <= 0)
        {
            // the apu runs x16 slower than cpu!
            PSG.tone[index].counter += PSG.tone[index].tone * 16;

            /*
                from the docs:

                Sample playback makes use of a feature of the psg's tone generators:
                when the half-wavelength (tone value) is set to 1, they output a DC offset
                value corresponding to the volume level (i.e. the wave does not flip-flop).
                By rapidly manipulating the volume, a crude form of PCM is obtained.
            */
            // this effect is used by the sega intro in Tail's Adventure and
            // sonic tripple trouble.
            if (PSG.tone[index].tone != 1)
            {
                // change the polarity
                PSG.polarity[index] ^= 1;
            }
        }
    }
}

static FORCE_INLINE void tick_noise(struct SMS_Core* sms, const uint8_t cycles)
{
    PSG.noise.counter -= cycles;

    if (PSG.noise.counter <= 0)
    {
        // the drums sound much better in basically every game if
        // the timer is 16, 32, 64.
        // however, the correct sound is at 256, 512, 1024.
        const uint8_t multi = sms->better_drums ? 1 : 16;

        // the apu runs x16 slower than cpu!
        switch (PSG.noise.shift_rate)
        {
            case 0x0: PSG.noise.counter += 1 * 16 * multi; break;
            case 0x1: PSG.noise.counter += 2 * 16 * multi; break;
            case 0x2: PSG.noise.counter += 4 * 16 * multi; break;
            // note: if tone == 0, this will cause a lot of random noise!
            case 0x3: PSG.noise.counter += PSG.tone[2].tone * 16; break;
        }

        // change the polarity
        PSG.noise.flip_flop ^= 1;

        // the nose is clocked every 2 countdowns!
        if (PSG.noise.flip_flop == true)
        {
            // this is the bit used for the mixer
            PSG.polarity[3] = (PSG.noise.lfsr & 0x1);

            if (PSG.noise.mode == WHITE_NOISE)
            {
                PSG.noise.lfsr = (PSG.noise.lfsr >> 1) | (SMS_parity16(PSG.noise.lfsr & TAPPED_BITS) << 15);
            }
            else
            {
                PSG.noise.lfsr = (PSG.noise.lfsr >> 1) | ((PSG.noise.lfsr & 0x1) << 15);
            }
        }
    }
}

static FORCE_INLINE uint8_t sample_channel(struct SMS_Core* sms, const uint8_t index)
{
    // the volume is inverted, so that 0xF = OFF, 0x0 = MAX
    return PSG.polarity[index] * (0xF - PSG.volume[index]);
}

static void sample(struct SMS_Core* sms)
{	
	// generate samples
    const uint8_t tone0 = sample_channel(sms, 0);
    const uint8_t tone1 = sample_channel(sms, 1);
    const uint8_t tone2 = sample_channel(sms, 2);
    const uint8_t noise = sample_channel(sms, 3);

    // apply stereo
	unsigned long sum = 0;
	
    sum += tone0 * PSG.channel_enable[0][0];
    sum += tone0 * PSG.channel_enable[0][1];
    sum += tone1 * PSG.channel_enable[1][0];
    sum += tone1 * PSG.channel_enable[1][1];
    sum += tone2 * PSG.channel_enable[2][0];
	sum += tone2 * PSG.channel_enable[2][1];
    sum += noise * PSG.channel_enable[3][0];
	sum += noise * PSG.channel_enable[3][1];
	
	audio_buffer[audio_write] = (unsigned char)(sum & 0x00FF);
		
	audio_write++;
		
	if (audio_write >= AUDIO_LEN) audio_write = 0;
}

// this is called on psg_reg_write() and at the end of a frame
void psg_sync(struct SMS_Core* sms)
{
	sms->apu_callback_freq = AUDIO_FREQ;

    // psg is 16x slower than the cpu, so, it only makes sense to tick
    // each component at every 16 step.
    enum { STEP = 16 };

    // this loop will *not* cause PSG.cycles to underflow!
    for (; STEP <= PSG.cycles; PSG.cycles -= STEP)
    {
        tick_tone(sms, 0, STEP);
        tick_tone(sms, 1, STEP);
        tick_tone(sms, 2, STEP);
        tick_noise(sms, STEP);

        sms->apu_callback_counter += STEP;

        while (sms->apu_callback_counter >= sms->apu_callback_freq)
        {
            sms->apu_callback_counter -= sms->apu_callback_freq;
            sample(sms);
        }
    }
}

void psg_run(struct SMS_Core* sms, const uint8_t cycles)
{
    PSG.cycles += cycles; // PSG.cycles is an uint32_t, so it won't overflow
}

#endif // SMS_DISABLE_AUDIO

void psg_init(struct SMS_Core* sms)
{
    // by default, all channels are enabled in GG mode.
    // as sms is mono, these values will not be changed elsewhere
    // (so always enabled!).
	for (int i=0; i<4; i++)
	{
		PSG.polarity[i] = 1;
		PSG.volume[i] = 0xF;

		PSG.channel_enable[i][0] = 1;
		PSG.channel_enable[i][1] = 1;
    }

    PSG.noise.lfsr = LFSR_RESET_VALUE;
    PSG.noise.mode = 0;
    PSG.noise.shift_rate = 0;
    PSG.noise.flip_flop = true;

    PSG.latched_channel = 0;
}


// *** sms_vdp.c ***


#define VDP sms->vdp


typedef uint32_t pixel_width_t;

enum
{
    NTSC_HCOUNT_MAX = 342,
    NTSC_VCOUNT_MAX = 262,

    NTSC_FPS = 60,

    NTSC_CYCLES_PER_LINE = 228, // SMS_CPU_CLOCK / NTSC_VCOUNT_MAX / NTSC_FPS, 228

    SPRITE_EOF = 208,
};

// SOURCE: https://www.smspower.org/forums/8161-SMSDisplayTiming
// (divide mclks by 3)
// 59,736 (179,208 mclks, 228x262)
// frame_int 202 cycles into line 192 (607 mclks)
// line_int 202 cycles into triggering (608 mclks)

static FORCE_INLINE bool vdp_is_line_irq_wanted(const struct SMS_Core* sms)
{
    return IS_BIT_SET(VDP.registers[0x0], 4);
}

static FORCE_INLINE bool vdp_is_vblank_irq_wanted(const struct SMS_Core* sms)
{
    return IS_BIT_SET(VDP.registers[0x1], 5);
}

static FORCE_INLINE bool vdp_is_screen_size_change_enabled(const struct SMS_Core* sms)
{
    return IS_BIT_SET(VDP.registers[0x0], 1);
}

static FORCE_INLINE uint16_t vdp_get_nametable_base_addr(const struct SMS_Core* sms)
{
    if (SMS_is_system_type_sg(sms))
    {
        return (VDP.registers[0x2] & 0xF) << 10;
    }
    else
    {
        return (VDP.registers[0x2] & 0xE) << 10;
    }
}

static FORCE_INLINE uint16_t vdp_get_sprite_attribute_base_addr(const struct SMS_Core* sms)
{
    if (SMS_is_system_type_sg(sms))
    {
        return (VDP.registers[0x5] & 0x7F) * 128;
    }
    else
    {
        return (VDP.registers[0x5] & 0x7E) * 128;
    }
}

static FORCE_INLINE bool vdp_get_sprite_pattern_select(const struct SMS_Core* sms)
{
    return IS_BIT_SET(VDP.registers[0x6], 2);
}

static FORCE_INLINE bool vdp_is_display_enabled(const struct SMS_Core* sms)
{
    return IS_BIT_SET(VDP.registers[0x1], 6);
}

static FORCE_INLINE uint8_t vdp_get_sprite_height(const struct SMS_Core* sms)
{
    const bool doubled_sprites = IS_BIT_SET(VDP.registers[0x1], 0);
    const uint8_t sprite_size = IS_BIT_SET(VDP.registers[0x1], 1) ? 16 : 8;

    return sprite_size << doubled_sprites;
}

// returns the hieght of the screen
static uint16_t vdp_get_screen_height(const struct SMS_Core* sms)
{
    if (!vdp_is_screen_size_change_enabled(sms))
    {
        return 192;
    }
    else if (IS_BIT_SET(VDP.registers[1], 4))
    {
        return 224;
    }
    else if (IS_BIT_SET(VDP.registers[1], 3))
    {
        return 240;
    }
    else
    {
        return 192;
    }
}

static uint8_t vdp_get_overscan_colour(const struct SMS_Core* sms)
{
    return VDP.registers[0x7] & 0xF;
}

struct VDP_region
{
    uint16_t startx;
    uint16_t endx;
    uint16_t pixelx;
    uint16_t pixely;
};

static FORCE_INLINE struct VDP_region vdp_get_region(const struct SMS_Core* sms)
{
    if (SMS_is_system_type_gg(sms))
    {
        return (struct VDP_region)
        {
            .startx = 48,
            .endx = 160 + 48,
            .pixelx = 62 - 48,
            .pixely = VDP.vcount + 27,
        };
    }
    else
    {
        return (struct VDP_region)
        {
            .startx = IS_BIT_SET(VDP.registers[0x0], 5) ? 8 : 0,
            .endx = SMS_SCREEN_WIDTH,
            .pixelx = 0,
            .pixely = VDP.vcount,
        };
    }
}

static FORCE_INLINE bool vdp_is_display_active(const struct SMS_Core* sms)
{
    if (SMS_is_system_type_gg(sms))
    {
        return VDP.vcount >= 24 && VDP.vcount < 144 + 24;
    }
    else
    {
        return VDP.vcount < 192;
    }
}

static void vdp_write_scanline_to_frame(struct SMS_Core* sms, const pixel_width_t* scanline, const uint8_t y)
{
	if (screen_handheld == 0)
	{
		for (int i = 0; i < SMS_SCREEN_WIDTH; ++i)
		{
			screen_pixel_vga_raw(i, y, (uint8_t)scanline[i]);
		}
	}
	else
	{
		for (int i = 0; i < SMS_SCREEN_WIDTH; ++i)
		{
			screen_pixel_lcd_raw(i, y, (uint16_t)(((scanline[i]&0xE0) >> 3) | ((scanline[i]&0x1C) << 6) | ((scanline[i]&0x03) << 14)));
		}
	}
}

uint8_t vdp_status_flag_read(struct SMS_Core* sms)
{
    if (SMS_is_system_type_sg(sms))
    {
        uint8_t v = 0;

        v |= VDP.frame_interrupt_pending << 7;
        v |= VDP.sprite_overflow << 6;
        v |= VDP.sprite_collision << 5;
        v |= VDP.fifth_sprite_num;

        // these are reset on read
        VDP.frame_interrupt_pending = false;
        VDP.sprite_overflow = false;
        VDP.sprite_collision = false;
        VDP.fifth_sprite_num = 0;

        return v;
    }
    else
    {
        uint8_t v = 31; // unused bits 1,2,3,4

        v |= VDP.frame_interrupt_pending << 7;
        v |= VDP.sprite_overflow << 6;
        v |= VDP.sprite_collision << 5;

        // these are reset on read
        VDP.frame_interrupt_pending = false;
        VDP.line_interrupt_pending = false;
        VDP.sprite_overflow = false;
        VDP.sprite_collision = false;

        return v;
    }
}

void vdp_io_write(struct SMS_Core* sms, const uint8_t addr, const uint8_t value)
{
    if (SMS_is_system_type_sg(sms))
    {
        VDP.registers[addr & 0x7] = value;
    }
    else
    {
        switch (addr & 0xF)
        {
            case 0x0:
                //assert(IS_BIT_SET(value, 2) && "not mode4, using TMS9918 modes!");
                //assert(IS_BIT_SET(value, 1) && !IS_BIT_SET(VDP.registers[1], 3) && "240 height mode set");
                //assert(IS_BIT_SET(value, 1) && !IS_BIT_SET(VDP.registers[1], 4) && "224 height mode set");
                break;

            case 0x1:
                //assert(!IS_BIT_SET(value, 3) && IS_BIT_SET(VDP.registers[0], 1) && "240 height mode set");
                //assert(!IS_BIT_SET(value, 4) && IS_BIT_SET(VDP.registers[0], 1) && "224 height mode set");
                break;

            case 0x3:
                //assert(value == 0xFF && "colour table bits not all set");
                break;

            // unused registers
            case 0xB:
            case 0xC:
            case 0xD:
            case 0xE:
            case 0xF:
                return;
        }

        VDP.registers[addr & 0xF] = value;
    }
}

// same as i used in dmg / gbc rendering for gb
struct PriorityBuf
{
    bool array[SMS_SCREEN_WIDTH];
};

static void vdp_mode2_render_background(struct SMS_Core* sms, pixel_width_t* scanline)
{
    const uint8_t line = VDP.vcount;
    const uint8_t fine_line = line & 0x7;
    const uint8_t row = line >> 3;
    const uint8_t overscan_colour = vdp_get_overscan_colour(sms);

    const uint16_t pattern_table_addr = (VDP.registers[4] & 0x04) << 11;
    const uint16_t colour_map_addr = (VDP.registers[3] & 0x80) << 6;
    const uint16_t region = (VDP.registers[4] & 0x03) << 8;

    for (uint8_t col = 0; col < 32; col++)
    {
        const uint16_t tile_number = (row * 32) + col;
        const uint16_t name_tile_addr = vdp_get_nametable_base_addr(sms) + tile_number;
        const uint16_t name_tile = VDP.vram[name_tile_addr] | (region & 0x300 & tile_number);

        const uint8_t pattern_line = VDP.vram[pattern_table_addr + (name_tile * 8) + fine_line];
        const uint8_t color_line = VDP.vram[colour_map_addr + (name_tile * 8) + fine_line];

        const uint8_t bg_color = color_line & 0x0F;
        const uint8_t fg_color = color_line >> 4;

        for (uint8_t x = 0; x < 8; x++)
        {
            const uint8_t x_index = (col * 8) + x;

            uint8_t colour = IS_BIT_SET(pattern_line, 7 - x) ? fg_color : bg_color;
            colour = (colour > 0) ? colour : overscan_colour;

            scanline[x_index] = sms->vdp.colour[colour];
        }
    }
}

static void vdp_mode1_render_background(struct SMS_Core* sms, pixel_width_t* scanline)
{
    const uint8_t line = VDP.vcount;
    const uint8_t fine_line = line & 0x7;
    const uint8_t row = line >> 3;
    const uint8_t overscan_colour = vdp_get_overscan_colour(sms);

    const uint16_t name_table_addr = (VDP.registers[2] & 0x0F) << 10;
    const uint16_t pattern_table_addr = (VDP.registers[4] & 0x07) << 11;
    const uint16_t colour_map_addr = VDP.registers[3] << 6;

    for (uint8_t col = 0; col < 32; col++)
    {
        const uint16_t tile_number = (row * 32) + col;
        const uint16_t name_tile_addr = name_table_addr + tile_number;
        const uint16_t name_tile = VDP.vram[name_tile_addr];

        const uint8_t pattern_line = VDP.vram[pattern_table_addr + (name_tile * 8) + fine_line];
        const uint8_t color_line = VDP.vram[colour_map_addr + (name_tile >> 3)];

        const uint8_t bg_color = color_line & 0x0F;
        const uint8_t fg_color = color_line >> 4;

        for (uint8_t x = 0; x < 8; x++)
        {
            const uint8_t x_index = (col * 8) + x;

            uint8_t colour = IS_BIT_SET(pattern_line, 7 - x) ? fg_color : bg_color;
            colour = (colour > 0) ? colour : overscan_colour;

            scanline[x_index] = sms->vdp.colour[colour];
        }
    }
}

static inline struct CachedPalette vdp_get_palette(struct SMS_Core* sms, const uint16_t pattern_index)
{
    struct CachedPalette* cpal = &VDP.cached_palette[pattern_index >> 2];
	

    // check if one of the 4 bit_planes have changed
    if (VDP.dirty_vram[pattern_index >> 2])
    {
        VDP.dirty_vram[pattern_index >> 2] = 0;

        const uint8_t bit_plane0 = VDP.vram[pattern_index + 0];
        const uint8_t bit_plane1 = VDP.vram[pattern_index + 1];
        const uint8_t bit_plane2 = VDP.vram[pattern_index + 2];
        const uint8_t bit_plane3 = VDP.vram[pattern_index + 3];

        for (uint8_t x = 0; x < 8; ++x)
        {
            const uint8_t bit_flip = x;
            const uint8_t bit_norm = 7 - x;

            cpal->flipped <<= 4;
            cpal->flipped |= IS_BIT_SET(bit_plane0, bit_flip) << 0;
            cpal->flipped |= IS_BIT_SET(bit_plane1, bit_flip) << 1;
            cpal->flipped |= IS_BIT_SET(bit_plane2, bit_flip) << 2;
            cpal->flipped |= IS_BIT_SET(bit_plane3, bit_flip) << 3;

            cpal->normal <<= 4;
            cpal->normal |= IS_BIT_SET(bit_plane0, bit_norm) << 0;
            cpal->normal |= IS_BIT_SET(bit_plane1, bit_norm) << 1;
            cpal->normal |= IS_BIT_SET(bit_plane2, bit_norm) << 2;
            cpal->normal |= IS_BIT_SET(bit_plane3, bit_norm) << 3;
        }
    }

    return *cpal;
}

static void vdp_render_background(struct SMS_Core* sms, pixel_width_t* scanline, struct PriorityBuf* prio)
{
    const struct VDP_region region = vdp_get_region(sms);

    const uint8_t line = VDP.vcount;
    const uint8_t fine_line = line & 0x7;
    const uint8_t row = line >> 3;

    // check if horizontal scrolling should be disabled
    // doesn't work in GG mode as the screen centered
    const bool horizontal_disabled = IS_BIT_SET(VDP.registers[0x0], 6) && line < 16;

    const uint8_t starting_col = (32 - (VDP.registers[0x8] >> 3)) & 31;
    const uint8_t fine_scrollx = horizontal_disabled ? 0 : VDP.registers[0x8] & 0x7;
    const uint8_t horizontal_scroll = horizontal_disabled ? 0 : starting_col;

    const uint8_t starting_row = VDP.vertical_scroll >> 3;
    const uint8_t fine_scrolly = VDP.vertical_scroll & 0x7;

    const uint8_t* nametable = NULL;
    uint8_t check_col = 0;
    uint8_t palette_index_offset = 0;

    // if set, we use the internal col counter, else, the starting_col
    if (!IS_BIT_SET(VDP.registers[0x1], 7))
    {
        check_col = (starting_col) & 31;
    }

    {
        // we need to check if we cross the next row
        const bool next_row = (fine_line + fine_scrolly) > 7;

        const uint16_t vertical_offset = ((row + starting_row + next_row) % 28) * 64;
        palette_index_offset = (fine_line + fine_scrolly) & 0x7;
        nametable = &VDP.vram[vdp_get_nametable_base_addr(sms) + vertical_offset];
    }

    if (region.startx == 8)
    {
        // render overscan
        const uint8_t palette_index = 16 + vdp_get_overscan_colour(sms);

        for (int x_index = 0; x_index < 8; x_index++)
        {
            // used when sprite rendering, will skip if prio set and not pal0
            prio->array[x_index] = true;//priority && palette_index != 0;

            scanline[x_index] = VDP.colour[palette_index];
        }
    }

    for (uint8_t col = 0; col < 32; ++col)
    {
        check_col = (check_col + 1) & 31;
        const uint16_t horizontal_offset = ((horizontal_scroll + col) & 31) * 2;

        // check if vertical scrolling should be disabled
        if (IS_BIT_SET(VDP.registers[0x0], 7) && check_col >= 24)
        {
            const uint16_t vertical_offset = (row % 28) * 64;
            palette_index_offset = fine_line;
            nametable = &VDP.vram[vdp_get_nametable_base_addr(sms) + vertical_offset];
        }

        const uint16_t tile =
            nametable[horizontal_offset + 0] << 0 |
            nametable[horizontal_offset + 1] << 8 ;

        // if set, background will display over sprites
        const bool priority = IS_BIT_SET(tile, 12);
        // select from either sprite or background palette
        const bool palette_select = IS_BIT_SET(tile, 11);
        // vertical flip
        const bool vertical_flip = IS_BIT_SET(tile, 10);
        // horizontal flip
        const bool horizontal_flip = IS_BIT_SET(tile, 9);
        // one of the 512 patterns to select
        uint16_t pattern_index = (tile & 0x1FF) * 32;

        if (vertical_flip)
        {
            pattern_index += (7 - palette_index_offset) * 4;
        }
        else
        {
            pattern_index += palette_index_offset * 4;
        }

        const struct CachedPalette cpal = vdp_get_palette(sms, pattern_index);
        const uint32_t palette = horizontal_flip ? cpal.flipped : cpal.normal;
        const uint8_t pal_base = palette_select ? 16 : 0;

        for (uint8_t x = 0; x < 8; ++x)
        {
            const uint8_t x_index = ((col * 8) + x + fine_scrollx) % SMS_SCREEN_WIDTH;

            if (x_index >= region.endx)
            {
                break;
            }

            if (x_index < region.startx)
            {
                continue;
            }

            const uint8_t palette_index =  (palette >> (28 - (4 * x))) & 0xF;
            prio->array[x_index] = priority && palette_index != 0;

            scanline[x_index] = VDP.colour[pal_base + palette_index];
        }
    }
}

void SMS_get_pixel_region(const struct SMS_Core* sms, int* x, int* y, int* w, int* h)
{
    // todo: support different height modes
    if (SMS_is_system_type_gg(sms))
    {
        *x = 48;
        *y = 24;
        *w = 160;
        *h = 144;
    }
    else
    {
        *x = 0;
        *y = 0;
        *w = SMS_SCREEN_WIDTH;
        *h = SMS_SCREEN_HEIGHT;
    }
}

struct SgSpriteEntry
{
    int16_t y;
    int16_t x;
    uint8_t tile_num;
    uint8_t colour;
};

struct SgSpriteEntries
{
    struct SgSpriteEntry entry[32];
    uint8_t count;
};

static struct SgSpriteEntries vdp_parse_sg_sprites(struct SMS_Core* sms)
{
    if (!SMS_is_system_type_sg(sms))
    {
        //assert(IS_BIT_SET(VDP.registers[0x5], 0) && "needs lower index for oam");
        //assert((VDP.registers[0x6] & 0x3) == 0x3 && "Sprite Pattern Generator Base Address");
    }

    struct SgSpriteEntries sprites = {0};

    const uint8_t line = VDP.vcount;
    const uint16_t sprite_attribute_base_addr = vdp_get_sprite_attribute_base_addr(sms);
    const uint8_t sprite_size = vdp_get_sprite_height(sms);

    for (uint8_t i = 0; i < 128; i += 4)
    {
        // todo: find out how the sprite y wrapping works!
        int16_t y = VDP.vram[sprite_attribute_base_addr + i + 0] + 1;

        // special number used to stop sprite parsing!
        if (y == SPRITE_EOF + 1)
        {
            break;
        }

        if (y > 192)
        {
            y -= 256;
        }

        if (line >= y && line < (y + sprite_size))
        {
            const int16_t x = VDP.vram[sprite_attribute_base_addr + i + 1];
            const uint8_t tile_num = VDP.vram[sprite_attribute_base_addr + i + 2];
            const uint8_t colour = VDP.vram[sprite_attribute_base_addr + i + 3];

            sprites.entry[sprites.count].y = y;
            // note: docs say its shifted 32 pixels NOT 8!
            sprites.entry[sprites.count].x = x - (IS_BIT_SET(colour, 7) ? 32 : 0);
            sprites.entry[sprites.count].tile_num = sprite_size > 8 ? tile_num & ~0x3 : tile_num;
            sprites.entry[sprites.count].colour = colour & 0xF;
            sprites.count++;
        }
    }

    return sprites;
}

static void vdp_mode1_render_sprites(struct SMS_Core* sms, pixel_width_t* scanline)
{
    const uint8_t line = VDP.vcount;
    const uint16_t tile_addr = (VDP.registers[0x6] & 0x7) * 0x800;
    const uint8_t sprite_size = vdp_get_sprite_height(sms);
    const struct SgSpriteEntries sprites = vdp_parse_sg_sprites(sms);
    bool drawn_sprites[SMS_SCREEN_WIDTH] = {0};
    uint8_t sprite_rendered_count = 0;

    for (uint8_t i = 0; i < sprites.count; i++)
    {
        const struct SgSpriteEntry* sprite = &sprites.entry[i];

        if (sprite->colour == 0)
        {
            continue;
        }

        uint8_t pattern_line = VDP.vram[tile_addr + (line - sprite->y) + (sprite->tile_num * 8)];
        // check if we actually drawn a sprite
        bool did_draw_a_sprite = false;

        for (uint8_t x = 0; x < sprite_size; x++)
        {
            const int16_t x_index = x + sprite->x;

            // skip if column0 or offscreen
            if (x_index < 0)
            {
                continue;
            }

            if (x_index >= SMS_SCREEN_WIDTH)
            {
                break;
            }

            if (drawn_sprites[x_index])
            {
                VDP.sprite_collision = true;
                continue;
            }

            // idk what name to give this
            // basically, a sprite can be 32x32 but it doubles
            // up on the bits so that bit0 will be for pixel 0,1
            const uint8_t x2 = x >> (sprite_size == 32);

            if (x2 == 8)
            {
                // reload
                pattern_line = VDP.vram[tile_addr + (line - sprite->y) + (sprite->tile_num * 8) + 16];
            }

            if (!IS_BIT_SET(pattern_line, 7 - (x2 & 7)))
            {
                continue;
            }

            if (sprite_rendered_count < 4) // max of 4 sprites lol
            {
                did_draw_a_sprite = true;
                drawn_sprites[x_index] = true;

                scanline[x_index] = sms->vdp.colour[sprite->colour];
            }
        }

        // update count if we drawn a sprite
        sprite_rendered_count += did_draw_a_sprite;
        // now check if theres more than 5 spirtes on a line
        if (i >= 4)
        {
            VDP.sprite_overflow = true;
            VDP.fifth_sprite_num = i;
        }
    }
}

struct SpriteEntry
{
    int16_t y;
    uint8_t xn_index;
};

struct SpriteEntries
{
    struct SpriteEntry entry[8];
    uint8_t count;
};

static struct SpriteEntries vdp_parse_sprites(struct SMS_Core* sms)
{
    // //assert((VDP.registers[0x6] & 0x3) == 0x3 && "Sprite Pattern Generator Base Address");

    struct SpriteEntries sprites = {0};

    const uint8_t line = VDP.vcount;
    const uint16_t sprite_attribute_base_addr = vdp_get_sprite_attribute_base_addr(sms);
    const uint8_t sprite_attribute_x_index = IS_BIT_SET(VDP.registers[0x5], 0) ? 128 : 0;
    const uint8_t sprite_size = vdp_get_sprite_height(sms);

    for (uint8_t i = 0; i < 64; ++i)
    {
        // todo: find out how the sprite y wrapping works!
        int16_t y = VDP.vram[sprite_attribute_base_addr + i] + 1;

        // special number used to stop sprite parsing!
        if (y == SPRITE_EOF + 1)
        {
            break;
        }

        // docs (TMS9918.pdf) state that y is partially signed (-31, 0) which
        // is line 224 im not sure if this is correct, as it likely isn't as
        // theres a 240 hieght mode, meaning no sprites could be displayed past 224...
        // so maybe this should reset on 240? or maybe it depends of the
        // the height mode selected, ie, 192, 224 and 240
        #if 0
        if (y > 224)
        #else
        if (y > 192)
        #endif
        {
            y -= 256;
        }

        if (line >= y && line < (y + sprite_size))
        {
            if (sprites.count < 8)
            {
                sprites.entry[sprites.count].y = y;
                // xn values are either low (0) or high (128) end of SAT
                sprites.entry[sprites.count].xn_index = sprite_attribute_x_index + (i * 2);
                sprites.count++;
            }

            // if we have filled the sprite array, we need to keep checking further
            // entries just in case another sprite falls on the same line, in which
            // case, the sprite overflow flag is set for stat.
            else
            {
                if (!SMS_is_system_type_sg(sms))
                {
                    VDP.sprite_overflow = true;
                }
                break;
            }
        }
    }

    return sprites;
}

static void vdp_render_sprites(struct SMS_Core* sms, pixel_width_t* scanline, const struct PriorityBuf* prio)
{
    const struct VDP_region region = vdp_get_region(sms);

    const uint8_t line = VDP.vcount;
    const uint16_t attr_addr = vdp_get_sprite_attribute_base_addr(sms);
    // if set, we fetch patterns from upper table
    const uint16_t pattern_select = vdp_get_sprite_pattern_select(sms) ? 256 : 0;
    // if set, sprites start 8 to the left
    const int8_t sprite_x_offset = IS_BIT_SET(VDP.registers[0x0], 3) ? -8 : 0;

    const struct SpriteEntries sprites = vdp_parse_sprites(sms);

    bool drawn_sprites[SMS_SCREEN_WIDTH] = {0};

    for (uint8_t i = 0; i < sprites.count; ++i)
    {
        const struct SpriteEntry* sprite = &sprites.entry[i];

        // signed because the sprite can be negative if -8!
        const int16_t sprite_x = VDP.vram[attr_addr + sprite->xn_index + 0] + sprite_x_offset;

        if (sprite_x+8 < region.startx || sprite_x>=region.endx)
        {
            continue;
        }

        uint16_t pattern_index = VDP.vram[attr_addr + sprite->xn_index + 1] + pattern_select;

        // docs state that if bit1 of reg1 is set (should always), bit0 is ignored
        // i am not sure if this is applied to the final value of the value
        // initial value however...
        if (IS_BIT_SET(VDP.registers[0x1], 1))
        {
            pattern_index &= ~0x1;
        }

        pattern_index *= 32;

        // this has already taken into account of sprite size when parsing
        // sprites, so for example:
        // - line = 3,
        // - sprite->y = 1,
        // - sprite_size = 8,
        // then y will be accepted for this line, but needs to be offset from
        // the current line we are on, so line-sprite->y = 2
        pattern_index += (line - sprite->y) * 4;

        const struct CachedPalette cpal = vdp_get_palette(sms, pattern_index);
        const uint32_t palette = cpal.normal;//horizontal_flip ? cpal.flipped : cpal.normal;

        // note: the order of the below ifs are important.
        // opaque sprites can collide, even when behind background/
        for (uint8_t x = 0; x < 8; ++x)
        {
            const int16_t x_index = x + sprite_x;

            // skip if column0 or offscreen
            if (x_index < region.startx)
            {
                continue;
            }

            if (x_index >= region.endx)
            {
                break;
            }

            const uint8_t palette_index = (palette >> (28 - (4 * x))) & 0xF;

            // for sprites, pal0 is transparent
            if (palette_index == 0)
            {
                continue;
            }

            // skip if we already rendered a sprite!
            if (drawn_sprites[x_index])
            {
                VDP.sprite_collision = true;
                continue;
            }

            // keep track of this sprite already being rendered
            drawn_sprites[x_index] = true;

            // skip is bg has priority
            if (prio->array[x_index])
            {
                continue;
            }

            // sprite cram index is the upper 16-bytes!
            scanline[x_index] = VDP.colour[palette_index + 16];
        }
    }
}

static void vdp_update_sms_colours(struct SMS_Core* sms)
{
    //assert(sms->vdp.dirty_cram_max <= 32);

    for (int i = sms->vdp.dirty_cram_min; i < sms->vdp.dirty_cram_max; i++)
    {
        if (sms->vdp.dirty_cram[i])
        {
            const uint8_t r = (sms->vdp.cram[i] >> 0) & 0x3;
            const uint8_t g = (sms->vdp.cram[i] >> 2) & 0x3;
            const uint8_t b = (sms->vdp.cram[i] >> 4) & 0x3;

			uint32_t val = (uint32_t)(((r&0x03) << 6) | ((g&0x03) << 3) | ((b&0x03)));

            sms->vdp.colour[i] = val;
            sms->vdp.dirty_cram[i] = false;
        }
    }

    sms->vdp.dirty_cram_min = sms->vdp.dirty_cram_max = 0;
}

static void vdp_update_gg_colours(struct SMS_Core* sms)
{
    for (int i = sms->vdp.dirty_cram_min; i < sms->vdp.dirty_cram_max; i += 2)
    {
        if (sms->vdp.dirty_cram[i])
        {
            // GG colours are in [----BBBBGGGGRRRR] format
            const uint8_t r = (sms->vdp.cram[i + 0] >> 0) & 0xF;
            const uint8_t g = (sms->vdp.cram[i + 0] >> 4) & 0xF;
            const uint8_t b = (sms->vdp.cram[i + 1] >> 0) & 0xF;

			uint32_t val = (uint32_t)(((r&0x0E) << 4) | ((g&0x0E) << 1) | ((b&0x0C) >> 2));

            // only 32 colours, 2 bytes per colour!
            sms->vdp.colour[i >> 1] = val;
            sms->vdp.dirty_cram[i] = false;
        }
    }

    sms->vdp.dirty_cram_min = sms->vdp.dirty_cram_max = 0;
}

static void vdp_update_sg_colours(struct SMS_Core* sms)
{
    struct Colour { uint8_t r,g,b; };
    // https://www.smspower.org/uploads/Development/sg1000.txt
    static const struct Colour SG_COLOUR_TABLE[] =
    {
        {0x00, 0x00, 0x00}, // 0: transparent
        {0x00, 0x00, 0x00}, // 1: black
        {0x20, 0xC0, 0x20}, // 2: green
        {0x60, 0xE0, 0x60}, // 3: bright green
        {0x20, 0x20, 0xE0}, // 4: blue
        {0x40, 0x60, 0xE0}, // 5: bright blue
        {0xA0, 0x20, 0x20}, // 6: dark red
        {0x40, 0xC0, 0xE0}, // 7: cyan (?)
        {0xE0, 0x20, 0x20}, // 8: red
        {0xE0, 0x60, 0x60}, // 9: bright red
        {0xC0, 0xC0, 0x20}, // 10: yellow
        {0xC0, 0xC0, 0x80}, // 11: bright yellow
        {0x20, 0x80, 0x20}, // 12: dark green
        {0xC0, 0x40, 0xA0}, // 13: pink
        {0xA0, 0xA0, 0xA0}, // 14: gray
        {0xE0, 0xE0, 0xE0}, // 15: white
    };

    // the dirty_* values are only set on romload and
    // loadstate. so we can check this value and if set
    // then update the colours.
    if (sms->vdp.dirty_cram_max == 0)
    {
        return;
    }

    for (int i = 0; i < 16; i++)
    {
        const struct Colour c = SG_COLOUR_TABLE[i];

		uint32_t val = (uint32_t)(((c.r&0xE0)) | ((c.g&0xE0) >> 3) | ((c.b&0xC0) >> 6));

        sms->vdp.colour[i] = val;
    }

    sms->vdp.dirty_cram_min = sms->vdp.dirty_cram_max = 0;
}

static void vdp_update_palette(struct SMS_Core* sms)
{
    switch (SMS_get_system_type(sms))
    {
        case SMS_System_SMS:
            vdp_update_sms_colours(sms);
            break;
        case SMS_System_GG:
            vdp_update_gg_colours(sms);
            break;
        case SMS_System_SG1000:
            vdp_update_sg_colours(sms);
            break;
    }
}

void vdp_mark_palette_dirty(struct SMS_Core* sms)
{
    sms->vdp.dirty_cram_min = 0;

    if (SMS_is_system_type_gg(sms))
    {
        sms->vdp.dirty_cram_max = 64;
    }
    else
    {
        sms->vdp.dirty_cram_max = 32;
    }

    vdp_update_palette(sms);
}

bool vdp_has_interrupt(const struct SMS_Core* sms)
{
    const bool frame_interrupt = VDP.frame_interrupt_pending && vdp_is_vblank_irq_wanted(sms);
    const bool line_interrupt = VDP.line_interrupt_pending && vdp_is_line_irq_wanted(sms);

    return frame_interrupt || line_interrupt;
}

static void vdp_advance_line_counter(struct SMS_Core* sms)
{
    // i don't think sg has line interrupt
    if (!SMS_is_system_type_sg(sms))
    {
        VDP.line_counter--;

        // reloads / fires irq (if enabled) on underflow
        if (VDP.line_counter == 0xFF)
        {
            VDP.line_interrupt_pending = true;
            VDP.line_counter = VDP.registers[0xA];
        }
    }
}

static void vdp_render_frame(struct SMS_Core* sms)
{
	// only render if display is enabled
	if (!vdp_is_display_enabled(sms))
	{
		// on sms/gg, sprite overflow still happens with display disabled
		if (!SMS_is_system_type_sg(sms))
		{
			vdp_parse_sprites(sms);
		}
		return;
	}
	// exit early if we have no pixels (this will break games that need sprite overflow and collision)
	if (sms->skip_frame) // || !sms->pixels)
	{
		return;
	}
	
	if (frame_tally == 2) // every three frames
	{
		struct PriorityBuf prio = {0};
		pixel_width_t scanline[SMS_SCREEN_WIDTH] = {0};

		if (SMS_is_system_type_sg(sms))
		{
			// this isn't correct, but it works :)
			if ((VDP.registers[0] & 0x7) == 0)
			{
				vdp_mode1_render_background(sms, scanline);
			}
			else
			{
				vdp_mode2_render_background(sms, scanline);
			}

			vdp_mode1_render_sprites(sms, scanline);
		}
		else // sms / gg render
		{
			vdp_render_background(sms, scanline, &prio);
			vdp_render_sprites(sms, scanline, &prio);
		}
	
		vdp_write_scanline_to_frame(sms, scanline, VDP.vcount);
	}
}

static void vdp_tick(struct SMS_Core* sms)
{
    if (LIKELY(vdp_is_display_active(sms)))
    {
        vdp_update_palette(sms);
        vdp_render_frame(sms);
    }
    else
    {
        VDP.vertical_scroll = VDP.registers[0x9];
    }

    // advance line counter on lines 0-191 and 192
    if (LIKELY(VDP.vcount <= 192))
    {
        vdp_advance_line_counter(sms);
    }
    else
    {
        VDP.line_counter = VDP.registers[0xA];
    }

    if (VDP.vcount == 192) // vblank. TODO: support diff hieght modes
    {
        SMS_skip_frame(sms, false);
        VDP.frame_interrupt_pending = true;

		frame_tally++;
    }

    if (VDP.vcount == 193 && SMS_is_spiderman_int_hack_enabled(sms) && vdp_is_vblank_irq_wanted(sms)) // hack for spiderman, will remove soon
    {
        z80_irq(sms);
    }

    if (VDP.vcount == 218) // see description in types.h for the jump back value
    {
        VDP.vcount_port = 212;
    }

    if (VDP.vcount == 261) // end of frame. TODO: support pal height
    {
        VDP.vcount = 0;
        VDP.vcount_port = 0;
    }
    else
    {
        VDP.vcount++;
        VDP.vcount_port++;
    }
}

void vdp_run(struct SMS_Core* sms, const uint8_t cycles)
{
    VDP.cycles += cycles;

    if (UNLIKELY(VDP.cycles >= NTSC_CYCLES_PER_LINE))
    {
        VDP.cycles -= NTSC_CYCLES_PER_LINE;
        vdp_tick(sms);
    }
}

void vdp_init(struct SMS_Core* sms)
{	
    // update palette
    vdp_mark_palette_dirty(sms);

    // values on starup
    VDP.registers[0x0] = 0x04; // %00000100 (taken from VDPTEST)
    VDP.registers[0x1] = 0x20; // %00100000 (taken from VDPTEST)
    VDP.registers[0x2] = 0xF1; // %11110001 (taken from VDPTEST)
    VDP.registers[0x3] = 0xFF; // %11111111 (taken from VDPTEST)
    VDP.registers[0x4] = 0x03; // %00000011 (taken from VDPTEST)
    VDP.registers[0x5] = 0x81; // %10000001 (taken from VDPTEST)
    VDP.registers[0x6] = 0xFB; // %11111011 (taken from VDPTEST)
    VDP.registers[0x7] = 0x00; // %00000000 (taken from VDPTEST)
    VDP.registers[0x8] = 0x00; // %00000000 (taken from VDPTEST)
    VDP.registers[0x9] = 0x00; // %00000000 (taken from VDPTEST)
    VDP.registers[0xA] = 0xFF; // %11111111 (taken from VDPTEST)
    // vdp registers are write-only, so the the values of 0xB-0xF don't matter

    if (1) // values after bios (todo: optional bios skip)
    {
        VDP.registers[0x0] = 0x36;
        VDP.registers[0x1] = 0x80;
        VDP.registers[0x6] = 0xFB;
    }

    VDP.line_counter = 0xFF;
}


// *** main.c ***



bool SMS_load_rom_data(unsigned char type, volatile uint8_t* data, size_t size)
{
	int system_hint = -1;

	if (type == 0)
	{
		system_hint = SMS_System_SMS;
    }
	else if (type == 1)
	{
		system_hint = SMS_System_GG;
   	}
	else if (type == 2)
	{
		system_hint = SMS_System_SG1000;
	}

    if (!SMS_loadrom(&sms, data, size, system_hint))
    {
        return false;
    }

	return true;
}

unsigned char sms_write_cart_ram_file(char filename[16])
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

unsigned char sms_read_cart_ram_file(char filename[16])
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

void TotalSMS(unsigned char type)
{	
	audio_bank = 0; // works like NES
	
	unsigned long size = 1048576; // 1 MB
	
    for (unsigned long i=0x0FFFFF; i>=0x0000FF; i--)
	{
		if (cart_rom[i] != 0xFF)
		{
			break;
		}
		
		size--;
	}
	
	if (size <= 32768) size = 32768;
	else if (size <= 65536) size = 65536;
	else if (size <= 131072) size = 131072;
	else if (size <= 262144) size = 262144;
	else if (size <= 524288) size = 524288;
	else size = 524288; // max 512 KB
	
	SMS_init(&sms);

	if (!SMS_load_rom_data(type, cart_rom, size))
	{
		SendString("Error loading SMS/GG/SG ROM!\\");

		while (1) { }
	}

	while (1)
	{		
		if (reset_check > 0)
		{
			reset_check = 0;
			
			SMS_reset(&sms);
			
			SMS_init(&sms);

			if (!SMS_load_rom_data(type, cart_rom, size))
			{
				SendString("Error loading SMS/GG/SG ROM!\\");

				while (1) { }
			}
			
			DelayMS(1000);
		}
		
		SMS_set_port_a(&sms, JOY1_UP_BUTTON, false);
		SMS_set_port_a(&sms, JOY1_DOWN_BUTTON, false);
		SMS_set_port_a(&sms, JOY1_LEFT_BUTTON, false);
		SMS_set_port_a(&sms, JOY1_RIGHT_BUTTON, false);
		SMS_set_port_a(&sms, JOY1_A_BUTTON, false);
		SMS_set_port_a(&sms, JOY1_B_BUTTON, false);
		
		SMS_set_port_a(&sms, JOY2_UP_BUTTON, false);
		SMS_set_port_a(&sms, JOY2_DOWN_BUTTON, false);
		SMS_set_port_b(&sms, JOY2_LEFT_BUTTON, false);
		SMS_set_port_b(&sms, JOY2_RIGHT_BUTTON, false);
		SMS_set_port_b(&sms, JOY2_A_BUTTON, false);
		SMS_set_port_b(&sms, JOY2_B_BUTTON, false);
		
		SMS_set_port_b(&sms, RESET_BUTTON, false);
		SMS_set_port_b(&sms, PAUSE_BUTTON, false);
		
		if ((controller_status_1 & 0x10) == 0x10) SMS_set_port_a(&sms, JOY1_UP_BUTTON, true);
		if ((controller_status_1 & 0x20) == 0x20) SMS_set_port_a(&sms, JOY1_DOWN_BUTTON, true);
		if ((controller_status_1 & 0x40) == 0x40) SMS_set_port_a(&sms, JOY1_LEFT_BUTTON, true);
		if ((controller_status_1 & 0x80) == 0x80) SMS_set_port_a(&sms, JOY1_RIGHT_BUTTON, true);
		if ((controller_status_1 & 0x01) == 0x01) SMS_set_port_a(&sms, JOY1_B_BUTTON, true); // button 2
		if ((controller_status_1 & 0x02) == 0x02) SMS_set_port_a(&sms, JOY1_A_BUTTON, true); // button 1
		if (button_disable == 0)
		{
			if ((controller_status_1 & 0x04) == 0x04) SMS_set_port_b(&sms, RESET_BUTTON, true); // select
			if ((controller_status_1 & 0x08) == 0x08) SMS_set_port_b(&sms, PAUSE_BUTTON, true); // start
		}
		
		if ((controller_status_2 & 0x10) == 0x10) SMS_set_port_a(&sms, JOY2_UP_BUTTON, true);
		if ((controller_status_2 & 0x20) == 0x20) SMS_set_port_a(&sms, JOY2_DOWN_BUTTON, true);
		if ((controller_status_2 & 0x40) == 0x40) SMS_set_port_b(&sms, JOY2_LEFT_BUTTON, true);
		if ((controller_status_2 & 0x80) == 0x80) SMS_set_port_b(&sms, JOY2_RIGHT_BUTTON, true);
		if ((controller_status_2 & 0x01) == 0x01) SMS_set_port_b(&sms, JOY2_B_BUTTON, true); // button 2
		if ((controller_status_2 & 0x02) == 0x02) SMS_set_port_b(&sms, JOY2_A_BUTTON, true); // button 1
		if (button_disable == 0)
		{
			if ((controller_status_2 & 0x04) == 0x04) SMS_set_port_b(&sms, RESET_BUTTON, true); // select
			if ((controller_status_2 & 0x08) == 0x08) SMS_set_port_b(&sms, PAUSE_BUTTON, true); // start
		}

		SMS_run(&sms, SMS_CYCLES_PER_FRAME);

		if (frame_tally == 3) // only draw every three frames
		{
			frame_tally = 0;
			screen_flip();
		}
		
		// speed limiter for when occasionally the SMS/GG/SG is too fast
		while (screen_sync == 0) { }
		
		screen_sync = 0;
	}
}



//-------------------------------------------------------------------------
//-------------------------------------------------------------------------
#include "barrier.h"
#include "rpi-gpio.h"
#include <circle/synchronize.h>
#include "RomType.h"

#define V8

#ifdef V1 

#define RPMV_D0	(1 << 0)
#define MD00_PIN	0
#define RA08	(1 << 8)
#define RA09	(1 << 9)
#define RA10	(1 << 10)
#define RA11	(1 << 11)
#define RA12	(1 << 12)
#define RA13	(1 << 13)
#define RA14	(1 << 14)
#define RA15	(1 << 15)
#define LE_A	(1 << 16)
#define LE_B	(1 << 17)
#define LE_D	(1 << 26)
#define RCLK	(1 << 25)
#define RST0	(1 << 19)
#define RST		(1 << 23)
#define RINT	(1 << 20)
#define RBUSDIR (1 << 22)
#define RWAIT	(1 << 24)
#define RW		(1 << 19)

#define SLTSL	RA11
#define MREQ	RA10
#define IORQ	RA12
#define RD		RA08
#define WR		RA14
#define RESET	RA13
#define BUSDIR  0
#define INT		0
#define WAIT	RST

#elif defined (V2)

#define RA08	(1 << 8)
#define RA09	(1 << 9)
#define RA10	(1 << 10)
#define RA11	(1 << 11)
#define RA12	(1 << 12)
#define RA13	(1 << 13)
#define RA14	(1 << 14)
#define RA15	(1 << 15)
#define RC16	(1 << 16)
#define RC17	(1 << 17)
#define RC18	(1 << 18)
#define RC19	(1 << 19)
#define RC20	(1 << 20)
#define RC21	(1 << 21)
#define RC22	(1 << 22)
#define RC23	(1 << 23)
#define RC24	(1 << 24)
#define RC25	(1 << 25)
#define RC26	(1 << 26)
#define RC27	(1 << 27)

#define SLTSL	RC16
#define IORQ	RC17
#define RESET	RC18
#define RD		RC19
#define MREQ	RC20
#define WAIT	RC21
#define INT		RC22
#define BUSDIR	RC23
#define LE_A	RC27
#define RW		RC25
#define LE_B	RC24
#define DIR_A	RC26

#elif defined (V8)

#define RA08	(1 << 8)
#define RA09	(1 << 9)
#define RA10	(1 << 10)
#define RA11	(1 << 11)
#define RA12	(1 << 12)
#define RA13	(1 << 13)
#define RA14	(1 << 14)
#define RA15	(1 << 15)
#define RC16	(1 << 16)
#define RC17	(1 << 17)
#define RC18	(1 << 18)
#define RC19	(1 << 19)
#define RC20	(1 << 20)
#define RC21	(1 << 21)
#define RC22	(1 << 22)
#define RC23	(1 << 23)
#define RC24	(1 << 24)
#define RC25	(1 << 25)
#define RC26	(1 << 26)
#define RC27	(1 << 27)

#define RESET	RC16
#define DAT_EN	RC16
#define SLTSL	RC17
#define SNDOUT	RC18
#define IORQ	RC19
#define RD 		RC20
#define BUSDIR	RC21
#define ADDR	RC22
#define INT  	RC23
#define WAIT	RC24
#define DAT_DIR	RC25
#define MREQ	RC26
#define WR		RC27

#endif 

#define PARTITION	"emmc1-1"

extern unsigned char ROM[];
extern int getRomSize();

extern "C" {
extern void start_l1cache();
}
volatile unsigned int* gpio = (unsigned int*)GPIO_BASE;
volatile unsigned int* pads = (unsigned int*)BCM2835_GPIO_PADS;
volatile unsigned int* gpio0 = (unsigned int*)(GPIO_BASE+GPIO_GPLEV0*4);
volatile unsigned int* gpio1 = (unsigned int*)(GPIO_BASE+GPIO_GPSET0*4);
volatile unsigned int* gpio2 = (unsigned int*)(GPIO_BASE+GPIO_GPCLR0*4);

typedef unsigned char byte;
typedef unsigned short word;

static enum RomType guessRomType(const unsigned char * rom, int size)
{
	if (size == 0) {
		return ROM_NORMAL;
	}
	const byte* data = &rom[0];

	if (size < 0x10000) {
		if ((size <= 0x4000) &&
		           (data[0] == 'A') && (data[1] == 'B')) {
			word initAddr = data[2] + 256 * data[3];
			word textAddr = data[8] + 256 * data[9];
			if ((textAddr & 0xC000) == 0x8000) {
				if ((initAddr == 0) ||
				    (((initAddr & 0xC000) == 0x8000) &&
				     (data[initAddr & (size - 1)] == 0xC9))) {
					return ROM_PAGE2;
				}
			}
		}
		// not correct for Konami-DAC, but does this really need
		// to be correct for _every_ rom?
		return ROM_MIRRORED;
	} else if (size == 0x10000 && !((data[0] == 'A') && (data[1] == 'B'))) {
		// 64 kB ROMs can be plain or memory mapped...
		// check here for plain, if not, try the auto detection
		// (thanks for the hint, hap)
		return ROM_MIRRORED;
	} else {
		//  GameCartridges do their bankswitching by using the Z80
		//  instruction ld(nn),a in the middle of program code. The
		//  adress nn depends upon the GameCartridge mappertype used.
		//  To guess which mapper it is, we will look how much writes
		//  with this instruction to the mapper-registers-addresses
		//  occur.

		unsigned typeGuess[ROM_END_OF_UNORDERED_LIST] = {}; // 0-initialized
		for (int i = 0; i < size - 3; ++i) {
			if (data[i] == 0x32) {
				word value = data[i + 1] + (data[i + 2] << 8);
				switch (value) {
				case 0x4001:
					typeGuess[ROM_ZEMINA126IN1]++;
					break;
				case 0x5000:
				case 0x9000:
				case 0xb000:
					typeGuess[ROM_KONAMI_SCC]++;
					break;
				case 0x4000:
					typeGuess[ROM_KONAMI]++;
					typeGuess[ROM_ZEMINA126IN1]++;
					break;
				case 0x8000:
				case 0xa000:
					typeGuess[ROM_KONAMI]++;
					break;
				case 0x6800:
				case 0x7800:
					typeGuess[ROM_ASCII8]++;
					break;
				case 0x6000:
					typeGuess[ROM_KONAMI]++;
					typeGuess[ROM_ASCII8]++;
					typeGuess[ROM_ASCII16]++;
					break;
				case 0x7000:
					typeGuess[ROM_KONAMI_SCC]++;
					typeGuess[ROM_ASCII8]++;
					typeGuess[ROM_ASCII16]++;
					break;
				case 0x77ff:
					typeGuess[ROM_ASCII16]++;
					break;
				}
			}
		}
		if (typeGuess[ROM_ASCII8]) typeGuess[ROM_ASCII8]--; // -1 -> max_int
		enum RomType type = ROM_GENERIC_8KB;
		for (int i = 0; i < ROM_END_OF_UNORDERED_LIST; ++i) {
			// debug: fprintf(stderr, "%d: %d\n", i, typeGuess[i]);
			if (typeGuess[i] && (typeGuess[i] >= typeGuess[type])) {
				type = (RomType) i;
			}
		}
		// in case of doubt we go for type ROM_GENERIC_8KB
		// in case of even type ROM_ASCII16 and ROM_ASCII8 we would
		// prefer ROM_ASCII16 but we would still prefer ROM_GENERIC_8KB
		// above ROM_ASCII8 or ROM_ASCII16
		if ((type == ROM_ASCII16) &&
		    (typeGuess[ROM_GENERIC_8KB] == typeGuess[ROM_ASCII16])) {
			type = ROM_GENERIC_8KB;
		}
#if 0		
		for(int i =0 ; i < ROM_END_OF_UNORDERED_LIST; i++)
		{
			if (typeGuess[i] > 0)
				printf("%s: %d\n", ROMTYPE[i], typeGuess[i]);
		}
#endif		
		return type;
	}
}

#if 0
void PUT32 ( unsigned int a, unsigned int b)
{
	*(unsigned int *)a = b;
}
unsigned int GET32 ( unsigned int a)
{
	return *(unsigned int *)a;
}

unsigned int GPIO_GET(void)
{
	return gpio[GPIO_GPLEV0];
}

void GPIO_SET(unsigned int b)
{
	gpio[GPIO_GPSET0] = b;
}

void GPIO_CLR(unsigned int b)
{
	gpio[GPIO_GPCLR0] = b;
}

void GPIO_PUT(unsigned int a, unsigned int b)
{
	gpio[GPIO_GPSET0] = a;
	gpio[GPIO_GPCLR0] = b;
}
#else
#define GPIO_GET() gpio[GPIO_GPLEV0]
#define GPIO_SET(a) *gpio1 = a
#define GPIO_CLR(a) *gpio2 = a
#endif

#include <circle/startup.h>

class Mapper {
public:
	Mapper(void) {};
	unsigned char *ROM;
	Mapper(unsigned char *rom)
	{
		setROM(rom);
	}
	void setROM(unsigned char *rom)
	{
		ROM = rom;
	}
	virtual unsigned char getByte(unsigned short addr) = 0;
	virtual void setBank(unsigned short addr, unsigned char byte) = 0;
	void mapping(void)
	{
		register int sltsl = 0;
		register unsigned int g;
		register unsigned short addr = 0;
		register unsigned char byte;
//		register int p;
		while(1)
		{
			g = GPIO_GET();
			if (!(g & (SLTSL | RD)))
			{
				if (sltsl == 0)
				{
					GPIO_CLR(DAT_EN | DAT_DIR);
				}
				if (sltsl == 1)
				{
					addr = g;
	#if 0
					p = page2[(addr & 0x8000) > 0] | (addr & 0x3fff);
					byte = ROM[p];
					GPIO_SET(byte| ADDR);
	#else
					GPIO_SET(getByte(addr) | ADDR);
	#endif
				}
				sltsl++;
			}
			else if (!(g & (SLTSL | WR)))
			{
				if (sltsl == 0)
				{
					addr = g;
					GPIO_SET(ADDR);
					GPIO_CLR(DAT_EN);
				}
				else if (sltsl == 2)
				{
					byte = g;//GPIO_GET();
	#if 0				
					if (addr == 0x6000)
					{
						page2[0] = byte << 14;
	//							dmb();
					}
					else if (addr == 0x7000)
					{
						page2[1] = byte << 14;
	//							dmb();
					}
	//						GPIO_SET(DAT_EN);
	#else
					setBank(addr, byte);
	#endif			
				}
				sltsl++;
			}
			else if (g & SLTSL && sltsl)
			{
				GPIO_SET(DAT_EN | DAT_DIR);
				GPIO_CLR(ADDR | 0xffff);
				break;
			}
			else if (!(g & IORQ))
			{
				
			}
		}		
	}
};
class MapperAscii16 : public Mapper {
private:
	int page2[2] = {0, 0};
	
public:
	MapperAscii16(void) {};
	MapperAscii16(unsigned char *rom) : Mapper(rom) {};
	void setROM(unsigned char *rom);
	unsigned char getByte(unsigned short addr)
	{
		return ROM[page2[(addr & 0x8000)>0] | (addr & 0x3fff)];
	}
	void setBank(unsigned short addr, unsigned char byte)
	{
		if (addr == 0x6000)
		{
			page2[0] = byte << 14;
		}
		else if (addr == 0x7000 || addr == 0x77ff)
		{
			page2[1] = byte << 14;
		}
	}
};
class MapperAscii8 : Mapper {
private:
	int page4[4] = {0, 1, 2, 3};
	
public:
	void setROM(unsigned char *rom);
	unsigned char getByte(unsigned short addr)
	{
		return ROM[page4[((addr & 0xe000) >> 12)-4] | (addr & 0x1fff)];
	}
	void setBank(unsigned short addr, unsigned char byte)
	{
		if (addr >= 0x6000 && addr < 0x6800)
		{
			page4[0] = byte << 13;
		}
		else if (addr >= 0x6800 && addr < 0x7000)
		{
			page4[1] = byte << 13;
		}
		else if (addr >= 0x7000 && addr < 0x7800)
		{
			page4[2] = byte << 13;
		}
		else if (addr >= 0x7800 && addr < 0x8000)
		{
			page4[3] = byte << 13;
		}
	}
};

class MapperKonami5 : Mapper {
private:
	int page4[6] = {0, 0, 0, 1, 2, 3};
	
public:
	void setROM(unsigned char *rom);
	unsigned char getByte(unsigned short addr)
	{
		return ROM[page4[((addr & 0xe000) >> 13)] | (addr & 0x1fff)];
	}
	void setBank(unsigned short addr, unsigned char byte)
	{
		if (addr >= 0x5000 && addr < 0x5800)
		{
			page4[0] = byte << 13;
		}
		else if (addr >= 0x7000 && addr < 0x7800)
		{
			page4[1] = byte << 13;
		}
		else if (addr >= 0x9000 && addr < 0x9800)
		{
			page4[2] = byte << 13;
		}
		else if (addr >= 0xb000 && addr < 0xb800)
		{
			page4[3] = byte << 13;
		}
	}
};

class NoMapper : public Mapper {
public:
	NoMapper(void) {};
	NoMapper(unsigned char *rom) : Mapper(rom) {};
	void setROM(unsigned char *rom);
	unsigned char getByte(unsigned short addr)
	{
		if (addr >= 0x4000 && addr < 0xc000)
			return ROM[addr - 0x4000];
		else
			return 0xff;
	}
	void setBank(unsigned short addr, unsigned char byte) {} ;
};

class MapperKonami4 : public Mapper {
private:
	int page4[6] = {0, 0, 0, 1, 2, 3};
	
public:
	MapperKonami4(void) {};
	MapperKonami4(unsigned char *rom) : Mapper(rom) {};
	unsigned char getByte(unsigned short addr)
	{
		return ROM[page4[((addr & 0xe000) >> 13)] | (addr & 0x1fff)];
	}
	void setBank(unsigned short addr, unsigned char byte)
	{
		if (addr >= 0x6000 && addr < 0xc000)
		{
			page4[(addr & 0xe000) >> 13] = byte << 13;
		}
	}
};

class MapperGameMaster2 : Mapper {
private:
	int page4[4];
	unsigned char RAM[0x2000];
	
public:
	unsigned char getByte(unsigned short addr)
	{
		if (addr >= 0xb000 && addr < 0xbfff)
			return RAM[addr - 0xb000];
		else
			return ROM[page4[((addr & 0xe000) >> 12)-4] | (addr & 0x1fff)];
	}
	void setBank(unsigned short addr, unsigned char byte)
	{
		if (addr >= 0x6000 && addr < 0x7000)
		{
			page4[1] = byte << 13;
		}
		else if (addr >= 0x8000 && addr < 0x9000)
		{
			page4[2] = byte << 13;
		}
		else if (addr >= 0xa000 && addr < 0xb000)
		{
			page4[3] = byte << 13;
		}
		else if (addr >= 0xb000 && addr < 0xbfff)
		{
			RAM[addr - 0xb000] = byte;
		}
	}
};

static Mapper *mappers[4];

extern "C" {
extern void start_l1cache();
}

MapperAscii16 m(ROM);
void MSXHandler(void *);
void RPMV_Init(unsigned char *rom, int size)
{
	/** GPIO Register set */
	Mapper *m0;
	switch (guessRomType(rom, size))
	{
		case ROM_ASCII16:
			m0 = new MapperAscii16(rom);
			break;
		case ROM_KONAMI:
			m0 = new MapperKonami4(rom);
			break;
		case ROM_NORMAL:
			m0 = new NoMapper(rom);
		default:
			m0 = &m;
			break;
	}
	mappers[0] = m0;
    gpio[GPIO_GPFSEL0] = 0x49249249;
	gpio[GPIO_GPFSEL1] = 0x49249249;
	gpio[GPIO_GPFSEL2] = 0x49249249;
//	pads[BCM2835_PADS_GPIO_0_27/4] = BCM2835_PAD_PASSWRD | BCM2835_PAD_DRIVE_4mA;
	GPIO_CLR(0xffffffff);
	GPIO_CLR(ADDR | 0xffff);
	GPIO_SET(INT | WAIT | DAT_DIR | DAT_EN | 0xffff);	
	start_l1cache();
}

void RPMV_Wait(int b)
{
	if (!b)
		GPIO_SET(WAIT);
	else
		GPIO_CLR(WAIT);
}

void MSXHandler(void *pParam)
{
	mappers[0]->mapping();
}		

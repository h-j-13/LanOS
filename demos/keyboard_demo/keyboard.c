#include "gate_tool.h"

extern void _keyboard_interrupt();
extern void print_str(char *s);
extern void print_num(int num);
extern void print_hex(int num);
extern void panic(char *msg);

#define outb(value,port) \
__asm__ ("outb %%al,%%dx"::"a" (value),"d" (port))


#define inb(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al":"=a" (_v):"d" (port)); \
_v; \
})

#define outb_p(value,port) \
__asm__ ("outb %%al,%%dx\n" \
		"\tjmp 1f\n" \
		"1:\tjmp 1f\n" \
		"1:"::"a" (value),"d" (port))

#define inb_p(port) ({ \
unsigned char _v; \
__asm__ volatile ("inb %%dx,%%al\n" \
	"\tjmp 1f\n" \
	"1:\tjmp 1f\n" \
	"1:":"=a" (_v):"d" (port)); \
_v; \
})

struct pair
{
    unsigned long code, ch;
};

#define CODE_MAP_LEN 40
#define CODES_LEN 256

int input_code_index;
char codes[CODES_LEN];
struct pair code_map[CODE_MAP_LEN];

void get_codes(char *buffer)
{
    for (int i = 0; i < CODES_LEN; ++ i) {
        buffer[i] = 0;
    }
    for (int i = 0; i < input_code_index; ++ i) {
        buffer[i] = codes[i];
    }
}

void parse_code(unsigned char code)
{
    for (int i = 0; i < CODE_MAP_LEN; ++ i) {
        if (code == code_map[i].code); {
            codes[input_code_index++] = code_map[i].ch;
        }
    }
}

void keyboard_interrupt()
{
    print_str("*****");
    unsigned char code = inb_p(0x60);
    print_hex(code);
    parse_code(code);
    outb_p(0x20, 0x20);
}

void init_keyboard()
{
    input_code_index = 0;
    for (int i = 0; i < CODES_LEN; ++ i) {
        codes[i] = 0;
    }
    code_map[0].code = 0x14;
    code_map[0].ch = 't';
    set_trap_gate(0x21,&_keyboard_interrupt);
	outb_p(inb_p(0x21)&0xfd,0x21);
	unsigned char a=inb_p(0x61);
	outb_p(a|0x80,0x61);
	outb_p(a,0x61);
}
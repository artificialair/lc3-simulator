#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#define MEMLEN 65536

typedef unsigned short int Address;
typedef unsigned short int Value;

typedef struct {
    Address pc;
    Address ir;
    Value reg[8];
    Value mem[MEMLEN];
    int n;
    int z;
    int p;
    Address origin;
    char running;
    int count;
} CPU;

typedef struct {
    int end_reg;
    int steps_reg;
    int show_count;
} Flags;

CPU cpu;  // global
Flags flags;

void set_cc(int dr) {
    cpu.n = (signed short int)cpu.reg[dr] < 0;
    cpu.z = (signed short int)cpu.reg[dr] == 0;
    cpu.p = (signed short int)cpu.reg[dr] > 0;
}

Value sign_extend(Value v, int b) {
    if ((v >> (b - 1)) & 1) {
        return v | ((Value)0xFFFF << b);
    } else {
        return v;
    }
    
}

Address calculate_offset(Value offset) {
    return cpu.pc + offset;
}

void add() {
    int dr = (cpu.ir >> 9) & 7;
    Value v1 = cpu.reg[(cpu.ir >> 6) & 7];
    Value v2;
    if (cpu.ir & 0x20) {
        v2 = sign_extend(cpu.ir & 31, 5);
    } else {
        v2 = cpu.reg[cpu.ir & 7];
    }
    cpu.reg[dr] = v1 + v2;
    set_cc(dr);
}

void and() {
    int dr = (cpu.ir >> 9) & 7;
    Value v1 = cpu.reg[(cpu.ir >> 6) & 7];
    Value v2;
    if (cpu.ir & 0x20) {
        v2 = sign_extend(cpu.ir & 31, 5);
    } else {
        v2 = cpu.reg[cpu.ir & 7];
    }
    cpu.reg[dr] = v1 & v2;
    set_cc(dr);
}

void br() {
    if ((cpu.ir >> 9) & ((cpu.n << 2) + (cpu.z << 1) + cpu.p)) {
        Value offset = sign_extend(cpu.ir & 0x01FF, 9);
        cpu.pc = calculate_offset(offset);
    }
}

void jmp() {
    int dr = (cpu.ir >> 6) & 7;
    cpu.pc = cpu.reg[dr];
}

void jsr() {
    cpu.reg[7] = cpu.pc;
    if (cpu.ir & 0x0800) {
        Value offset = sign_extend(cpu.ir & 0x07FF, 11);
        cpu.pc = calculate_offset(offset);
    } else {
        int dr = (cpu.ir >> 6) & 7;
        cpu.pc = cpu.reg[dr];
    }
}

void ld() {
    int dr = (cpu.ir >> 9) & 7;
    Value offset = sign_extend(cpu.ir & 0x01FF, 9);
    cpu.reg[dr] = cpu.mem[calculate_offset(offset)];
    set_cc(dr);
}

void ldi() {
    int dr = (cpu.ir >> 9) & 7;
    Value offset = sign_extend(cpu.ir & 0x01FF, 9);
    cpu.reg[dr] = cpu.mem[cpu.mem[calculate_offset(offset)]];
    set_cc(dr);
}

void ldr() {
    int dr = (cpu.ir >> 9) & 7;
    int sr = (cpu.ir >> 6) & 7;
    Value offset = sign_extend(cpu.ir & 0x003F, 6);
    cpu.reg[dr] = cpu.mem[cpu.reg[sr] + offset];
    set_cc(dr);
}

void lea() {
    int dr = (cpu.ir >> 9) & 7;
    Value offset = sign_extend(cpu.ir & 0x01FF, 9);
    cpu.reg[dr] = calculate_offset(offset);
}

void not() {
    int dr = (cpu.ir >> 9) & 7;
    Value v1 = cpu.reg[(cpu.ir >> 6) & 7];
    cpu.reg[dr] = ~v1;
}

void st() {
    int dr = (cpu.ir >> 9) & 7;
    Value offset = sign_extend(cpu.ir & 0x01FF, 9);
    cpu.mem[calculate_offset(offset)] = cpu.reg[dr];
}

void sti() {
    int dr = (cpu.ir >> 9) & 7;
    Value offset = sign_extend(cpu.ir & 0x01FF, 9);
    cpu.mem[cpu.mem[calculate_offset(offset)]] = cpu.reg[dr];
}

void str() {
    int dr = (cpu.ir >> 9) & 7;
    int sr = (cpu.ir >> 6) & 7;
    Value offset = sign_extend(cpu.ir & 0x003F, 6);
    cpu.mem[cpu.reg[sr] + offset] = cpu.reg[dr];
}

void trap() {
    switch (cpu.ir & 0x0FFF) {
        case 0x020: {
            cpu.reg[0] = _getch();
            break;
        }
        case 0x021: { 
            printf("%c", cpu.reg[0]); 
            break; 
        }
        case 0x022: {
            Address p = cpu.reg[0];
            Value w = cpu.mem[p];
            while (w != 0) {
                printf("%c", w);
                p++;
                w = cpu.mem[p];
            }
            break;
        }
        case 0x025: { 
            printf("\n --- halting the LC3 ---\n\n");
            cpu.running = 0; 
            break; 
        }
    }
}

void init_cpu(char file[]) {
    FILE *fptr = fopen(file, "r");
    if (fptr == NULL) {
        printf("File can't be opened \n");
    }
    int buff;
    Address address;
    if (fscanf(fptr, "%x", &buff) == 1) {
        address = (Address)buff;
        cpu.origin = address;
    }
    while (fscanf(fptr, "%x", &buff) == 1) {
        cpu.mem[address] = (Value)buff;
        address = address + 1;
    }
    fclose(fptr);
}

void run_lc3() {
    cpu.pc = cpu.origin;
    cpu.running = 1;
    while (cpu.running) {
        if (flags.steps_reg) {
            printf("Registers: ");
            printf("PC:%04X IR:%04X | ", cpu.pc, cpu.ir);
            for (int i = 0; i < 8; i++) {
                printf("R%d:%04X ", i, cpu.reg[i]);
            }
            printf("\n");
        }
        cpu.ir = cpu.mem[cpu.pc];
        cpu.pc = cpu.pc + 1;
        cpu.count++;
        if (cpu.ir == 0x0000) {  // the actual LC3 does not do this. it will just keep going
            printf("\n --- Detected useless break statement in IR (x0000) --- ");
            cpu.ir = 0xF025;
        }
        switch (cpu.ir >> 12) {
            case 0: { 
                br(); 
                break; 
            }
            case 1: { 
                add(); 
                break; 
            }
            case 2: { 
                ld(); 
                break; 
            }
            case 3: { 
                st(); 
                break; 
            }
            case 4: { 
                jsr(); 
                break;
            }
            case 5: { 
                and(); 
                break; 
            }
            case 6: { 
                ldr(); 
                break; 
            }
            case 7: { 
                str(); 
                break; 
            }
            // case 8: { rti(); }
            case 9: { 
                not(); 
                break; 
            }
            case 10: { 
                ldi(); 
                break; 
            }
            case 11: { 
                sti(); 
                break; 
            }
            case 12: { 
                jmp(); 
                break; 
            }
            // case 13: {}
            case 14: { 
                lea(); 
                break; 
            }
            case 15: { 
                trap(); 
                break; 
            }
        }
    }
}

int main(int argc, char **argv) {
    if (argc <= 1) {
        printf("You must include a file.");
        exit(1);
    }
    if (argc > 2) {
        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "-r") == 0) {
                flags.end_reg = 1;
            }
            if (strcmp(argv[i], "-R") == 0) {
                flags.end_reg = 1;
                flags.steps_reg = 1;
            }
            if (strcmp(argv[i], "-c") == 0) {
                flags.show_count = 1;
            }
        }
    }
    init_cpu(argv[1]);
    run_lc3();
    if (flags.end_reg) {
        printf("Registers: ");
        printf("PC:%04X IR:%04X | ", cpu.pc, cpu.ir);
        for (int i = 0; i < 8; i++) {
            printf("R%d:%04X ", i, cpu.reg[i]);
        }
        printf("\n");
    }
    if (flags.show_count) {
        printf("Total instructions executed: %u", cpu.count);
    }
}

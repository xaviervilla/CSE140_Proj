#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "computer.h"
#undef mips			/* gcc already has a def for mips */

unsigned int endianSwap(unsigned int);

void PrintInfo (int changedReg, int changedMem);
unsigned int Fetch (int);
void Decode (unsigned int, DecodedInstr*, RegVals*);
int Execute (DecodedInstr*, RegVals*);
int Mem(DecodedInstr*, int, int *);
void RegWrite(DecodedInstr*, int, int *);
void UpdatePC(DecodedInstr*, int);
void PrintInstruction (DecodedInstr*);

/*Globally accessible Computer variable*/
Computer mips;
RegVals rVals;

int debug_decode = 1;

/*
 *  Return an initialized computer with the stack pointer set to the
 *  address of the end of data memory, the remaining registers initialized
 *  to zero, and the instructions read from the given file.
 *  The other arguments govern how the program interacts with the user.
 */
void InitComputer (FILE* filein, int printingRegisters, int printingMemory,
  int debugging, int interactive) {
    int k;
    unsigned int instr;

    /* Initialize registers and memory */

    for (k=0; k<32; k++) {
        mips.registers[k] = 0;
    }
    
    /* stack pointer - Initialize to highest address of data segment */
    mips.registers[29] = 0x00400000 + (MAXNUMINSTRS+MAXNUMDATA)*4;

    for (k=0; k<MAXNUMINSTRS+MAXNUMDATA; k++) {
        mips.memory[k] = 0;
    }

    k = 0;
    while (fread(&instr, 4, 1, filein)) {
	/*swap to big endian, convert to host byte order. Ignore this.*/
        mips.memory[k] = ntohl(endianSwap(instr));
        k++;
        if (k>MAXNUMINSTRS) {
            fprintf (stderr, "Program too big.\n");
            exit (1);
        }
    }

    mips.printingRegisters = printingRegisters;
    mips.printingMemory = printingMemory;
    mips.interactive = interactive;
    mips.debugging = debugging;
}

unsigned int endianSwap(unsigned int i) {
    return (i>>24)|(i>>8&0x0000ff00)|(i<<8&0x00ff0000)|(i<<24);
}

/*
 *  Run the simulation.
 */
void Simulate () {
    char s[40];  /* used for handling interactive input */
    unsigned int instr;
    int changedReg=-1, changedMem=-1, val;
    DecodedInstr d;
    
    /* Initialize the PC to the start of the code section */
    mips.pc = 0x00400000;
    if (debug_decode){
        for (int i = 0; i < 10; i++){
            if (mips.interactive) {
                printf ("> ");
                fgets (s,sizeof(s),stdin);
                if (s[0] == 'q') {
                    return;
                }
            }

            /* Fetch instr at mips.pc, returning it in instr */
            instr = Fetch (mips.pc);

            printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

            /* 
        * Decode instr, putting decoded instr in d
        * Note that we reuse the d struct for each instruction.
        */
            Decode (instr, &d, &rVals);

            /*Print decoded instruction*/
            PrintInstruction(&d);

            /* 
        * Perform computation needed to execute d, returning computed value 
        * in val 
        */
            val = Execute(&d, &rVals);

        UpdatePC(&d,val);

            /* 
        * Perform memory load or store. Place the
        * address of any updated memory in *changedMem, 
        * otherwise put -1 in *changedMem. 
        * Return any memory value that is read, otherwise return -1.
            */
            val = Mem(&d, val, &changedMem);

            /* 
        * Write back to register. If the instruction modified a register--
        * (including jal, which modifies $ra) --
            * put the index of the modified register in *changedReg,
            * otherwise put -1 in *changedReg.
            */
            RegWrite(&d, val, &changedReg);

            PrintInfo (changedReg, changedMem);
        }
    }
    else{
        while (1) {
            // for (int i = 0; i < 10; i++){
                if (mips.interactive) {
                    printf ("> ");
                    fgets (s,sizeof(s),stdin);
                    if (s[0] == 'q') {
                        return;
                    }
                }

                /* Fetch instr at mips.pc, returning it in instr */
                instr = Fetch (mips.pc);

                printf ("Executing instruction at %8.8x: %8.8x\n", mips.pc, instr);

                /* 
            * Decode instr, putting decoded instr in d
            * Note that we reuse the d struct for each instruction.
            */
                Decode (instr, &d, &rVals);

                /*Print decoded instruction*/
                PrintInstruction(&d);

                /* 
            * Perform computation needed to execute d, returning computed value 
            * in val 
            */
                val = Execute(&d, &rVals);

            UpdatePC(&d,val);

                /* 
            * Perform memory load or store. Place the
            * address of any updated memory in *changedMem, 
            * otherwise put -1 in *changedMem. 
            * Return any memory value that is read, otherwise return -1.
                */
                val = Mem(&d, val, &changedMem);

                /* 
            * Write back to register. If the instruction modified a register--
            * (including jal, which modifies $ra) --
                * put the index of the modified register in *changedReg,
                * otherwise put -1 in *changedReg.
                */
                RegWrite(&d, val, &changedReg);

                PrintInfo (changedReg, changedMem);
            }
    }
    
}

/*
 *  Print relevant information about the state of the computer.
 *  changedReg is the index of the register changed by the instruction
 *  being simulated, otherwise -1.
 *  changedMem is the address of the memory location changed by the
 *  simulated instruction, otherwise -1.
 *  Previously initialized flags indicate whether to print all the
 *  registers or just the one that changed, and whether to print
 *  all the nonzero memory or just the memory location that changed.
 */
void PrintInfo ( int changedReg, int changedMem) {
    int k, addr;
    printf ("New pc = %8.8x\n", mips.pc);
    if (!mips.printingRegisters && changedReg == -1) {
        printf ("No register was updated.\n");
    } else if (!mips.printingRegisters) {
        printf ("Updated r%2.2d to %8.8x\n",
        changedReg, mips.registers[changedReg]);
    } else {
        for (k=0; k<32; k++) {
            printf ("r%2.2d: %8.8x  ", k, mips.registers[k]);
            if ((k+1)%4 == 0) {
                printf ("\n");
            }
        }
    }
    if (!mips.printingMemory && changedMem == -1) {
        printf ("No memory location was updated.\n");
    } else if (!mips.printingMemory) {
        printf ("Updated memory at address %8.8x to %8.8x\n",
        changedMem, Fetch (changedMem));
    } else {
        printf ("Nonzero memory\n");
        printf ("ADDR	  CONTENTS\n");
        for (addr = 0x00400000+4*MAXNUMINSTRS;
             addr < 0x00400000+4*(MAXNUMINSTRS+MAXNUMDATA);
             addr = addr+4) {
            if (Fetch (addr) != 0) {
                printf ("%8.8x  %8.8x\n", addr, Fetch (addr));
            }
        }
    }
}

/*
 *  Return the contents of memory at the given address. Simulates
 *  instruction fetch. 
 */
unsigned int Fetch ( int addr) {
    return mips.memory[(addr-0x00400000)/4];
}

/* Decode instr, returning decoded instruction. */
void Decode ( unsigned int instr, DecodedInstr* d, RegVals* rVals) {

    // Compute mask of opcode and assign to d
    if(instr == 0x0){ exit(0); }
    d->op = (0xfc000000 & instr) >> 26;

    if(debug_decode){printf("Opcode: %x\n", d->op);}
    
    // Determine type based on opcpde
    switch (d->op){
        case 0x0:
            d->type = 0;
            break;
        case 0x2:
            d->type = 2;
            break;
        case 0x3:
            d->type = 2;
            break;
        case 0x9:
            d->type = 1;
        case 0xc:
            d->type = 1;
            break;
        case 0xd:
            d->type = 1;
            break;
        case 0xf:
            d->type = 1;
            break;
        case 0x4:
            d->type = 1;
            break;
        case 0x5:
            d->type = 1;
            break;
        case 0x23:
            d->type = 1;
            break;
        case 0x2b:
            d->type = 1;
            break;
        default:
            if(debug_decode){ printf("exiting!\n"); }
            else{ exit(0); }
    }

    // decode based on type
    switch(d->type){
        case 0: //r
            d->regs.r.funct = (0x0000003f & instr);
            d->regs.r.rs = (0x03e00000 & instr) >> 21;
            d->regs.r.rt = (0x001f0000 & instr) >> 16;
            d->regs.r.rd = (0x0000f800 & instr) >> 11;
            d->regs.r.shamt = (0x000007c0 & instr) >> 6;
            break;
        case 1: //i
            d->regs.i.rs = (0x03e00000 & instr) >> 21;
            d->regs.i.rt = (0x001f0000 & instr) >> 16;
            d->regs.i.addr_or_immed = (0x0000ffff & instr);
            // Do sign extension if immed is neg
            if( d->regs.i.addr_or_immed & 0x00008000 ){
                d->regs.i.addr_or_immed = d->regs.i.addr_or_immed + 0xffff0000;
            }
            if(debug_decode) {printf("d->regs.i.addr_or_immed: %i\n", d->regs.i.addr_or_immed); }
            break;
        case 2: //j
    	    d->regs.j.target = (0x03ffffff & instr) << 2;
            break;
        default:
            if(debug_decode){ printf("Exiting!\n"); }
            else{ exit(0); }
    }


}

/*
 *  Print the disassembled version of the given instruction
 *  followed by a newline.
 */
void PrintInstruction ( DecodedInstr* d) {
    switch (d->type){
        case 0:
            // Print for R types
	    switch (d->regs.r.funct){
            case 0x21: //addu
                printf("addu\t$%i, $%i, $%i\n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
                break;
            case 0x23: //subu
                printf("subu\t$%i, $%i, $%i\n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
                break;
            case 0x00: //sll
                printf("sll\t$%i, $%i, %i\n", d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
                break;
            case 0x02: //srl
                printf("srl\t$%i, $%i, %i\n", d->regs.r.rd, d->regs.r.rt, d->regs.r.shamt);
                break;
            case 0x24: //and
                printf("and\t$%i, $%i, $%i\n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
                break;
            case 0x25: //or
                printf("or\t$%i, $%i, $%i\n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
                break;
            case 0x2a: //slt
                printf("slt\t$%i, $%i, $%i\n", d->regs.r.rd, d->regs.r.rs, d->regs.r.rt);
                break;
            case 0x08: //jr
                printf("jr\t$%i\n", d->regs.r.rs);
                break;
                default:
                    if(debug_decode){ printf("exiting!\n"); }
                    else{ exit(0); }
	        }
            break;
        case 1:
            // Print for I types
            switch (d->op){
            case 0x9:
                printf("addiu\t$%i, $%i, %i\n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
                break;
            case 0xc:
                printf("andi\t$%i, $%i, 0x%04x\n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
                break;
            case 0xd:
                printf("ori\t$%i, $%i, 0x%04x\n", d->regs.i.rt, d->regs.i.rs, d->regs.i.addr_or_immed);
                break;
            case 0xf:
                printf("lui\t$%i, 0x%04x\n", d->regs.i.rt, d->regs.i.addr_or_immed);
                break;
            case 0x4:
                printf("beq\t$%i, $%i, 0x%08x\n", d->regs.i.rs, d->regs.i.rt, 4*(d->regs.i.addr_or_immed+1) + mips.pc);
                break;
            case 0x5:
                printf("bne\t$%i, $%i, 0x%08x\n", d->regs.i.rs, d->regs.i.rt, 4*(d->regs.i.addr_or_immed+1) + mips.pc);
                break;
            case 0x23:
                printf("lw\t$%i, %i($%i)\n", d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
                break;
            case 0x2b:
                printf("sw\t$%i, %i($%i)\n", d->regs.i.rt, d->regs.i.addr_or_immed, d->regs.i.rs);
                break;
        }
            break;
        case 2:
            // Print for J types
            switch (d->op){
                case 0x2: //j
                printf("j\t0x%08x\n", d->regs.r.rs);
                break;
                case 0x3: //jal
                printf("jal\t0x%08x\n", d->regs.r.rs);
                break;
            }
            break;
        default:
            if(debug_decode){ printf("exiting!\n"); }
            else{ exit(0); }
    }
}

/* Perform computation needed to execute d, returning computed value */
int Execute ( DecodedInstr* d, RegVals* rVals) {
    /* Your code goes here */
  return 0;
}

/* 
 * Update the program counter based on the current instruction. For
 * instructions other than branches and jumps, for example, the PC
 * increments by 4 (which we have provided).
 */
void UpdatePC ( DecodedInstr* d, int val) {
    mips.pc+=4;
    /* Your code goes here */
}

/*
 * Perform memory load or store. Place the address of any updated memory 
 * in *changedMem, otherwise put -1 in *changedMem. Return any memory value 
 * that is read, otherwise return -1. 
 *
 * Remember that we're mapping MIPS addresses to indices in the mips.memory 
 * array. mips.memory[0] corresponds with address 0x00400000, mips.memory[1] 
 * with address 0x00400004, and so forth.
 *
 */
int Mem( DecodedInstr* d, int val, int *changedMem) {
    /* Your code goes here */
  return 0;
}

/* 
 * Write back to register. If the instruction modified a register--
 * (including jal, which modifies $ra) --
 * put the index of the modified register in *changedReg,
 * otherwise put -1 in *changedReg.
 */
void RegWrite( DecodedInstr* d, int val, int *changedReg) {
    /* Your code goes here */
}

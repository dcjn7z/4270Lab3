#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
	
	uint32_t instruction, opcode, function, rt, rd, output, lmd;
	
	instruction = MEM_WB.IR;
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	output = MEM_WB.ALUOutput;
	lmd = MEM_WB.LMD;
	
	if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x03: //SRA 
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x0C: //SYSCALL
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x11: //MTHI
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x13: //MTLO
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x18: //MULT
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x19: //MULTU
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x1A: //DIV 
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x1B: //DIVU
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x2A: //SLT
				NEXT_STATE.REGS[rd] = output;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x0A: //SLTI
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x20: //LB
				NEXT_STATE.REGS[rt] = lmd;
				break;
			case 0x21: //LH
				NEXT_STATE.REGS[rt] = lmd;
				break;
			case 0x23: //LW
				NEXT_STATE.REGS[rt] = lmd;
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	uint32_t instruction, opcode, b, alu, output;
	instruction = EX_MEM.IR;
	opcode = (instruction & 0xFC000000) >> 26;
	b = EX_MEM.B;
	alu = EX_MEM.ALUOutput;

if(opcode == 0x00){
	}
	else{
		switch(opcode){
			case 0x20: //LB
				output = mem_read_32(alu)
				break;
			case 0x21: //LH
				output = mem_read_32(alu)
				break;
			case 0x23: //LW
				output = mem_read_32(alu)
				break;
			case 0x28: //SB
				mem_write_32(alu) = b;				
				break;
			case 0x29: //SH
				mem_write_32(alu) = b;				
				break;
			case 0x2B: //SW
				mem_write_32(alu) = b;				
				break;
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}

	MEM_WB.IR = instruction;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.LMD = output;
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
<<<<<<< HEAD
	uint32_t instruction, immediate, opcode, function, output, sa;
	instruction = IF_EX.IR;
	a = IF_EX.A;
	b = IF_EX.B;
	immediate = IF_EX.imm;
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	sa = (instruction & 0x000007C0) >> 6;
	uint64_t product, p1, p2;

if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				output = b << sa;
				break;
			case 0x02: //SRL
				output = b >> sa;
				break;
			case 0x03: //SRA 
				if ((b & 0x80000000) == 1)
				{
					output =  ~(~b >> sa );
				}
				else{
					output = b >> sa;
				}
				break;
			case 0x0C: //SYSCALL
				if(CURRENT_STATE.REGS[2] == 0xa){
					RUN_FLAG = FALSE;
				}
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = CURRENT_STATE.REGS[rs];
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = CURRENT_STATE.LO;
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = CURRENT_STATE.REGS[rs];
				break;
			case 0x18: //MULT
				if ((a & 0x80000000) == 0x80000000){
					p1 = 0xFFFFFFFF00000000 | a;
				}else{
					p1 = 0x00000000FFFFFFFF & a;
				}
				if ((b & 0x80000000) == 0x80000000){
					p2 = 0xFFFFFFFF00000000 | b;
				}else{
					p2 = 0x00000000FFFFFFFF & b;
				}
				product = p1 * p2;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x19: //MULTU
				product = (uint64_t)a * (uint64_t)b;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x1A: //DIV 
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = (int32_t)a / (int32_t)b;
					NEXT_STATE.HI = (int32_t)a % (int32_t)b;
				}
				break;
			case 0x1B: //DIVU
				if(CURRENT_STATE.REGS[rt] != 0)
				{
					NEXT_STATE.LO = CURRENT_STATE.REGS[rs] / CURRENT_STATE.REGS[rt];
					NEXT_STATE.HI = CURRENT_STATE.REGS[rs] % CURRENT_STATE.REGS[rt];
				}
				break;
			case 0x20: //ADD
				output = a + b;
				break;
			case 0x21: //ADDU 
				output = a + b;
				break;
			case 0x22: //SUB
				output = a - b;
				break;
			case 0x23: //SUBU
				output = a - b;
				break;
			case 0x2A: //SLT
				if(a < b){
					output = 0x1;
				}
				else{
					output = 0x0;
				}
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{
		switch(opcode){
			case 0x08: //ADDI
				output = a + immediate;
				break;
			case 0x09: //ADDIU
				output = a + immediate;
				break;
			case 0x0A: //SLTI
				if ( (  (int32_t)a - (int32_t)( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF))) < 0){
					output = 0x1;
				}else{
					output = 0x0;
				}
				break;
			case 0x0F: //LUI
				output = immediate << 16;
				break;
			case 0x20: //LB
				output = a + imm;
				break;
			case 0x21: //LH
				output = a + imm;
				break;
			case 0x23: //LW
				output = a + imm;
				break;
			case 0x28: //SB
				output = a + imm;				
				break;
			case 0x29: //SH
				output = a + imm;
				break;
			case 0x2B: //SW
				output = a + imm;
				break;
			default:
				// put more things here
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}

	EX_MEM.IR = instruction;
	EX_MEM.B = b;
	EX_MEM.ALUOutput = output;
=======
	/*IMPLEMENT THIS*/
	//CHAN KIM
	
	//Memroy Reference (load/store)
	//ALUOutput <= A + imm
	IF_EX.ALUOutput = ID_IF.A + ID_IF.imm;
	
	//Register-register Operation
	//ALUOutput <= A op B
	IF_EX.ALUOutput = ID_IF.A + ID_IF.B;
	if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				IF_EX.ALUOutput = ID_IF.B << sa;
				break;
			case 0x02: //SRL
				IF_EX.ALUOutput = ID_IF.B << sa;
				break;
			case 0x03: //SRA
				if ((ID_IF.B & 0x80000000) == 1)
				{
					IF_EX.ALUOutput = ~(~ID_IF.B >> sa);
				}
				else
				{
					IF_EX.ALUOutput = ID_IF.B >> sa;
				}
			case 0x0C: //SYSCALL
				//if(CURRENT_STATE.REGS[2] == 0xa)
				if(ID_IF.REGS[2] == 0xa)
				{
					RUN_FLAG = FALSE;	
				}
			case 0x10: //MFHI
				//NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
				IF_EX.C = ID_IF.HI;
				break;
			case 0x11: //MTHI
				IF_EX.HI = ID_IF.A;
				break;
			case 0x12: //MFLO
				IF_EX.C = ID_IF.LO;
				break;
			case 0x13: //MTLO
				IF_EX.LO = ID_IF.A;
				break;
			case 0x18: //MULT
				if ((ID_IF.A & 0x80000000) == 0x80000000)
				{
					p1 = 0xFFFFFFFF00000000 | ID_IF.A;	
				}
				else
				{
					p1 = 0x00000000FFFFFFFF & ID_IF.A;	
				}
				if ((ID_IF.B & 0x80000000) == 0x80000000)
				{
					p2 = 0xFFFFFFFF00000000 | ID_IF.B;
				}
				else
				{
					p2 = 0x00000000FFFFFFFF & ID_IF.B;
				}
				product = p1 * p2;
				IF_EX.LO = (product & 0X00000000FFFFFFFF);
				IF_EX.HI = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x19: //MULTU
				product = (uint64_t)ID_IF.A * (uint64_t)ID_IF.B
				IF_EX.LO = (product & 0X00000000FFFFFFFF);
				IF_EX.HI = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x1A: //DIV
				if(ID_IF.B != 0)
				{
					IF_EX.LO = (int32_t)ID_IF.A / (int32_t)ID_IF.B;
					IF_EX.HI = (int32_t)ID_IF.A % (int32_t)ID_IF.B;
				}
				break;
			case 0x1B: //DIVU
				if(ID_IF.B !=0)
				{	
					IF_EX.LO = ID_IF.A / ID_IF.B;
					IF_EX.HI = ID_IF.A % ID_IF.B;
				}
				break;
			case 0x20: //ADD
				//NEXT_STATE.REGS[rd] = CURRENT_STATE.REGS[rs] + CURRENT_STATE.REGS[rt];
				IF_EX.C = ID_IF.A + ID_IF.B;
				break;
			case 0x21: //ADDU
				IF_EX.C = ID_IF.B + ID_IF.A;
				break;
			case 0x22: //SUB
				IF_EX.C = ID_IF.A - ID_IF.B;
				break;
			case 0x23: //SUBU
				IF_EX.C = ID_IF.A - ID_IF.B;
				break;
			case 0x24: //AND
				IF_EX.C = ID_IF.A & ID_IF.B;
				break;
			case 0x25: //OR
				IF_EX.C = ID_IF.A | ID_IF.B;
				break;
			case 0x27: //NOR
				IF_EX.C = ~(ID_IF.A | ID_IF.B);
				break;
			case 0x2A: //SLT
				if(ID_IF.A < ID_IF.B)
				{
					IF_EX.C = 0x0;	
				}
				else
				{
					IF_EX.C = 0x0;	
				}
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n" CURRENT_STATE.PC, 
				break;
		}	
	}
	//tch(opcode)
	else
	{
		switch(opcode){
			case 0x08: //ADDI
				IF_EX.B = ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				break;
			case 0x09: //ADDIU
				IF_EX.B = ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				break;
			case 0x0A: //SLTI
				if (( (int32_t)ID_IF.A - (int32_t)( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF))) < 0)
				{
					IF_EX.B = 0x1;	
				}
				else
				{
					IF_EX.B = 0x0;
				}
				break;
			case 0x0C: //ANDI
				IF_EX.B = ID_IF.A & (immediate & 0x0000FFFF);
				break;
			case 0x0D: //ORI
				IF_EX.B = ID_IF.A | (immediate & 0x0000FFFF);
				break;
			case 0x0E: //XORI
				IF_EX.B = ID_IF.A ^ (immediate & 0x0000FFFF);
				break;
			case 0x0F: //LUI
				IF_EX.B = immediate << 16;
				break;
			case 0x20 //LB
				data = mem_read_32(ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				IF_EX.B = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);
				break;
			case 0x21: //LH
				data = mem_read_32(ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				IF_EX.B = ((data & 0x0000FFFF) & 0x8000) > 0 ? (data | 0xFFFF0000) : (data & 0x0000FFFF);
				break;
			case 0x23: //LW
				IF_EX.B = mem_read_32(ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				break;
			case 0x28: //SB
				addr = ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32(addr);
				data = (data & 0xFFFFFF00) | (ID_IF.B & 0x000000FF);
				mem_write_32(addr,data);
				break;
			case 0x29: //SH
				addr = ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32(addr);
				data = (data & 0xFFFF0000) | (ID_IF.B & 0x0000FFFF);
				mem_write_32(addr,data);
				break;
			case 0x2B: //SW
				addr = ID_IF.A + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				mem_write_32(addr, ID_IF.B);
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
				
>>>>>>> a619aeea459ca6f1b98bf5666543e111d8d300e0
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
<<<<<<< HEAD
	
	uint32_t instruction, rs, rt, immediate;
	instruction = ID_IF.IR;
	
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	immediate = instruction & 0x0000FFFF;
	
	
	if ((immed & 0x00008000)>>15 == 0x1)
	{
		immed = immed + 0xFFFF0000;
	}
	
	IF_EX.A=CURRENT_STATE.REGS[rs];
	IF_EX.B=CURRENT_STATE.REGS[rt];
	IF_EX.IR = instruction;
	IF_EX.imm = immediate;

=======
	/*IMPLEMENT THIS*/
	uint32_t instruction;
	IF_EX = ID_IF
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	immediate = instruction & 0x0000FFFF;
	ID_IF.A = CURRENT_STATE.REGS[rs];
	ID_IF.B = CURRENT_STATE.REGS[rt];
	
	ID_IF.imm = 
>>>>>>> a619aeea459ca6f1b98bf5666543e111d8d300e0
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	ID_IF.IR = mem_read_32(CURRENT_STATE.PC);
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
	printf("CURRENT PC:\t%d\n",CURRENT_STATE.PC);
	printf("IF/ID.IR\t%d\t%s\n",ID_IF.IR,print_program(ID_IF.IR));
	printf("IF/ID.PC\t%d\t%s\n",ID_IF.PC);
	printf("\n");
	printf("ID/EX.IR\t%d\t%s\n",IF_EX.IR,print_program(ID_IF.IR));
	printf("ID/EX.A\t%d\n",IF_EX.A);
	printf("ID/EX.B\t%d\n",IF_EX.B);
	printf("ID/EX.imm\t%d\n",IF_EX.imm);
	printf("\n");
	printf("EX/MEM.IR\t%d\n",EX_MEM.IR);
	printf("EX/MEM.A\t%d\n",EX_MEM.A);
	printf("EX/MEM.B\t%d\n",EX_MEM.B);
	printf("EX/MEM.ALUOutput\t%d\n",EX_MEM.ALUOutput);
	printf("\n");
	printf("MEM/WB.IR\t%d\n",MEM_WB.IR);
	printf("MEM/WB.IR\t%d\n",MEM_WB.ALUOutput);
	printf("MEM/WB.LMD\t%d\n",MEM_WB.LMD);
	printf("\n");
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}

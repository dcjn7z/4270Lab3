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
	if (CURRENT_STATE.PC < 4194320){
		return;}
	
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
				if(CURRENT_STATE.REGS[2] == 0xa){
					RUN_FLAG = FALSE;
				}
				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x11: //MTHI
				break;
			case 0x12: //MFLO
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x13: //MTLO
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
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = output;
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = output;
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
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = output;
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = output;
				break;
		}
	}
	//show_pipeline();
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{
	if (CURRENT_STATE.PC < 4194316){
		return;}

	uint32_t instruction, opcode, b, alu, output;
	instruction = EX_MEM.IR;
	opcode = (instruction & 0xFC000000) >> 26;
	b = EX_MEM.B;
	alu = EX_MEM.ALUOutput;
	output=0;
	
	switch(opcode){
			case 0x20: //LB
				output = mem_read_32(alu);
				break;
			case 0x21: //LH
				output = mem_read_32(alu);
				break;
			case 0x23: //LW
				output = mem_read_32(alu);
				break;
			case 0x28: //SB
				mem_write_32(alu,b);				
				break;
			case 0x29: //SH
				mem_write_32(alu,b);				
				break;
			case 0x2B: //SW
				mem_write_32(alu,b);				
				break;
		}

	MEM_WB.IR = instruction;
	MEM_WB.ALUOutput = EX_MEM.ALUOutput;
	MEM_WB.LMD = output;
	//show_pipeline();
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	if (CURRENT_STATE.PC < 4194312){
		return;}

	uint32_t instruction, a, b, immediate, opcode, function, output, sa;
	instruction = IF_EX.IR;
	a = IF_EX.A;
	b = IF_EX.B;
	immediate = IF_EX.imm;
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	output = 0;
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
			case 0x10: //MFHI
				output = CURRENT_STATE.HI;
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = a;
				break;
			case 0x12: //MFLO
				output = CURRENT_STATE.LO;
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = a;
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
				EX_MEM.ALUOutput = (product & 0X00000000FFFFFFFF);
				EX_MEM.ALUOutput2 = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x19: //MULTU
				product = (uint64_t)a * (uint64_t)b;
				EX_MEM.ALUOutput = (product & 0X00000000FFFFFFFF);
				EX_MEM.ALUOutput2 = (product & 0XFFFFFFFF00000000)>>32;
				break;
			case 0x1A: //DIV 
				if(b != 0)
				{
					EX_MEM.ALUOutput = (int32_t)a / (int32_t)b;
					EX_MEM.ALUOutput2 = (int32_t)a % (int32_t)b;
				}
				break;
			case 0x1B: //DIVU
				if(b != 0)
				{
					EX_MEM.ALUOutput = a / b;
					EX_MEM.ALUOutput2 = a % b;
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
			case 0x24: //AND
				output = a & b;
				break;
			case 0x25: //OR
				output = a | b;
				break;
			case 0x26: //XOR
				output = a ^ b;
				break;
			case 0x27: //NOR
				output = ~(a | b);
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
				output = a + immediate;
				break;
			case 0x21: //LH
				output = a + immediate;
				break;
			case 0x23: //LW
				output = a + immediate;
				break;
			case 0x28: //SB
				output = a + immediate;				
				break;
			case 0x29: //SH
				output = a + immediate;
				break;
			case 0x2B: //SW
				output = a + immediate;
				break;
			case 0x0E: //XORI
				output = a ^ (immediate & 0x0000FFFF);
				break;
			case 0x0C: //ANDI
				output = a & (immediate & 0x0000FFFF);
				break;
			case 0x0D: //ORI
				output = a | (immediate & 0x0000FFFF);
				break;
		}
	}
	//passing through the pipelined, storing all values in the temporary registers
	EX_MEM.IR = instruction;
	EX_MEM.B = b;
	EX_MEM.ALUOutput = output;
	//show_pipeline();
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	uint32_t instruction, rs, rt, immediate;
	if (CURRENT_STATE.PC < 4194308){
		return;}
	instruction = ID_IF.IR;
	
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	immediate = instruction & 0x0000FFFF;
	
	
	if ((immediate & 0x00008000)>>15 == 0x1)
	{
		immediate = immediate + 0xFFFF0000;
	}
	
	IF_EX.A=CURRENT_STATE.REGS[rs];
	IF_EX.B=CURRENT_STATE.REGS[rt];
	IF_EX.IR = instruction;
	IF_EX.imm = immediate;
	//show_pipeline();
}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	if (ID_IF.IR ==	0xc){
		show_pipeline();
		return;}
	ID_IF.IR = mem_read_32(CURRENT_STATE.PC);
	NEXT_STATE.PC = CURRENT_STATE.PC + 4;
	ID_IF.PC = NEXT_STATE.PC;
	INSTRUCTION_COUNT++;
	show_pipeline();
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
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	
	instruction = mem_read_32(CURRENT_STATE.PC);
	
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	int i=0;
	
	while (function != 0x0C || opcode !=0x00){
		opcode = (instruction & 0xFC000000) >> 26;
		function = instruction & 0x0000003F;
		rs = (instruction & 0x03E00000) >> 21;
		rt = (instruction & 0x001F0000) >> 16;
		rd = (instruction & 0x0000F800) >> 11;
		sa = (instruction & 0x000007C0) >> 6;
		immediate = instruction & 0x0000FFFF;
		target = instruction & 0x03FFFFFF;
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				printf("SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				printf("SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				printf("SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				printf("JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					printf("JALR $r%u\n", rs);
				}
				else{
					printf("JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				printf("SYSCALL\n");
				break;
			case 0x10:
				printf("MFHI $r%u\n", rd);
				break;
			case 0x11:
				printf("MTHI $r%u\n", rs);
				break;
			case 0x12:
				printf("MFLO $r%u\n", rd);
				break;
			case 0x13:
				printf("MTLO $r%u\n", rs);
				break;
			case 0x18:
				printf("MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				printf("MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				printf("DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				printf("DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				printf("ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				printf("ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				printf("SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				printf("SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				printf("AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				printf("OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				printf("XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				printf("NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				printf("SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					printf("BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					printf("BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				printf("J 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				printf("JAL 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				printf("BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				printf("BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				printf("BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				printf("BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				printf("ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				printf("ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				printf("SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				printf("ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				printf("ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				printf("XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				printf("LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				printf("LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				printf("LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				printf("LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				printf("SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				printf("SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				printf("SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				printf("Instruction is not implemented!\n");
				break;
		}
	}
	i++;
	instruction = mem_read_32(CURRENT_STATE.PC + (4*i));
	
	}
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;
	
	char if_id[64];
	char id_ex[64];
	char *string1 = if_id;
	char *string2 = id_ex;
		
	instruction = ID_IF.IR;
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				snprintf(if_id,64,"SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				snprintf(if_id,64,"SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				snprintf(if_id,64,"SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				snprintf(if_id,64,"JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					snprintf(if_id,64,"JALR $r%u\n", rs);
				}
				else{
					snprintf(if_id,64,"JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				snprintf(if_id,64,"SYSCALL\n");
				break;
			case 0x10:
				snprintf(if_id,64,"MFHI $r%u\n", rd);
				break;
			case 0x11:
				snprintf(if_id,64,"MTHI $r%u\n", rs);
				break;
			case 0x12:
				snprintf(if_id,64,"MFLO $r%u\n", rd);
				break;
			case 0x13:
				snprintf(if_id,64,"MTLO $r%u\n", rs);
				break;
			case 0x18:
				snprintf(if_id,64,"MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				snprintf(if_id,64,"MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				snprintf(if_id,64,"DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				snprintf(if_id,64,"DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				snprintf(if_id,64,"ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				snprintf(if_id,64,"ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				snprintf(if_id,64,"SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				snprintf(if_id,64,"SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				snprintf(if_id,64,"AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				snprintf(if_id,64,"OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				snprintf(if_id,64,"XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				snprintf(if_id,64,"NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				snprintf(if_id,64,"SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				snprintf(if_id,64,"Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					snprintf(if_id,64,"BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					snprintf(if_id,64,"BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				snprintf(if_id,64,"J 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				snprintf(if_id,64,"JAL 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				snprintf(if_id,64,"BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				snprintf(if_id,64,"BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				snprintf(if_id,64,"BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				snprintf(if_id,64,"BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				snprintf(if_id,64,"ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				snprintf(if_id,64,"ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				snprintf(if_id,64,"SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				snprintf(if_id,64,"ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				snprintf(if_id,64,"ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				snprintf(if_id,64,"XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				snprintf(if_id,64,"LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				snprintf(if_id,64,"LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				snprintf(if_id,64,"LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				snprintf(if_id,64,"LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				snprintf(if_id,64,"SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				snprintf(if_id,64,"SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				snprintf(if_id,64,"SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				snprintf(if_id,64,"Instruction is not implemented!\n");
				break;
		}
	}
	
	instruction = IF_EX.IR;
	opcode = (instruction & 0xFC000000) >> 26;
	function = instruction & 0x0000003F;
	rs = (instruction & 0x03E00000) >> 21;
	rt = (instruction & 0x001F0000) >> 16;
	rd = (instruction & 0x0000F800) >> 11;
	sa = (instruction & 0x000007C0) >> 6;
	immediate = instruction & 0x0000FFFF;
	target = instruction & 0x03FFFFFF;
	
	if(opcode == 0x00){
		/*R format instructions here*/
		
		switch(function){
			case 0x00:
				snprintf(id_ex,64,"SLL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x02:
				snprintf(id_ex,64,"SRL $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x03:
				snprintf(id_ex,64,"SRA $r%u, $r%u, 0x%x\n", rd, rt, sa);
				break;
			case 0x08:
				snprintf(id_ex,64,"JR $r%u\n", rs);
				break;
			case 0x09:
				if(rd == 31){
					snprintf(id_ex,64,"JALR $r%u\n", rs);
				}
				else{
					snprintf(id_ex,64,"JALR $r%u, $r%u\n", rd, rs);
				}
				break;
			case 0x0C:
				snprintf(id_ex,64,"SYSCALL\n");
				break;
			case 0x10:
				snprintf(id_ex,64,"MFHI $r%u\n", rd);
				break;
			case 0x11:
				snprintf(id_ex,64,"MTHI $r%u\n", rs);
				break;
			case 0x12:
				snprintf(id_ex,64,"MFLO $r%u\n", rd);
				break;
			case 0x13:
				snprintf(id_ex,64,"MTLO $r%u\n", rs);
				break;
			case 0x18:
				snprintf(id_ex,64,"MULT $r%u, $r%u\n", rs, rt);
				break;
			case 0x19:
				snprintf(id_ex,64,"MULTU $r%u, $r%u\n", rs, rt);
				break;
			case 0x1A:
				snprintf(id_ex,64,"DIV $r%u, $r%u\n", rs, rt);
				break;
			case 0x1B:
				snprintf(id_ex,64,"DIVU $r%u, $r%u\n", rs, rt);
				break;
			case 0x20:
				snprintf(id_ex,64,"ADD $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x21:
				snprintf(id_ex,64,"ADDU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x22:
				snprintf(id_ex,64,"SUB $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x23:
				snprintf(id_ex,64,"SUBU $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x24:
				snprintf(id_ex,64,"AND $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x25:
				snprintf(id_ex,64,"OR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x26:
				snprintf(id_ex,64,"XOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x27:
				snprintf(id_ex,64,"NOR $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			case 0x2A:
				snprintf(id_ex,64,"SLT $r%u, $r%u, $r%u\n", rd, rs, rt);
				break;
			default:
				snprintf(id_ex,64,"Instruction is not implemented!\n");
				break;
		}
	}
	else{
		switch(opcode){
			case 0x01:
				if(rt == 0){
					snprintf(id_ex,64,"BLTZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				else if(rt == 1){
					snprintf(id_ex,64,"BGEZ $r%u, 0x%x\n", rs, immediate<<2);
				}
				break;
			case 0x02:
				snprintf(id_ex,64,"J 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x03:
				snprintf(id_ex,64,"JAL 0x%x\n", (CURRENT_STATE.PC & 0xF0000000) | (target<<2));
				break;
			case 0x04:
				snprintf(id_ex,64,"BEQ $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x05:
				snprintf(id_ex,64,"BNE $r%u, $r%u, 0x%x\n", rs, rt, immediate<<2);
				break;
			case 0x06:
				snprintf(id_ex,64,"BLEZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x07:
				snprintf(id_ex,64,"BGTZ $r%u, 0x%x\n", rs, immediate<<2);
				break;
			case 0x08:
				snprintf(id_ex,64,"ADDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x09:
				snprintf(id_ex,64,"ADDIU $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0A:
				snprintf(id_ex,64,"SLTI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0C:
				snprintf(id_ex,64,"ANDI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0D:
				snprintf(id_ex,64,"ORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0E:
				snprintf(id_ex,64,"XORI $r%u, $r%u, 0x%x\n", rt, rs, immediate);
				break;
			case 0x0F:
				snprintf(id_ex,64,"LUI $r%u, 0x%x\n", rt, immediate);
				break;
			case 0x20:
				snprintf(id_ex,64,"LB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x21:
				snprintf(id_ex,64,"LH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x23:
				snprintf(id_ex,64,"LW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x28:
				snprintf(id_ex,64,"SB $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x29:
				snprintf(id_ex,64,"SH $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			case 0x2B:
				snprintf(id_ex,64,"SW $r%u, 0x%x($r%u)\n", rt, immediate, rs);
				break;
			default:
				snprintf(id_ex,64,"Instruction is not implemented!\n");
				break;
		}
	}
	printf("************************************************************\n");
	printf("CURRENT PC:\t\t0x%x\n",CURRENT_STATE.PC);
	printf("IF/ID.IR\t\t0x%x\t%s",ID_IF.IR,string1);
	printf("IF/ID.PC\t\t0x%x\n",ID_IF.PC);
	printf("\n");
	printf("ID/EX.IR\t\t0x%x\t%s",IF_EX.IR,string2);
	printf("ID/EX.A\t\t\t0x%x\n",IF_EX.A);
	printf("ID/EX.B\t\t\t0x%x\n",IF_EX.B);
	printf("ID/EX.imm\t\t0x%x\n",IF_EX.imm);
	printf("\n");
	printf("EX/MEM.IR\t\t0x%x\n",EX_MEM.IR);
	printf("EX/MEM.A\t\t0x%x\n",EX_MEM.A);
	printf("EX/MEM.B\t\t0x%x\n",EX_MEM.B);
	printf("EX/MEM.ALUOutput\t0x%x\n",EX_MEM.ALUOutput);
	printf("\n");
	printf("MEM/WB.IR\t\t0x%x\n",MEM_WB.IR);
	printf("MEM/WB.ALUOutput\t0x%x\n",MEM_WB.ALUOutput);
	printf("MEM/WB.LMD\t\t0x%x\n",MEM_WB.LMD);
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

#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
#include <stdbool.h>
#include "structs.h"

#define STACK_BASE 0x0100

#define CIA_BASE 0xDC00
#define CIA_SIZE 0x0F

#define debug 0

extern bool paused;  

typedef struct {
    uint8_t cotrol_register;  
    uint8_t status_register;   
} CIA_State;

#define IRQ_VECTOR 0x314
#define BRK_VECTOR 0x0316
#define NMI_VECTOR 0x31A

extern CPUState cpu_state;
extern uint8_t memory[MEMORY_SIZE];
extern uint8_t temp;
extern uint16_t temp_address;
extern CIA_State cia_state;  

uint8_t read_memory(CPUState *state, uint16_t address);
static inline void write_memory(CPUState *state, uint16_t address, uint8_t value);

void reset_cpu(CPUState *state);
static inline void trigger_irq(CPUState *state, uint16_t irq_source);


static inline void BCD_ADD(CPUState *state, uint8_t value);
static inline void BCD_SUB(CPUState *state, uint8_t value);

static inline void BIT_ZEROPAGE(CPUState *state);
static inline void BIT_ABSOLUTE(CPUState *state);

static inline void ADC_IMMEDIATE(CPUState *state);   
static inline void ADC_ZEROPAGE(CPUState *state);   
static inline void ADC_ZEROPAGE_X(CPUState *state);  
static inline void ADC_ABSOLUTE(CPUState *state);    
static inline void ADC_ABSOLUTE_X(CPUState *state);  
static inline void ADC_ABSOLUTE_Y(CPUState *state);  
static inline void ADC_INDIRECT_X(CPUState *state);  
static inline void ADC_INDIRECT_Y(CPUState *state);  

static inline void AND_IMMEDIATE(CPUState *state);   
static inline void AND_ZEROPAGE(CPUState *state);   
static inline void AND_ZEROPAGE_X(CPUState *state);  
static inline void AND_ABSOLUTE(CPUState *state);    
static inline void AND_ABSOLUTE_X(CPUState *state);  
static inline void AND_ABSOLUTE_Y(CPUState *state);  
static inline void AND_INDIRECT_X(CPUState *state);  
static inline void AND_INDIRECT_Y(CPUState *state);  

static inline void ASL_IMMEDIATE(CPUState *state);   
static inline void ASL_ZEROPAGE(CPUState *state);   
static inline void ASL_ZEROPAGE_X(CPUState *state);  
static inline void ASL_ABSOLUTE(CPUState *state);    
static inline void ASL_ABSOLUTE_X(CPUState *state);  
static inline void ASL_ABSOLUTE_Y(CPUState *state);  
static inline void ASL_INDIRECT_X(CPUState *state);  
static inline void ASL_INDIRECT_Y(CPUState *state);  

static inline void BCC(CPUState *state);             
static inline void BCS(CPUState *state);             
static inline void BEQ(CPUState *state);             
static inline void BMI(CPUState *state);             
static inline void BNE(CPUState *state);             
static inline void BPL(CPUState *state);             
static inline void BRK(CPUState *state);             
static inline void BVC(CPUState *state);             
static inline void BVS(CPUState *state);             

static inline void CLC(CPUState *state);             
static inline void CLD(CPUState *state);             
static inline void CLI(CPUState *state);             
static inline void CLV(CPUState *state);             

static inline void CMP_IMMEDIATE(CPUState *state);   
static inline void CMP_ZEROPAGE(CPUState *state);   
static inline void CMP_ZEROPAGE_X(CPUState *state);  
static inline void CMP_ABSOLUTE(CPUState *state);    
static inline void CMP_ABSOLUTE_X(CPUState *state);  
static inline void CMP_ABSOLUTE_Y(CPUState *state);  
static inline void CMP_INDIRECT_X(CPUState *state);  
static inline void CMP_INDIRECT_Y(CPUState *state);  

static inline void CPX_IMMEDIATE(CPUState *state);   
static inline void CPX_ZEROPAGE(CPUState *state);   
static inline void CPX_ABSOLUTE(CPUState *state);    

static inline void CPY_IMMEDIATE(CPUState *state);   
static inline void CPY_ZEROPAGE(CPUState *state);   
static inline void CPY_ABSOLUTE(CPUState *state);    

static inline void DEC_ZEROPAGE(CPUState *state);   
static inline void DEC_ZEROPAGE_X(CPUState *state);  
static inline void DEC_ABSOLUTE(CPUState *state);    
static inline void DEC_ABSOLUTE_X(CPUState *state);  
static inline void DEC_ABSOLUTE_Y(CPUState *state);  

static inline void DEX(CPUState *state);             
static inline void DEY(CPUState *state);             

static inline void EOR_IMMEDIATE(CPUState *state);   
static inline void EOR_ZEROPAGE(CPUState *state);   
static inline void EOR_ZEROPAGE_X(CPUState *state);  
static inline void EOR_ABSOLUTE(CPUState *state);    
static inline void EOR_ABSOLUTE_X(CPUState *state);  
static inline void EOR_ABSOLUTE_Y(CPUState *state);  
static inline void EOR_INDIRECT_X(CPUState *state);  
static inline void EOR_INDIRECT_Y(CPUState *state);  

static inline void INC_ZEROPAGE(CPUState *state);   
static inline void INC_ZEROPAGE_X(CPUState *state);  
static inline void INC_ABSOLUTE(CPUState *state);    
static inline void INC_ABSOLUTE_X(CPUState *state);  
static inline void INC_ABSOLUTE_Y(CPUState *state);  

static inline void INX(CPUState *state);             
static inline void INY(CPUState *state);             

static inline void JMP_ABSOLUTE(CPUState *state);    
static inline void JMP_INDIRECT(CPUState *state);    

static inline void JSR(CPUState *state);             
static inline void SYSCALL(CPUState *state);         
static inline void HELLO_FROM_ARM(CPUState *state);

static inline void LDA_IMMEDIATE(CPUState *state);   
static inline void LDA_ZEROPAGE(CPUState *state);   
static inline void LDA_ZEROPAGE_X(CPUState *state);  
static inline void LDA_ABSOLUTE(CPUState *state);    
static inline void LDA_ABSOLUTE_X(CPUState *state);  
static inline void LDA_ABSOLUTE_Y(CPUState *state);  
static inline void LDA_INDIRECT_X(CPUState *state);  
static inline void LDA_INDIRECT_Y(CPUState *state);  

static inline void LDX_IMMEDIATE(CPUState *state);   
static inline void LDX_ZEROPAGE(CPUState *state);   
static inline void LDX_ZEROPAGE_Y(CPUState *state);  
static inline void LDX_ABSOLUTE(CPUState *state);    
static inline void LDX_ABSOLUTE_Y(CPUState *state);  

static inline void LDY_IMMEDIATE(CPUState *state);   
static inline void LDY_ZEROPAGE(CPUState *state);   
static inline void LDY_ZEROPAGE_X(CPUState *state);  
static inline void LDY_ABSOLUTE(CPUState *state);    
static inline void LDY_ABSOLUTE_X(CPUState *state);  
static inline void LDY_INDIRECT_X(CPUState *state);  
static inline void LDY_INDIRECT_Y(CPUState *state);  

static inline void NOP(CPUState *state);             

static inline void LSR_IMMEDIATE(CPUState *state);   
static inline void LSR_ZEROPAGE(CPUState *state);   
static inline void LSR_ZEROPAGE_X(CPUState *state);  
static inline void LSR_ABSOLUTE(CPUState *state);    
static inline void LSR_ABSOLUTE_X(CPUState *state);  
static inline void LSR_ABSOLUTE_Y(CPUState *state);  
static inline void LSR_INDIRECT_X(CPUState *state);  
static inline void LSR_INDIRECT_Y(CPUState *state);  

static inline void ORA_IMMEDIATE(CPUState *state);   
static inline void ORA_ZEROPAGE(CPUState *state);   
static inline void ORA_ZEROPAGE_X(CPUState *state);  
static inline void ORA_ABSOLUTE(CPUState *state);    
static inline void ORA_ABSOLUTE_X(CPUState *state);  
static inline void ORA_ABSOLUTE_Y(CPUState *state);  
static inline void ORA_INDIRECT_X(CPUState *state);  
static inline void ORA_INDIRECT_Y(CPUState *state);  

static inline void PHA(CPUState *state);             
static inline void PHP(CPUState *state);             
static inline void PLA(CPUState *state);             
static inline void PLP(CPUState *state);             

static inline void ROL_ACCUMULATOR(CPUState *state); 
static inline void ROL_ZEROPAGE(CPUState *state);   
static inline void ROL_ZEROPAGE_X(CPUState *state);  
static inline void ROL_ABSOLUTE(CPUState *state);    
static inline void ROL_ABSOLUTE_X(CPUState *state);  

static inline void ROR_ACCUMULATOR(CPUState *state); 
static inline void ROR_ZEROPAGE(CPUState *state);   
static inline void ROR_ZEROPAGE_X(CPUState *state);  
static inline void ROR_ABSOLUTE(CPUState *state);    
static inline void ROR_ABSOLUTE_X(CPUState *state);  

static inline void RTI(CPUState *state);             
static inline void RTS(CPUState *state);             

static inline void SBC_IMMEDIATE(CPUState *state);   
static inline void SBC_ZEROPAGE(CPUState *state);   
static inline void SBC_ZEROPAGE_X(CPUState *state);  
static inline void SBC_ABSOLUTE(CPUState *state);    
static inline void SBC_ABSOLUTE_X(CPUState *state);  
static inline void SBC_ABSOLUTE_Y(CPUState *state);  
static inline void SBC_INDIRECT_X(CPUState *state);  
static inline void SBC_INDIRECT_Y(CPUState *state);  

static inline void STA_ZEROPAGE(CPUState *state);   
static inline void STA_ZEROPAGE_X(CPUState *state);  
static inline void STA_ABSOLUTE(CPUState *state);    
static inline void STA_ABSOLUTE_X(CPUState *state);  
static inline void STA_ABSOLUTE_Y(CPUState *state);  
static inline void STA_INDIRECT_X(CPUState *state);  
static inline void STA_INDIRECT_Y(CPUState *state);  

static inline void STX_ZEROPAGE(CPUState *state);   
static inline void STX_ZEROPAGE_Y(CPUState *state);  
static inline void STX_ABSOLUTE(CPUState *state);    

static inline void STY_ZEROPAGE(CPUState *state);   
static inline void STY_ZEROPAGE_X(CPUState *state);  
static inline void STY_ABSOLUTE(CPUState *state);    

static inline void SEC(CPUState *state);             
static inline void SED(CPUState *state);             
static inline void SEI(CPUState *state);             

static inline void TAX(CPUState *state);             
static inline void TAY(CPUState *state);             

static inline void TSX(CPUState *state);             
static inline void TXA(CPUState *state);             

static inline void TXS(CPUState *state);             
static inline void TYA(CPUState *state);             

void execute_opcode(CPUState *state, uint8_t opcode);


extern void (*opcode_functions[256])(CPUState *state);
extern void (*arm_opcode_functions[256])(CPUState *state);

#endif 
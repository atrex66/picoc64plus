#ifndef OPCODES_H
#define OPCODES_H

#include <stdint.h>
#include <stdbool.h>
#include "structs.h"

#define STACK_BASE 0x0100

#define CIA_BASE 0xDC00
#define CIA_SIZE 0x0F

#define debug 0

extern bool paused;  // Declare the paused variable as extern

typedef struct {
    uint8_t cotrol_register;  // Control register for GPIO direction and pull-up configuration
    uint8_t status_register;   // Status register for GPIO input values
} CIA_State;

#define IRQ_VECTOR 0x314
#define BRK_VECTOR 0x0316
#define NMI_VECTOR 0x31A

extern CPUState cpu_state;
extern uint8_t memory[MEMORY_SIZE];
extern uint8_t temp;
extern uint16_t temp_address;
extern CIA_State cia_state;  // Declare the CIA state as extern

uint8_t read_memory(CPUState *state, uint16_t address);
void write_memory(CPUState *state, uint16_t address, uint8_t value);

void reset_cpu(CPUState *state);
void trigger_irq(CPUState *state, uint16_t irq_source);


void BCD_ADD(CPUState *state, uint8_t value);
void BCD_SUB(CPUState *state, uint8_t value);

void BIT_ZEROPAGE(CPUState *state);
void BIT_ABSOLUTE(CPUState *state);

void ADC_IMMEDIATE(CPUState *state);   // eg: ADC #$01
void ADC_ZEROPAGE(CPUState *state);   // eg: ADC $00
void ADC_ZEROPAGE_X(CPUState *state);  // eg: ADC $00,X
void ADC_ABSOLUTE(CPUState *state);    // eg: ADC $0000
void ADC_ABSOLUTE_X(CPUState *state);  // eg: ADC $0000,X
void ADC_ABSOLUTE_Y(CPUState *state);  // eg: ADC $0000,Y
void ADC_INDIRECT_X(CPUState *state);  // eg: ADC ($00,X)
void ADC_INDIRECT_Y(CPUState *state);  // eg: ADC ($00),Y

void AND_IMMEDIATE(CPUState *state);   // eg: AND #$01
void AND_ZEROPAGE(CPUState *state);   // eg: AND $00
void AND_ZEROPAGE_X(CPUState *state);  // eg: AND $00,X
void AND_ABSOLUTE(CPUState *state);    // eg: AND $0000
void AND_ABSOLUTE_X(CPUState *state);  // eg: AND $0000,X
void AND_ABSOLUTE_Y(CPUState *state);  // eg: AND $0000,Y
void AND_INDIRECT_X(CPUState *state);  // eg: AND ($00,X)
void AND_INDIRECT_Y(CPUState *state);  // eg: AND ($00),Y

void ASL_IMMEDIATE(CPUState *state);   // eg: ASL #$01
void ASL_ZEROPAGE(CPUState *state);   // eg: ASL $00
void ASL_ZEROPAGE_X(CPUState *state);  // eg: ASL $00,X
void ASL_ABSOLUTE(CPUState *state);    // eg: ASL $0000
void ASL_ABSOLUTE_X(CPUState *state);  // eg: ASL $0000,X
void ASL_ABSOLUTE_Y(CPUState *state);  // eg: ASL $0000,Y
void ASL_INDIRECT_X(CPUState *state);  // eg: ASL ($00,X)
void ASL_INDIRECT_Y(CPUState *state);  // eg: ASL ($00),Y

void BCC(CPUState *state);             // eg: BCC $00
void BCS(CPUState *state);             // eg: BCS $00
void BEQ(CPUState *state);             // eg: BEQ $00
void BMI(CPUState *state);             // eg: BMI $00
void BNE(CPUState *state);             // eg: BNE $00
void BPL(CPUState *state);             // eg: BPL $00
void BRK(CPUState *state);             // eg: BRK
void BVC(CPUState *state);             // eg: BVC $00
void BVS(CPUState *state);             // eg: BVS $00

void CLC(CPUState *state);             // eg: CLC
void CLD(CPUState *state);             // eg: CLD
void CLI(CPUState *state);             // eg: CLI
void CLV(CPUState *state);             // eg: CLV

void CMP_IMMEDIATE(CPUState *state);   // eg: CMP #$01
void CMP_ZEROPAGE(CPUState *state);   // eg: CMP $00
void CMP_ZEROPAGE_X(CPUState *state);  // eg: CMP $00,X
void CMP_ABSOLUTE(CPUState *state);    // eg: CMP $0000
void CMP_ABSOLUTE_X(CPUState *state);  // eg: CMP $0000,X
void CMP_ABSOLUTE_Y(CPUState *state);  // eg: CMP $0000,Y
void CMP_INDIRECT_X(CPUState *state);  // eg: CMP ($00,X)
void CMP_INDIRECT_Y(CPUState *state);  // eg: CMP ($00),Y

void CPX_IMMEDIATE(CPUState *state);   // eg: CPX #$01
void CPX_ZEROPAGE(CPUState *state);   // eg: CPX $00
void CPX_ABSOLUTE(CPUState *state);    // eg: CPX $0000

void CPY_IMMEDIATE(CPUState *state);   // eg: CPY #$01
void CPY_ZEROPAGE(CPUState *state);   // eg: CPY $00
void CPY_ABSOLUTE(CPUState *state);    // eg: CPY $0000

void DEC_ZEROPAGE(CPUState *state);   // eg: DEC $00
void DEC_ZEROPAGE_X(CPUState *state);  // eg: DEC $00,X
void DEC_ABSOLUTE(CPUState *state);    // eg: DEC $0000
void DEC_ABSOLUTE_X(CPUState *state);  // eg: DEC $0000,X
void DEC_ABSOLUTE_Y(CPUState *state);  // eg: DEC $0000,Y

void DEX(CPUState *state);             // eg: DEX
void DEY(CPUState *state);             // eg: DEY

void EOR_IMMEDIATE(CPUState *state);   // eg: EOR #$01
void EOR_ZEROPAGE(CPUState *state);   // eg: EOR $00
void EOR_ZEROPAGE_X(CPUState *state);  // eg: EOR $00,X
void EOR_ABSOLUTE(CPUState *state);    // eg: EOR $0000
void EOR_ABSOLUTE_X(CPUState *state);  // eg: EOR $0000,X
void EOR_ABSOLUTE_Y(CPUState *state);  // eg: EOR $0000,Y
void EOR_INDIRECT_X(CPUState *state);  // eg: EOR ($00,X)
void EOR_INDIRECT_Y(CPUState *state);  // eg: EOR ($00),Y

void INC_ZEROPAGE(CPUState *state);   // eg: INC $00
void INC_ZEROPAGE_X(CPUState *state);  // eg: INC $00,X
void INC_ABSOLUTE(CPUState *state);    // eg: INC $0000
void INC_ABSOLUTE_X(CPUState *state);  // eg: INC $0000,X
void INC_ABSOLUTE_Y(CPUState *state);  // eg: INC $0000,Y

void INX(CPUState *state);             // eg: INX
void INY(CPUState *state);             // eg: INY

void JMP_ABSOLUTE(CPUState *state);    // eg: JMP $0000
void JMP_INDIRECT(CPUState *state);    // eg: JMP ($0000)

void JSR(CPUState *state);             // eg: JSR $0000
void SYSCALL(CPUState *state);         // eg: SYSCALL #$00
void HELLO_FROM_ARM(CPUState *state);

void LDA_IMMEDIATE(CPUState *state);   // eg: LDA #$01
void LDA_ZEROPAGE(CPUState *state);   // eg: LDA $00
void LDA_ZEROPAGE_X(CPUState *state);  // eg: LDA $00,X
void LDA_ABSOLUTE(CPUState *state);    // eg: LDA $0000
void LDA_ABSOLUTE_X(CPUState *state);  // eg: LDA $0000,X
void LDA_ABSOLUTE_Y(CPUState *state);  // eg: LDA $0000,Y
void LDA_INDIRECT_X(CPUState *state);  // eg: LDA ($00,X)
void LDA_INDIRECT_Y(CPUState *state);  // eg: LDA ($00),Y

void LDX_IMMEDIATE(CPUState *state);   // eg: LDX #$01
void LDX_ZEROPAGE(CPUState *state);   // eg: LDX $00
void LDX_ZEROPAGE_Y(CPUState *state);  // eg: LDX $00,Y
void LDX_ABSOLUTE(CPUState *state);    // eg: LDX $0000
void LDX_ABSOLUTE_Y(CPUState *state);  // eg: LDX $0000,Y

void LDY_IMMEDIATE(CPUState *state);   // eg: LDY #$01
void LDY_ZEROPAGE(CPUState *state);   // eg: LDY $00
void LDY_ZEROPAGE_X(CPUState *state);  // eg: LDY $00,X
void LDY_ABSOLUTE(CPUState *state);    // eg: LDY $0000
void LDY_ABSOLUTE_X(CPUState *state);  // eg: LDY $0000,X
void LDY_INDIRECT_X(CPUState *state);  // eg: LDY ($00,X)
void LDY_INDIRECT_Y(CPUState *state);  // eg: LDY ($00),Y

void NOP(CPUState *state);             // eg: NOP

void LSR_IMMEDIATE(CPUState *state);   // eg: LSR #$01
void LSR_ZEROPAGE(CPUState *state);   // eg: LSR $00
void LSR_ZEROPAGE_X(CPUState *state);  // eg: LSR $00,X
void LSR_ABSOLUTE(CPUState *state);    // eg: LSR $0000
void LSR_ABSOLUTE_X(CPUState *state);  // eg: LSR $0000,X
void LSR_ABSOLUTE_Y(CPUState *state);  // eg: LSR $0000,Y
void LSR_INDIRECT_X(CPUState *state);  // eg: LSR ($00,X)
void LSR_INDIRECT_Y(CPUState *state);  // eg: LSR ($00),Y

void ORA_IMMEDIATE(CPUState *state);   // eg: ORA #$01
void ORA_ZEROPAGE(CPUState *state);   // eg: ORA $00
void ORA_ZEROPAGE_X(CPUState *state);  // eg: ORA $00,X
void ORA_ABSOLUTE(CPUState *state);    // eg: ORA $0000
void ORA_ABSOLUTE_X(CPUState *state);  // eg: ORA $0000,X
void ORA_ABSOLUTE_Y(CPUState *state);  // eg: ORA $0000,Y
void ORA_INDIRECT_X(CPUState *state);  // eg: ORA ($00,X)
void ORA_INDIRECT_Y(CPUState *state);  // eg: ORA ($00),Y

void PHA(CPUState *state);             // eg: PHA
void PHP(CPUState *state);             // eg: PHP
void PLA(CPUState *state);             // eg: PLA
void PLP(CPUState *state);             // eg: PLP

void ROL_ACCUMULATOR(CPUState *state); // eg: ROL #$01
void ROL_ZEROPAGE(CPUState *state);   // eg: ROL $00
void ROL_ZEROPAGE_X(CPUState *state);  // eg: ROL $00,X
void ROL_ABSOLUTE(CPUState *state);    // eg: ROL $0000
void ROL_ABSOLUTE_X(CPUState *state);  // eg: ROL $0000,X

void ROR_ACCUMULATOR(CPUState *state); // eg: ROR #$01
void ROR_ZEROPAGE(CPUState *state);   // eg: ROR $00
void ROR_ZEROPAGE_X(CPUState *state);  // eg: ROR $00,X
void ROR_ABSOLUTE(CPUState *state);    // eg: ROR $0000
void ROR_ABSOLUTE_X(CPUState *state);  // eg: ROR $0000,X

void RTI(CPUState *state);             // eg: RTI
void RTS(CPUState *state);             // eg: RTS

void SBC_IMMEDIATE(CPUState *state);   // eg: SBC #$01
void SBC_ZEROPAGE(CPUState *state);   // eg: SBC $00
void SBC_ZEROPAGE_X(CPUState *state);  // eg: SBC $00,X
void SBC_ABSOLUTE(CPUState *state);    // eg: SBC $0000
void SBC_ABSOLUTE_X(CPUState *state);  // eg: SBC $0000,X
void SBC_ABSOLUTE_Y(CPUState *state);  // eg: SBC $0000,Y
void SBC_INDIRECT_X(CPUState *state);  // eg: SBC ($00,X)
void SBC_INDIRECT_Y(CPUState *state);  // eg: SBC ($00),Y

void STA_ZEROPAGE(CPUState *state);   // eg: STA $00
void STA_ZEROPAGE_X(CPUState *state);  // eg: STA $00,X
void STA_ABSOLUTE(CPUState *state);    // eg: STA $0000
void STA_ABSOLUTE_X(CPUState *state);  // eg: STA $0000,X
void STA_ABSOLUTE_Y(CPUState *state);  // eg: STA $0000,Y
void STA_INDIRECT_X(CPUState *state);  // eg: STA ($00,X)
void STA_INDIRECT_Y(CPUState *state);  // eg: STA ($00),Y

void STX_ZEROPAGE(CPUState *state);   // eg: STX $00
void STX_ZEROPAGE_Y(CPUState *state);  // eg: STX $00,Y
void STX_ABSOLUTE(CPUState *state);    // eg: STX $0000

void STY_ZEROPAGE(CPUState *state);   // eg: STY $00
void STY_ZEROPAGE_X(CPUState *state);  // eg: STY $00,X
void STY_ABSOLUTE(CPUState *state);    // eg: STY $0000

void SEC(CPUState *state);             // eg: SEC
void SED(CPUState *state);             // eg: SED
void SEI(CPUState *state);             // eg: SEI

void TAX(CPUState *state);             // eg: TAX
void TAY(CPUState *state);             // eg: TAY

void TSX(CPUState *state);             // eg: TSX
void TXA(CPUState *state);             // eg: TXA

void TXS(CPUState *state);             // eg: TXS
void TYA(CPUState *state);             // eg: TYA

void execute_opcode(CPUState *state, uint8_t opcode);

// array of function pointers for the opcodes (defined in opcodes.c)
extern void (*opcode_functions[256])(CPUState *state);
extern void (*arm_opcode_functions[256])(CPUState *state);

#endif // OPCODES_H
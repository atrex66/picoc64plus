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
static inline void write_memory(CPUState *state, uint16_t address, uint8_t value);

void reset_cpu(CPUState *state);
static inline void trigger_irq(CPUState *state, uint16_t irq_source);


static inline void BCD_ADD(CPUState *state, uint8_t value);
static inline void BCD_SUB(CPUState *state, uint8_t value);

static inline void BIT_ZEROPAGE(CPUState *state);
static inline void BIT_ABSOLUTE(CPUState *state);

static inline void ADC_IMMEDIATE(CPUState *state);   // eg: ADC #$01
static inline void ADC_ZEROPAGE(CPUState *state);   // eg: ADC $00
static inline void ADC_ZEROPAGE_X(CPUState *state);  // eg: ADC $00,X
static inline void ADC_ABSOLUTE(CPUState *state);    // eg: ADC $0000
static inline void ADC_ABSOLUTE_X(CPUState *state);  // eg: ADC $0000,X
static inline void ADC_ABSOLUTE_Y(CPUState *state);  // eg: ADC $0000,Y
static inline void ADC_INDIRECT_X(CPUState *state);  // eg: ADC ($00,X)
static inline void ADC_INDIRECT_Y(CPUState *state);  // eg: ADC ($00),Y

static inline void AND_IMMEDIATE(CPUState *state);   // eg: AND #$01
static inline void AND_ZEROPAGE(CPUState *state);   // eg: AND $00
static inline void AND_ZEROPAGE_X(CPUState *state);  // eg: AND $00,X
static inline void AND_ABSOLUTE(CPUState *state);    // eg: AND $0000
static inline void AND_ABSOLUTE_X(CPUState *state);  // eg: AND $0000,X
static inline void AND_ABSOLUTE_Y(CPUState *state);  // eg: AND $0000,Y
static inline void AND_INDIRECT_X(CPUState *state);  // eg: AND ($00,X)
static inline void AND_INDIRECT_Y(CPUState *state);  // eg: AND ($00),Y

static inline void ASL_IMMEDIATE(CPUState *state);   // eg: ASL #$01
static inline void ASL_ZEROPAGE(CPUState *state);   // eg: ASL $00
static inline void ASL_ZEROPAGE_X(CPUState *state);  // eg: ASL $00,X
static inline void ASL_ABSOLUTE(CPUState *state);    // eg: ASL $0000
static inline void ASL_ABSOLUTE_X(CPUState *state);  // eg: ASL $0000,X
static inline void ASL_ABSOLUTE_Y(CPUState *state);  // eg: ASL $0000,Y
static inline void ASL_INDIRECT_X(CPUState *state);  // eg: ASL ($00,X)
static inline void ASL_INDIRECT_Y(CPUState *state);  // eg: ASL ($00),Y

static inline void BCC(CPUState *state);             // eg: BCC $00
static inline void BCS(CPUState *state);             // eg: BCS $00
static inline void BEQ(CPUState *state);             // eg: BEQ $00
static inline void BMI(CPUState *state);             // eg: BMI $00
static inline void BNE(CPUState *state);             // eg: BNE $00
static inline void BPL(CPUState *state);             // eg: BPL $00
static inline void BRK(CPUState *state);             // eg: BRK
static inline void BVC(CPUState *state);             // eg: BVC $00
static inline void BVS(CPUState *state);             // eg: BVS $00

static inline void CLC(CPUState *state);             // eg: CLC
static inline void CLD(CPUState *state);             // eg: CLD
static inline void CLI(CPUState *state);             // eg: CLI
static inline void CLV(CPUState *state);             // eg: CLV

static inline void CMP_IMMEDIATE(CPUState *state);   // eg: CMP #$01
static inline void CMP_ZEROPAGE(CPUState *state);   // eg: CMP $00
static inline void CMP_ZEROPAGE_X(CPUState *state);  // eg: CMP $00,X
static inline void CMP_ABSOLUTE(CPUState *state);    // eg: CMP $0000
static inline void CMP_ABSOLUTE_X(CPUState *state);  // eg: CMP $0000,X
static inline void CMP_ABSOLUTE_Y(CPUState *state);  // eg: CMP $0000,Y
static inline void CMP_INDIRECT_X(CPUState *state);  // eg: CMP ($00,X)
static inline void CMP_INDIRECT_Y(CPUState *state);  // eg: CMP ($00),Y

static inline void CPX_IMMEDIATE(CPUState *state);   // eg: CPX #$01
static inline void CPX_ZEROPAGE(CPUState *state);   // eg: CPX $00
static inline void CPX_ABSOLUTE(CPUState *state);    // eg: CPX $0000

static inline void CPY_IMMEDIATE(CPUState *state);   // eg: CPY #$01
static inline void CPY_ZEROPAGE(CPUState *state);   // eg: CPY $00
static inline void CPY_ABSOLUTE(CPUState *state);    // eg: CPY $0000

static inline void DEC_ZEROPAGE(CPUState *state);   // eg: DEC $00
static inline void DEC_ZEROPAGE_X(CPUState *state);  // eg: DEC $00,X
static inline void DEC_ABSOLUTE(CPUState *state);    // eg: DEC $0000
static inline void DEC_ABSOLUTE_X(CPUState *state);  // eg: DEC $0000,X
static inline void DEC_ABSOLUTE_Y(CPUState *state);  // eg: DEC $0000,Y

static inline void DEX(CPUState *state);             // eg: DEX
static inline void DEY(CPUState *state);             // eg: DEY

static inline void EOR_IMMEDIATE(CPUState *state);   // eg: EOR #$01
static inline void EOR_ZEROPAGE(CPUState *state);   // eg: EOR $00
static inline void EOR_ZEROPAGE_X(CPUState *state);  // eg: EOR $00,X
static inline void EOR_ABSOLUTE(CPUState *state);    // eg: EOR $0000
static inline void EOR_ABSOLUTE_X(CPUState *state);  // eg: EOR $0000,X
static inline void EOR_ABSOLUTE_Y(CPUState *state);  // eg: EOR $0000,Y
static inline void EOR_INDIRECT_X(CPUState *state);  // eg: EOR ($00,X)
static inline void EOR_INDIRECT_Y(CPUState *state);  // eg: EOR ($00),Y

static inline void INC_ZEROPAGE(CPUState *state);   // eg: INC $00
static inline void INC_ZEROPAGE_X(CPUState *state);  // eg: INC $00,X
static inline void INC_ABSOLUTE(CPUState *state);    // eg: INC $0000
static inline void INC_ABSOLUTE_X(CPUState *state);  // eg: INC $0000,X
static inline void INC_ABSOLUTE_Y(CPUState *state);  // eg: INC $0000,Y

static inline void INX(CPUState *state);             // eg: INX
static inline void INY(CPUState *state);             // eg: INY

static inline void JMP_ABSOLUTE(CPUState *state);    // eg: JMP $0000
static inline void JMP_INDIRECT(CPUState *state);    // eg: JMP ($0000)

static inline void JSR(CPUState *state);             // eg: JSR $0000
static inline void SYSCALL(CPUState *state);         // eg: SYSCALL #$00
static inline void HELLO_FROM_ARM(CPUState *state);

static inline void LDA_IMMEDIATE(CPUState *state);   // eg: LDA #$01
static inline void LDA_ZEROPAGE(CPUState *state);   // eg: LDA $00
static inline void LDA_ZEROPAGE_X(CPUState *state);  // eg: LDA $00,X
static inline void LDA_ABSOLUTE(CPUState *state);    // eg: LDA $0000
static inline void LDA_ABSOLUTE_X(CPUState *state);  // eg: LDA $0000,X
static inline void LDA_ABSOLUTE_Y(CPUState *state);  // eg: LDA $0000,Y
static inline void LDA_INDIRECT_X(CPUState *state);  // eg: LDA ($00,X)
static inline void LDA_INDIRECT_Y(CPUState *state);  // eg: LDA ($00),Y

static inline void LDX_IMMEDIATE(CPUState *state);   // eg: LDX #$01
static inline void LDX_ZEROPAGE(CPUState *state);   // eg: LDX $00
static inline void LDX_ZEROPAGE_Y(CPUState *state);  // eg: LDX $00,Y
static inline void LDX_ABSOLUTE(CPUState *state);    // eg: LDX $0000
static inline void LDX_ABSOLUTE_Y(CPUState *state);  // eg: LDX $0000,Y

static inline void LDY_IMMEDIATE(CPUState *state);   // eg: LDY #$01
static inline void LDY_ZEROPAGE(CPUState *state);   // eg: LDY $00
static inline void LDY_ZEROPAGE_X(CPUState *state);  // eg: LDY $00,X
static inline void LDY_ABSOLUTE(CPUState *state);    // eg: LDY $0000
static inline void LDY_ABSOLUTE_X(CPUState *state);  // eg: LDY $0000,X
static inline void LDY_INDIRECT_X(CPUState *state);  // eg: LDY ($00,X)
static inline void LDY_INDIRECT_Y(CPUState *state);  // eg: LDY ($00),Y

static inline void NOP(CPUState *state);             // eg: NOP

static inline void LSR_IMMEDIATE(CPUState *state);   // eg: LSR #$01
static inline void LSR_ZEROPAGE(CPUState *state);   // eg: LSR $00
static inline void LSR_ZEROPAGE_X(CPUState *state);  // eg: LSR $00,X
static inline void LSR_ABSOLUTE(CPUState *state);    // eg: LSR $0000
static inline void LSR_ABSOLUTE_X(CPUState *state);  // eg: LSR $0000,X
static inline void LSR_ABSOLUTE_Y(CPUState *state);  // eg: LSR $0000,Y
static inline void LSR_INDIRECT_X(CPUState *state);  // eg: LSR ($00,X)
static inline void LSR_INDIRECT_Y(CPUState *state);  // eg: LSR ($00),Y

static inline void ORA_IMMEDIATE(CPUState *state);   // eg: ORA #$01
static inline void ORA_ZEROPAGE(CPUState *state);   // eg: ORA $00
static inline void ORA_ZEROPAGE_X(CPUState *state);  // eg: ORA $00,X
static inline void ORA_ABSOLUTE(CPUState *state);    // eg: ORA $0000
static inline void ORA_ABSOLUTE_X(CPUState *state);  // eg: ORA $0000,X
static inline void ORA_ABSOLUTE_Y(CPUState *state);  // eg: ORA $0000,Y
static inline void ORA_INDIRECT_X(CPUState *state);  // eg: ORA ($00,X)
static inline void ORA_INDIRECT_Y(CPUState *state);  // eg: ORA ($00),Y

static inline void PHA(CPUState *state);             // eg: PHA
static inline void PHP(CPUState *state);             // eg: PHP
static inline void PLA(CPUState *state);             // eg: PLA
static inline void PLP(CPUState *state);             // eg: PLP

static inline void ROL_ACCUMULATOR(CPUState *state); // eg: ROL #$01
static inline void ROL_ZEROPAGE(CPUState *state);   // eg: ROL $00
static inline void ROL_ZEROPAGE_X(CPUState *state);  // eg: ROL $00,X
static inline void ROL_ABSOLUTE(CPUState *state);    // eg: ROL $0000
static inline void ROL_ABSOLUTE_X(CPUState *state);  // eg: ROL $0000,X

static inline void ROR_ACCUMULATOR(CPUState *state); // eg: ROR #$01
static inline void ROR_ZEROPAGE(CPUState *state);   // eg: ROR $00
static inline void ROR_ZEROPAGE_X(CPUState *state);  // eg: ROR $00,X
static inline void ROR_ABSOLUTE(CPUState *state);    // eg: ROR $0000
static inline void ROR_ABSOLUTE_X(CPUState *state);  // eg: ROR $0000,X

static inline void RTI(CPUState *state);             // eg: RTI
static inline void RTS(CPUState *state);             // eg: RTS

static inline void SBC_IMMEDIATE(CPUState *state);   // eg: SBC #$01
static inline void SBC_ZEROPAGE(CPUState *state);   // eg: SBC $00
static inline void SBC_ZEROPAGE_X(CPUState *state);  // eg: SBC $00,X
static inline void SBC_ABSOLUTE(CPUState *state);    // eg: SBC $0000
static inline void SBC_ABSOLUTE_X(CPUState *state);  // eg: SBC $0000,X
static inline void SBC_ABSOLUTE_Y(CPUState *state);  // eg: SBC $0000,Y
static inline void SBC_INDIRECT_X(CPUState *state);  // eg: SBC ($00,X)
static inline void SBC_INDIRECT_Y(CPUState *state);  // eg: SBC ($00),Y

static inline void STA_ZEROPAGE(CPUState *state);   // eg: STA $00
static inline void STA_ZEROPAGE_X(CPUState *state);  // eg: STA $00,X
static inline void STA_ABSOLUTE(CPUState *state);    // eg: STA $0000
static inline void STA_ABSOLUTE_X(CPUState *state);  // eg: STA $0000,X
static inline void STA_ABSOLUTE_Y(CPUState *state);  // eg: STA $0000,Y
static inline void STA_INDIRECT_X(CPUState *state);  // eg: STA ($00,X)
static inline void STA_INDIRECT_Y(CPUState *state);  // eg: STA ($00),Y

static inline void STX_ZEROPAGE(CPUState *state);   // eg: STX $00
static inline void STX_ZEROPAGE_Y(CPUState *state);  // eg: STX $00,Y
static inline void STX_ABSOLUTE(CPUState *state);    // eg: STX $0000

static inline void STY_ZEROPAGE(CPUState *state);   // eg: STY $00
static inline void STY_ZEROPAGE_X(CPUState *state);  // eg: STY $00,X
static inline void STY_ABSOLUTE(CPUState *state);    // eg: STY $0000

static inline void SEC(CPUState *state);             // eg: SEC
static inline void SED(CPUState *state);             // eg: SED
static inline void SEI(CPUState *state);             // eg: SEI

static inline void TAX(CPUState *state);             // eg: TAX
static inline void TAY(CPUState *state);             // eg: TAY

static inline void TSX(CPUState *state);             // eg: TSX
static inline void TXA(CPUState *state);             // eg: TXA

static inline void TXS(CPUState *state);             // eg: TXS
static inline void TYA(CPUState *state);             // eg: TYA

void execute_opcode(CPUState *state, uint8_t opcode);

// array of function pointers for the opcodes (defined in opcodes.c)
extern void (*opcode_functions[256])(CPUState *state);
extern void (*arm_opcode_functions[256])(CPUState *state);

#endif // OPCODES_H
#ifndef MCS51_INSTR_LIST_H
#define MCS51_INSTR_LIST_H

#define INSTR_LIST(X) \
    X(0x00, MN_NOP,  AM_NONE,   AM_NONE,  1, "NOP") \
    X(0x04, MN_INC,  AM_A,      AM_NONE,  1, "INC A") \
    X(0x14, MN_DEC,  AM_A,      AM_NONE,  1, "DEC A") \
    X(0x24, MN_ADD,  AM_NONE,   AM_IMM8,  2, "ADD A,#imm") \
    X(0x74, MN_MOV,  AM_A,      AM_IMM8,  2, "MOV A,#imm") \
    X(0x75, MN_MOV,  AM_DIRECT, AM_IMM8,  3, "MOV direct,#imm") \
    X(0xE5, MN_MOV,  AM_A,      AM_DIRECT,2, "MOV A,direct") \
    X(0xF5, MN_MOV,  AM_DIRECT, AM_A,     2, "MOV direct,A") \
    X(0x80, MN_SJMP, AM_REL8,   AM_NONE,  2, "SJMP rel") \
    X(0x02, MN_LJMP, AM_ADDR16, AM_NONE,  3, "LJMP addr16") \
    X(0x01, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0x21, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0x41, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0x61, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0x81, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0xA1, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0xC1, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0xE1, MN_AJMP, AM_ADDR11, AM_NONE,  2, "AJMP addr11") \
    X(0xD2, MN_SETB, AM_BIT,    AM_NONE,  2, "SETB bit") \
    X(0xC2, MN_CLR,  AM_BIT,    AM_NONE,  2, "CLR bit") \
    X(0xC0, MN_PUSH, AM_DIRECT, AM_NONE,  2, "PUSH direct") \
    X(0xD0, MN_POP,  AM_DIRECT, AM_NONE,  2, "POP direct") \
    X(0x06, MN_INC,  AM_AT_RI,  AM_NONE,  1, "INC @Ri") \
    X(0x07, MN_INC,  AM_AT_RI,  AM_NONE,  1, "INC @Ri") \
    X(0x08, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x09, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0A, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0B, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0C, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0D, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0E, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x0F, MN_INC,  AM_RN,     AM_NONE,  1, "INC Rn") \
    X(0x16, MN_DEC,  AM_AT_RI,  AM_NONE,  1, "DEC @Ri") \
    X(0x17, MN_DEC,  AM_AT_RI,  AM_NONE,  1, "DEC @Ri") \
    X(0x18, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x19, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1A, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1B, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1C, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1D, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1E, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x1F, MN_DEC,  AM_RN,     AM_NONE,  1, "DEC Rn") \
    X(0x26, MN_ADD,  AM_NONE,   AM_AT_RI, 1, "ADD A,@Ri") \
    X(0x27, MN_ADD,  AM_NONE,   AM_AT_RI, 1, "ADD A,@Ri") \
    X(0x28, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x29, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2A, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2B, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2C, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2D, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2E, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x2F, MN_ADD,  AM_NONE,   AM_RN,    1, "ADD A,Rn") \
    X(0x76, MN_MOV,  AM_AT_RI,  AM_IMM8,  2, "MOV @Ri,#imm") \
    X(0x77, MN_MOV,  AM_AT_RI,  AM_IMM8,  2, "MOV @Ri,#imm") \
    X(0x78, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x79, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7A, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7B, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7C, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7D, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7E, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0x7F, MN_MOV,  AM_RN,     AM_IMM8,  2, "MOV Rn,#imm") \
    X(0xE6, MN_MOV,  AM_A,      AM_AT_RI, 1, "MOV A,@Ri") \
    X(0xE7, MN_MOV,  AM_A,      AM_AT_RI, 1, "MOV A,@Ri") \
    X(0xE8, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xE9, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xEA, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xEB, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xEC, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xED, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xEE, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xEF, MN_MOV,  AM_A,      AM_RN,    1, "MOV A,Rn") \
    X(0xF6, MN_MOV,  AM_AT_RI,  AM_A,     1, "MOV @Ri,A") \
    X(0xF7, MN_MOV,  AM_AT_RI,  AM_A,     1, "MOV @Ri,A") \
    X(0xF8, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xF9, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFA, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFB, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFC, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFD, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFE, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0xFF, MN_MOV,  AM_RN,     AM_A,     1, "MOV Rn,A") \
    X(0x88, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x89, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8A, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8B, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8C, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8D, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8E, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0x8F, MN_MOV,  AM_DIRECT, AM_RN,    2, "MOV direct,Rn") \
    X(0xA8, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xA9, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAA, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAB, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAC, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAD, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAE, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0xAF, MN_MOV,  AM_RN,     AM_DIRECT,2, "MOV Rn,direct") \
    X(0x54, MN_ANL,  AM_A,      AM_IMM8,  2, "ANL A,#imm") \
    X(0x55, MN_ANL,  AM_A,      AM_DIRECT,2, "ANL A,direct") \
    X(0x56, MN_ANL,  AM_A,      AM_AT_RI, 1, "ANL A,@Ri") \
    X(0x57, MN_ANL,  AM_A,      AM_AT_RI, 1, "ANL A,@Ri") \
    X(0x58, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x59, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5A, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5B, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5C, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5D, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5E, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x5F, MN_ANL,  AM_A,      AM_RN,    1, "ANL A,Rn") \
    X(0x52, MN_ANL,  AM_DIRECT, AM_A,     2, "ANL direct,A") \
    X(0x53, MN_ANL,  AM_DIRECT, AM_IMM8,  3, "ANL direct,#imm") \
    X(0x44, MN_ORL,  AM_A,      AM_IMM8,  2, "ORL A,#imm") \
    X(0x45, MN_ORL,  AM_A,      AM_DIRECT,2, "ORL A,direct") \
    X(0x46, MN_ORL,  AM_A,      AM_AT_RI, 1, "ORL A,@Ri") \
    X(0x47, MN_ORL,  AM_A,      AM_AT_RI, 1, "ORL A,@Ri") \
    X(0x48, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x49, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4A, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4B, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4C, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4D, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4E, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x4F, MN_ORL,  AM_A,      AM_RN,    1, "ORL A,Rn") \
    X(0x42, MN_ORL,  AM_DIRECT, AM_A,     2, "ORL direct,A") \
    X(0x43, MN_ORL,  AM_DIRECT, AM_IMM8,  3, "ORL direct,#imm") \
    X(0x64, MN_XRL,  AM_A,      AM_IMM8,  2, "XRL A,#imm") \
    X(0x65, MN_XRL,  AM_A,      AM_DIRECT,2, "XRL A,direct") \
    X(0x66, MN_XRL,  AM_A,      AM_AT_RI, 1, "XRL A,@Ri") \
    X(0x67, MN_XRL,  AM_A,      AM_AT_RI, 1, "XRL A,@Ri") \
    X(0x68, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x69, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6A, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6B, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6C, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6D, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6E, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x6F, MN_XRL,  AM_A,      AM_RN,    1, "XRL A,Rn") \
    X(0x62, MN_XRL,  AM_DIRECT, AM_A,     2, "XRL direct,A") \
    X(0x63, MN_XRL,  AM_DIRECT, AM_IMM8,  3, "XRL direct,#imm") \
    X(0x86, MN_MOV,  AM_DIRECT, AM_AT_RI, 2, "MOV direct,@Ri") \
    X(0x87, MN_MOV,  AM_DIRECT, AM_AT_RI, 2, "MOV direct,@Ri") \
    X(0xA6, MN_MOV,  AM_AT_RI,  AM_DIRECT,2, "MOV @Ri,direct") \
    X(0xA7, MN_MOV,  AM_AT_RI,  AM_DIRECT,2, "MOV @Ri,direct") \
    X(0x82, MN_ANL,  AM_C,      AM_BIT,   2, "ANL C,bit") \
    X(0xB0, MN_ANL,  AM_C,      AM_BIT_NOT,2, "ANL C,/bit") \
    X(0x72, MN_ORL,  AM_C,      AM_BIT,   2, "ORL C,bit") \
    X(0xA0, MN_ORL,  AM_C,      AM_BIT_NOT,2, "ORL C,/bit") \
    X(0x25, MN_ADD,  AM_A,      AM_DIRECT,2, "ADD A,direct") \
    X(0x34, MN_ADDC, AM_A,      AM_IMM8,  2, "ADDC A,#imm") \
    X(0x35, MN_ADDC, AM_A,      AM_DIRECT,2, "ADDC A,direct") \
    X(0x36, MN_ADDC, AM_A,      AM_AT_RI, 1, "ADDC A,@Ri") \
    X(0x37, MN_ADDC, AM_A,      AM_AT_RI, 1, "ADDC A,@Ri") \
    X(0x38, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x39, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3A, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3B, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3C, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3D, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3E, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0x3F, MN_ADDC, AM_A,      AM_RN,    1, "ADDC A,Rn") \
    X(0xC3, MN_CLR,  AM_C,      AM_NONE,  1, "CLR C") \
    X(0xD3, MN_SETB, AM_C,      AM_NONE,  1, "SETB C") \
    X(0xB3, MN_CPL,  AM_C,      AM_NONE,  1, "CPL C") \
    X(0xF4, MN_CPL,  AM_A,      AM_NONE,  1, "CPL A") \
    X(0xB2, MN_CPL,  AM_BIT,    AM_NONE,  2, "CPL bit") \
    X(0x11, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0x31, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0x51, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0x71, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0x91, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0xB1, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0xD1, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0xF1, MN_ACALL, AM_ADDR11, AM_NONE, 2, "ACALL addr11") \
    X(0x12, MN_LCALL, AM_ADDR16, AM_NONE, 3, "LCALL addr16") \
    X(0x22, MN_RET,   AM_NONE,  AM_NONE, 1, "RET") \
    X(0x32, MN_RETI,  AM_NONE,  AM_NONE, 1, "RETI") \
    X(0x40, MN_JC,   AM_REL8,  AM_NONE,  2, "JC rel") \
    X(0x50, MN_JNC,  AM_REL8,  AM_NONE,  2, "JNC rel") \
    X(0x60, MN_JZ,   AM_REL8,  AM_NONE,  2, "JZ rel") \
    X(0x70, MN_JNZ,  AM_REL8,  AM_NONE,  2, "JNZ rel") \
    X(0x10, MN_JBC,  AM_BIT,   AM_REL8,  3, "JBC bit,rel") \
    X(0x20, MN_JB,   AM_BIT,   AM_REL8,  3, "JB bit,rel") \
    X(0x30, MN_JNB,  AM_BIT,   AM_REL8,  3, "JNB bit,rel") \
    X(0xD5, MN_DJNZ, AM_DIRECT,AM_REL8,  3, "DJNZ direct,rel") \
    X(0xD8, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xD9, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDA, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDB, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDC, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDD, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDE, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xDF, MN_DJNZ, AM_RN,    AM_REL8,  2, "DJNZ Rn,rel") \
    X(0xB4, MN_CJNE_A_IMM,     AM_REL8,  AM_IMM8, 3, "CJNE A,#imm,rel") \
    X(0xB5, MN_CJNE_A_DIRECT,  AM_REL8,  AM_DIRECT,3, "CJNE A,direct,rel") \
    X(0xB6, MN_CJNE_AT_RI_IMM, AM_REL8,  AM_IMM8, 3, "CJNE @Ri,#imm,rel") \
    X(0xB7, MN_CJNE_AT_RI_IMM, AM_REL8,  AM_IMM8, 3, "CJNE @Ri,#imm,rel") \
    X(0xB8, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xB9, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBA, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBB, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBC, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBD, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBE, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0xBF, MN_CJNE_RN_IMM,    AM_REL8,  AM_IMM8, 3, "CJNE Rn,#imm,rel") \
    X(0x85, MN_MOV_DIRECT_DIRECT, AM_DIRECT, AM_DIRECT, 3, "MOV direct,direct") \
    X(0x90, MN_MOV_DPTR_IMM, AM_ADDR16, AM_NONE, 3, "MOV DPTR,#imm16") \
    X(0xA2, MN_MOV,  AM_C,      AM_BIT,   2, "MOV C,bit") \
    X(0x92, MN_MOV,  AM_BIT,    AM_C,     2, "MOV bit,C") \
    X(0x73, MN_JMPA_DPTR, AM_NONE, AM_NONE, 1, "JMP @A+DPTR") \
    X(0xE0, MN_MOVX_A_DPTR,  AM_NONE, AM_NONE, 1, "MOVX A,@DPTR") \
    X(0xF0, MN_MOVX_DPTR_A,  AM_NONE, AM_NONE, 1, "MOVX @DPTR,A") \
    X(0xE2, MN_MOVX_A_AT_RI, AM_AT_RI, AM_NONE, 1, "MOVX A,@Ri") \
    X(0xE3, MN_MOVX_A_AT_RI, AM_AT_RI, AM_NONE, 1, "MOVX A,@Ri") \
    X(0xF2, MN_MOVX_AT_RI_A, AM_AT_RI, AM_NONE, 1, "MOVX @Ri,A") \
    X(0xF3, MN_MOVX_AT_RI_A, AM_AT_RI, AM_NONE, 1, "MOVX @Ri,A") \
    X(0x93, MN_MOVC_A_DPTR,  AM_NONE, AM_NONE, 1, "MOVC A,@A+DPTR") \
    X(0x83, MN_MOVC_A_PC,    AM_NONE, AM_NONE, 1, "MOVC A,@A+PC") \
    X(0x94, MN_SUBB, AM_A,      AM_IMM8,  2, "SUBB A,#imm") \
    X(0x95, MN_SUBB, AM_A,      AM_DIRECT,2, "SUBB A,direct") \
    X(0x96, MN_SUBB, AM_A,      AM_AT_RI, 1, "SUBB A,@Ri") \
    X(0x97, MN_SUBB, AM_A,      AM_AT_RI, 1, "SUBB A,@Ri") \
    X(0x98, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x99, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9A, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9B, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9C, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9D, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9E, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0x9F, MN_SUBB, AM_A,      AM_RN,    1, "SUBB A,Rn") \
    X(0xD4, MN_DA,   AM_NONE,   AM_NONE,  1, "DA A") \
    X(0xA4, MN_MUL,  AM_NONE,   AM_NONE,  1, "MUL AB") \
    X(0x84, MN_DIV,  AM_NONE,   AM_NONE,  1, "DIV AB") \
    X(0xC5, MN_XCH,  AM_DIRECT, AM_NONE,  2, "XCH A,direct") \
    X(0xC6, MN_XCH,  AM_AT_RI,  AM_NONE,  1, "XCH A,@Ri") \
    X(0xC7, MN_XCH,  AM_AT_RI,  AM_NONE,  1, "XCH A,@Ri") \
    X(0xC8, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xC9, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCA, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCB, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCC, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCD, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCE, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xCF, MN_XCH,  AM_RN,     AM_NONE,  1, "XCH A,Rn") \
    X(0xD6, MN_XCHD, AM_AT_RI,  AM_NONE,  1, "XCHD A,@Ri") \
    X(0xD7, MN_XCHD, AM_AT_RI,  AM_NONE,  1, "XCHD A,@Ri") \
    X(0x05, MN_INC,  AM_DIRECT, AM_NONE,  2, "INC direct") \
    X(0x15, MN_DEC,  AM_DIRECT, AM_NONE,  2, "DEC direct") \
    X(0xA3, MN_INC,  AM_DPTR,   AM_NONE,  1, "INC DPTR") \
    X(0xE4, MN_CLR,  AM_A,      AM_NONE,  1, "CLR A")

#endif

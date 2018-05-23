/*
 * Copyright (C) 2018 ETH Zurich and University of Bologna
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* 
 * Authors: Germain Haugou, ETH (germain.haugou@iis.ee.ethz.ch)
 */

#ifndef __LIB_HPP__
#define __LIB_HPP__
#include <strings.h>
#include <stdio.h>
#define MAX(a,b) ((a)>=(b)?(a):(b))
#define MIN(a,b) ((a)<=(b)?(a):(b))

static inline unsigned int getField(unsigned int val, int shift, int bits)
{
  return (val >> shift) & ((1<<bits) - 1);
}

static inline int getSignedValue(unsigned int val, int bits)
{
  return ((int)val) << (ISS_REG_WIDTH-bits) >> (ISS_REG_WIDTH-bits);
}

static inline iss_opcode_t getSignedField(iss_opcode_t val, int shift, int bits)
{
  return getSignedValue(getField(val, shift, bits), bits);
}


/*
 * LOGICAL OPERATIONS
 */

static inline unsigned int lib_SLL(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a << b; }
static inline unsigned int lib_SRL(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a >> b; }
static inline unsigned int lib_SRA(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return ((int32_t)a) >> b; }
static inline unsigned int lib_ROR(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return (a >> b) | (a << (32 - b)); }
static inline unsigned int lib_XOR(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a ^ b; }
static inline unsigned int lib_OR(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a | b; }
static inline unsigned int lib_AND(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a & b; }




/*
 * ARITHMETIC OPERATIONS
 */

#ifdef ISS_STATE_HAS_CARRY

static inline unsigned int add(iss_cpu_state_t *s, unsigned int a, unsigned int b)
{
  uint64_t result = ((uint64_t)(uint32_t)a) + ((uint64_t)(uint32_t)b);
  s->carry = (result & 0x100000000) != 0;
  s->overflow = ((result & 0x80000000) ^ (a & 0x80000000)) != 0;
  return result; 
}

static inline unsigned int addWithCarry(iss_cpu_state_t *s, unsigned int a, unsigned int b)
{
  uint64_t result = ((uint64_t)(uint32_t)a) + ((uint64_t)(uint32_t)b) + ((uint64_t)(uint32_t)s->carry);
  s->carry = (result & 0x100000000) != 0;
  s->overflow = ((result & 0x80000000) ^ (a & 0x80000000)) != 0;
  return result; 
}

static inline unsigned int lib_ADD_C(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return add(s, a, b); }

#endif

static inline unsigned int lib_ADD(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a + b; }
#ifdef ISS_STATE_HAS_CARRY
static inline unsigned int lib_ADDC_C(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return addWithCarry(s, a, b); }
#endif
static inline unsigned int lib_SUB(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a - b; }

#ifdef ISS_STATE_HAS_CARRY
static inline unsigned int lib_SUB_C(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
 unsigned int result = a - b;
  s->carry = (uint32_t)b > (uint32_t)a;
  s->overflow = (((int32_t) a <  0) && ((int32_t) b >=  0) && ((int32_t) result >= 0)) || (((int32_t) a >= 0) && ((int32_t) b < 0) && ((int32_t) result <  0));
  // TODO seems to be a bug on OR10N, the carry is equivalent to the overflow, whereas it should be different
  s->carry = s->overflow;
  //printf("SUB %x %x -> %x / %d\n", a, b, result, s->carry);
  return result;
}
#endif

static inline unsigned int lib_MAC(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + b * c; }
#ifdef ISS_STATE_HAS_CARRY
static inline unsigned int lib_MAC_C(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, b * c); }
static inline unsigned int lib_MACC(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a, b * c); }
#endif
static inline unsigned int lib_MSU(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - b * c; }
static inline unsigned int lib_MMUL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return - b * c; }

#define SL(val) ((int16_t)((val) & 0xffff))
#define SH(val) ((int16_t)(((val)>>16) & 0xffff))
#define ZL(val) ((uint16_t)((val) & 0xffff))
#define ZH(val) ((uint16_t)(((val)>>16) & 0xffff))
#define L_H_TO_W(l, h) (((l) & 0xffff) | ((h) <<16))

static inline unsigned int lib_MAC_SL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SL(b) * SL(c); }
static inline unsigned int lib_MAC_SL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SL(b) * SH(c); }
static inline unsigned int lib_MAC_SH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SH(b) * SL(c); }
static inline unsigned int lib_MAC_SH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SH(b) * SH(c); }
static inline unsigned int lib_MAC_ZL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZL(b) * ZL(c); }
static inline unsigned int lib_MAC_ZL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZL(b) * ZH(c); }
static inline unsigned int lib_MAC_ZH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZH(b) * ZL(c); }
static inline unsigned int lib_MAC_ZH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZH(b) * ZH(c); }

static inline unsigned int lib_MAC_SL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SL(b) * ZL(c); }
static inline unsigned int lib_MAC_SL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SL(b) * ZH(c); }
static inline unsigned int lib_MAC_SH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SH(b) * ZL(c); }
static inline unsigned int lib_MAC_SH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + SH(b) * ZH(c); }
static inline unsigned int lib_MAC_ZL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZL(b) * SL(c); }
static inline unsigned int lib_MAC_ZL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZL(b) * SH(c); }
static inline unsigned int lib_MAC_ZH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZH(b) * SL(c); }
static inline unsigned int lib_MAC_ZH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a + ZH(b) * SH(c); }

static inline unsigned int lib_MAC_SL_SL_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { return ((int32_t)(a + SL(b) * SL(c))) >> shift; }
static inline unsigned int lib_MAC_SH_SH_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { return ((int32_t)(a + SH(b) * SH(c))) >> shift; }
static inline unsigned int lib_MAC_ZL_ZL_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { return ((uint32_t)(a + ZL(b) * ZL(c))) >> shift; }
static inline unsigned int lib_MAC_ZH_ZH_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { return ((uint32_t)(a + ZH(b) * ZH(c))) >> shift; }

static inline unsigned int lib_MAC_SL_SL_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift)
{ 
  int32_t result = (int32_t)(a + SL(b) * SL(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MAC_SH_SH_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift)
{
  int32_t result = (int32_t)(a + SH(b) * SH(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MAC_ZL_ZL_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift)
{
  uint32_t result = (uint32_t)(a + ZL(b) * ZL(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MAC_ZH_ZH_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift)
{
  uint32_t result = (uint32_t)(a + ZH(b) * ZH(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}

#ifdef ISS_STATE_HAS_CARRY
static inline unsigned int lib_MAC_C_SL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SL(b) * SL(c)); }
static inline unsigned int lib_MAC_C_SL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SL(b) * SH(c)); }
static inline unsigned int lib_MAC_C_SH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SH(b) * SL(c)); }
static inline unsigned int lib_MAC_C_SH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SH(b) * SH(c)); }
static inline unsigned int lib_MAC_C_ZL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZL(b) * ZL(c)); }
static inline unsigned int lib_MAC_C_ZL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZL(b) * ZH(c)); }
static inline unsigned int lib_MAC_C_ZH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZH(b) * ZL(c)); }
static inline unsigned int lib_MAC_C_ZH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZH(b) * ZH(c)); }

static inline unsigned int lib_MAC_C_SL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SL(b) * ZL(c)); }
static inline unsigned int lib_MAC_C_SL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SL(b) * ZH(c)); }
static inline unsigned int lib_MAC_C_SH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SH(b) * ZL(c)); }
static inline unsigned int lib_MAC_C_SH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, SH(b) * ZH(c)); }
static inline unsigned int lib_MAC_C_ZL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZL(b) * SL(c)); }
static inline unsigned int lib_MAC_C_ZL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZL(b) * SH(c)); }
static inline unsigned int lib_MAC_C_ZH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZH(b) * SL(c)); }
static inline unsigned int lib_MAC_C_ZH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return add(s, a, ZH(b) * SH(c)); }

static inline unsigned int lib_MACC_SL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , SL(b) * SL(c)); }
static inline unsigned int lib_MACC_SH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , SH(b) * SL(c)); }
static inline unsigned int lib_MACC_SH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , SH(b) * SH(c)); }
static inline unsigned int lib_MACC_ZL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , ZL(b) * ZL(c)); }
static inline unsigned int lib_MACC_ZH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , ZH(b) * ZL(c)); }
static inline unsigned int lib_MACC_ZH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , ZH(b) * ZH(c)); }
static inline unsigned int lib_MACC_SH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return addWithCarry(s, a , SH(b) * ZL(c)); }
#endif

static inline unsigned int lib_MSU_SL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SL(b) * SL(c); }
static inline unsigned int lib_MSU_SL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SL(b) * SH(c); }
static inline unsigned int lib_MSU_SH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SH(b) * SL(c); }
static inline unsigned int lib_MSU_SH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SH(b) * SH(c); }
static inline unsigned int lib_MSU_ZL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZL(b) * ZL(c); }
static inline unsigned int lib_MSU_ZL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZL(b) * ZH(c); }
static inline unsigned int lib_MSU_ZH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZH(b) * ZL(c); }
static inline unsigned int lib_MSU_ZH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZH(b) * ZH(c); }

static inline unsigned int lib_MSU_SL_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SL(b) * ZL(c); }
static inline unsigned int lib_MSU_SL_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SL(b) * ZH(c); }
static inline unsigned int lib_MSU_SH_ZL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SH(b) * ZL(c); }
static inline unsigned int lib_MSU_SH_ZH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - SH(b) * ZH(c); }
static inline unsigned int lib_MSU_ZL_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZL(b) * SL(c); }
static inline unsigned int lib_MSU_ZL_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZL(b) * SH(c); }
static inline unsigned int lib_MSU_ZH_SL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZH(b) * SL(c); }
static inline unsigned int lib_MSU_ZH_SH(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) { return a - ZH(b) * SH(c); }

static inline unsigned int lib_MUL_SL_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SL(b) * SL(c); }
static inline unsigned int lib_MUL_SL_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SL(b) * SH(c); }
static inline unsigned int lib_MUL_SH_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SH(b) * SL(c); }
static inline unsigned int lib_MUL_SH_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SH(b) * SH(c); }
static inline unsigned int lib_MUL_ZL_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZL(b) * ZL(c); }
static inline unsigned int lib_MUL_ZL_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZL(b) * ZH(c); }
static inline unsigned int lib_MUL_ZH_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZH(b) * ZL(c); }
static inline unsigned int lib_MUL_ZH_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZH(b) * ZH(c); }

static inline unsigned int lib_MUL_SL_SL_NR(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift) { return ((int32_t)(SL(b) * SL(c))) >> shift; }
static inline unsigned int lib_MUL_SH_SH_NR(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift) { return ((int32_t)(SH(b) * SH(c))) >> shift; }
static inline unsigned int lib_MUL_ZL_ZL_NR(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift) { return ((uint32_t)(ZL(b) * ZL(c))) >> shift; }
static inline unsigned int lib_MUL_ZH_ZH_NR(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift) { return ((uint32_t)(ZH(b) * ZH(c))) >> shift; }

static inline unsigned int lib_MUL_SL_SL_NR_R(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift)
{
  int32_t result = (int32_t)(SL(b) * SL(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MUL_SH_SH_NR_R(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift)
{
  int32_t result = (int32_t)(SH(b) * SH(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MUL_ZL_ZL_NR_R(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift)
{
  uint32_t result = (uint32_t)(ZL(b) * ZL(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}
static inline unsigned int lib_MUL_ZH_ZH_NR_R(iss_cpu_state_t *s, unsigned int b, unsigned int c, unsigned int shift)
{
  uint32_t result = (uint32_t)(ZH(b) * ZH(c));
  if (shift > 0) result = (result + (1<<(shift-1))) >> shift;
  return result;
}

static inline unsigned int lib_MUL_SL_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SL(b) * ZL(c); }
static inline unsigned int lib_MUL_SL_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SL(b) * ZH(c); }
static inline unsigned int lib_MUL_SH_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SH(b) * ZL(c); }
static inline unsigned int lib_MUL_SH_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return SH(b) * ZH(c); }
static inline unsigned int lib_MUL_ZL_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZL(b) * SL(c); }
static inline unsigned int lib_MUL_ZL_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZL(b) * SH(c); }
static inline unsigned int lib_MUL_ZH_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZH(b) * SL(c); }
static inline unsigned int lib_MUL_ZH_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return ZH(b) * SH(c); }

static inline unsigned int lib_MMUL_SL_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SL(b) * SL(c); }
static inline unsigned int lib_MMUL_SL_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SL(b) * SH(c); }
static inline unsigned int lib_MMUL_SH_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SH(b) * SL(c); }
static inline unsigned int lib_MMUL_SH_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SH(b) * SH(c); }
static inline unsigned int lib_MMUL_ZL_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZL(b) * ZL(c); }
static inline unsigned int lib_MMUL_ZL_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZL(b) * ZH(c); }
static inline unsigned int lib_MMUL_ZH_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZH(b) * ZL(c); }
static inline unsigned int lib_MMUL_ZH_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZH(b) * ZH(c); }

static inline unsigned int lib_MMUL_SL_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SL(b) * ZL(c); }
static inline unsigned int lib_MMUL_SL_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SL(b) * ZH(c); }
static inline unsigned int lib_MMUL_SH_ZL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SH(b) * ZL(c); }
static inline unsigned int lib_MMUL_SH_ZH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - SH(b) * ZH(c); }
static inline unsigned int lib_MMUL_ZL_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZL(b) * SL(c); }
static inline unsigned int lib_MMUL_ZL_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZL(b) * SH(c); }
static inline unsigned int lib_MMUL_ZH_SL(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZH(b) * SL(c); }
static inline unsigned int lib_MMUL_ZH_SH(iss_cpu_state_t *s, unsigned int b, unsigned int c) { return - ZH(b) * SH(c); }



static inline unsigned int lib_MULS(iss_cpu_state_t *s, int a, int b) { return a * b; }
static inline unsigned int lib_MULU(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a * b; }
static inline unsigned int lib_DIVS(iss_cpu_state_t *s, int a, int b) { if (b == 0) return 0; else return a / b; }
static inline unsigned int lib_DIVU(iss_cpu_state_t *s, unsigned int a, unsigned int b) { if (b == 0) return 0; else return a / b; }
static inline unsigned int lib_MINU(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a < b ? a : b; }
static inline int lib_MINS(iss_cpu_state_t *s, int a, int b) { return a < b ? a : b; }
static inline unsigned int lib_MAXU(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return a > b ? a : b; }
static inline int lib_MAXS(iss_cpu_state_t *s, int a, int b) { return a > b ? a : b; }
static inline int lib_ABS(iss_cpu_state_t *s, int a) { return a >= 0 ? a : -a; }
static inline unsigned int lib_AVGU(iss_cpu_state_t *s, unsigned int a, unsigned int b) { return (a + b) >> 1; }
static inline int lib_AVGS(iss_cpu_state_t *s, int a, int b) { return (a + b) >> 1; }







/*
 * CONDITIONAL OPERATIONS
 */

static inline unsigned int lib_CMOV(iss_cpu_state_t *s, unsigned int flag, unsigned int a, unsigned int b) { return flag ? a : b; }







/*
 * BIT MANIPULATION OPERATIONS
 */

static inline unsigned int lib_FF1_or1k(iss_cpu_state_t *s, unsigned int a) { return ffs(a); }

static inline unsigned int lib_FL1_or1k(iss_cpu_state_t *s, unsigned int t) {
  /* Reverse the word and use ffs */
  t = (((t & 0xaaaaaaaa) >> 1) | ((t & 0x55555555) << 1));
  t = (((t & 0xcccccccc) >> 2) | ((t & 0x33333333) << 2));
  t = (((t & 0xf0f0f0f0) >> 4) | ((t & 0x0f0f0f0f) << 4));
  t = (((t & 0xff00ff00) >> 8) | ((t & 0x00ff00ff) << 8));
  t = ffs ((t >> 16) | (t << 16));
  
  return  (0 == t ? t : 33 - t);
}

static inline unsigned int lib_FF1(iss_cpu_state_t *s, unsigned int a) { 
  unsigned int result = ffs(a);
  if (result == 0) return 32;
  else return result - 1;
}

static inline unsigned int lib_FL1(iss_cpu_state_t *s, unsigned int t) {
  /* Reverse the word and use ffs */
  t = (((t & 0xaaaaaaaa) >> 1) | ((t & 0x55555555) << 1));
  t = (((t & 0xcccccccc) >> 2) | ((t & 0x33333333) << 2));
  t = (((t & 0xf0f0f0f0) >> 4) | ((t & 0x0f0f0f0f) << 4));
  t = (((t & 0xff00ff00) >> 8) | ((t & 0x00ff00ff) << 8));
  t = ffs ((t >> 16) | (t << 16));
  
  return  (0 == t ? 32 : 32 - t);
}

static inline unsigned int lib_CNT(iss_cpu_state_t *s, unsigned int t) {
#if 1
  return __builtin_popcount(t);
#else  
  unsigned int v = cpu->regs[pc->inReg[0]];
  v = v - ((v >> 1) & 0x55555555);
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);
  cpu->regs[pc->outReg[0]] = ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24;
#endif
}

static inline unsigned int lib_CLB(iss_cpu_state_t *s, unsigned int t)
{
  if (t == 0) return 0;
  else return __builtin_clrsb(t);
}


static inline unsigned int lib_BSET(iss_cpu_state_t *s, unsigned int val, unsigned int mask)
{
  return val | mask;
}

static inline unsigned int lib_BCLR(iss_cpu_state_t *s, unsigned int val, unsigned int mask)
{
  return val & ~mask;
}

static inline int lib_BEXTRACT(iss_cpu_state_t *s, unsigned int val, unsigned int bits, unsigned int shift)
{
  if (shift + bits > 32) bits = 32 - shift;
  return getSignedField(val, shift, bits);
}

static inline unsigned int lib_BEXTRACTU(iss_cpu_state_t *s, unsigned int val, unsigned int mask, unsigned int shift)
{
  return (val & mask) >> shift;
}

static inline unsigned int lib_BINSERT(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int mask, unsigned int shift)
{
  return (b & ~mask) | ((a << shift) & mask);
}



/*
 *  VECTORS
 */

#define VEC_OP(operName, type, elemType, elemSize, num_elem, oper)                \
static inline type lib_VEC_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, type b) {  \
  elemType *tmp_a = (elemType*)&a;                                                \
  elemType *tmp_b = (elemType*)&b;                                                \
  type out;                                                                       \
  elemType *tmp_out = (elemType*)&out;                                            \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    tmp_out[i] = tmp_a[i] oper tmp_b[i];                                          \
  return out;                                                                     \
}                                                                                 \
                                                                                  \
static inline type lib_VEC_##operName##_SC_##elemType##_to_##type(iss_cpu_state_t *s, type a, elemType b) { \
  elemType *tmp_a = (elemType*)&a;                                                      \
  type out;                                                                             \
  elemType *tmp_out = (elemType*)&out;                                                  \
  int i;                                                                                \
  for (i = 0; i < num_elem; i++)                                                        \
    tmp_out[i] = tmp_a[i] oper b;                                                       \
  return out;                                                                           \
}

#define VEC_OP_DIV2(operName, type, elemType, elemSize, num_elem, oper)                \
static inline type lib_VEC_##operName##_##elemType##_to_##type##_div2(iss_cpu_state_t *s, type a, type b) {  \
  elemType *tmp_a = (elemType*)&a;                                                \
  elemType *tmp_b = (elemType*)&b;                                                \
  type out;                                                                       \
  elemType *tmp_out = (elemType*)&out;                                            \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    tmp_out[i] = ((elemType)(tmp_a[i] oper tmp_b[i]))>>1;                         \
  return out;                                                                     \
}

#define VEC_OP_DIV4(operName, type, elemType, elemSize, num_elem, oper)                \
static inline type lib_VEC_##operName##_##elemType##_to_##type##_div4(iss_cpu_state_t *s, type a, type b) {  \
  elemType *tmp_a = (elemType*)&a;                                                \
  elemType *tmp_b = (elemType*)&b;                                                \
  type out;                                                                       \
  elemType *tmp_out = (elemType*)&out;                                            \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    tmp_out[i] = ((elemType)(tmp_a[i] oper tmp_b[i]))>>2;                         \
  return out;                                                                     \
}

#define VEC_EXPR(operName, type, elemType, elemSize, num_elem, expr)                \
static inline type lib_VEC_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, type b) {  \
  elemType *tmp_a = (elemType*)&a;                                                \
  elemType *tmp_b = (elemType*)&b;                                                \
  type out;                                                                       \
  elemType *tmp_out = (elemType*)&out;                                            \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    tmp_out[i] = expr;                                                            \
  return out;                                                                     \
}

#define VEC_EXPR_SC(operName, type, elemType, elemSize, num_elem, expr)                \
static inline type lib_VEC_##operName##_SC_##elemType##_to_##type(iss_cpu_state_t *s, type a, elemType b) { \
  elemType *tmp_a = (elemType*)&a;                                                      \
  type out;                                                                             \
  elemType *tmp_out = (elemType*)&out;                                                  \
  int i;                                                                                \
  for (i = 0; i < num_elem; i++)                                                        \
    tmp_out[i] = expr;                                                                  \
  return out;                                                                           \
}

#define VEC_CMP(operName, type, elemType, elemSize, num_elem, oper)                   \
static inline type lib_VEC_CMP##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, type b) {  \
  elemType *tmp_a = (elemType*)&a;                                                    \
  elemType *tmp_b = (elemType*)&b;                                                    \
  type out;                                                                           \
  elemType *tmp_out = (elemType*)&out;                                                \
  int i;                                                                              \
  for (i = 0; i < num_elem; i++)                                                      \
    if (tmp_a[i] oper tmp_b[i])                                                       \
      tmp_out[i] = -1;                                                                \
    else                                                                              \
      tmp_out[i] = 0;                                                                 \
  return out;                                                                         \
}                                                                                     \
                                                                                      \
static inline type lib_VEC_CMP##operName##_SC_##elemType##_to_##type(iss_cpu_state_t *s, type a, elemType b) { \
  elemType *tmp_a = (elemType*)&a;                                                          \
  type out;                                                                                 \
  elemType *tmp_out = (elemType*)&out;                                                      \
  int i;                                                                                    \
  for (i = 0; i < num_elem; i++)                                                            \
    if (tmp_a[i] oper b)                                                                    \
      tmp_out[i] = -1;                                                                      \
    else                                                                                    \
      tmp_out[i] = 0;                                                                       \
  return out;                                                                               \
}

#define VEC_ALL(operName, type, elemType, elemSize, num_elem, oper)                                 \
static inline type lib_VEC_ALL_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, type b, int *flagPtr) {  \
  elemType *tmp_a = (elemType*)&a;                                                                  \
  elemType *tmp_b = (elemType*)&b;                                                                  \
  type out;                                                                                         \
  elemType *tmp_out = (elemType*)&out;                                                              \
  int isOper = 1;                                                                                   \
  int i;                                                                                            \
  for (i = 0; i < num_elem; i++)                                                                    \
    if (tmp_a[i] oper tmp_b[i])                                                                     \
      tmp_out[i] = -1;                                                                              \
    else {                                                                                          \
      tmp_out[i] = 0;                                                                               \
      isOper = 0;                                                                                   \
    }                                                                                               \
  *flagPtr = isOper;                                                                                \
  return out;                                                                                       \
}                                                                                                   \
                                                                                                    \
static inline type lib_VEC_ALL_SC_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, elemType b, int *flagPtr) { \
  elemType *tmp_a = (elemType*)&a;                                                                        \
  type out;                                                                                               \
  elemType *tmp_out = (elemType*)&out;                                                                    \
  int isOper = 1;                                                                                         \
  int i;                                                                                                  \
  for (i = 0; i < num_elem; i++)                                                                          \
    if (tmp_a[i] oper b)                                                                                  \
      tmp_out[i] = -1;                                                                                    \
    else {                                                                                                \
      tmp_out[i] = 0;                                                                                     \
      isOper = 0;                                                                                         \
    }                                                                                                     \
  *flagPtr = isOper;                                                                                      \
  return out;                                                                                             \
}

#define VEC_ANY(operName, type, elemType, elemSize, num_elem, oper)                                 \
static inline type lib_VEC_ANY_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, type b, int *flagPtr) {  \
  elemType *tmp_a = (elemType*)&a;                                                                  \
  elemType *tmp_b = (elemType*)&b;                                                                  \
  type out;                                                                                         \
  elemType *tmp_out = (elemType*)&out;                                                              \
  int isOper = 0;                                                                                   \
  int i;                                                                                            \
  for (i = 0; i < num_elem; i++)                                                                    \
    if (tmp_a[i] oper tmp_b[i]) {                                                                   \
      tmp_out[i] = -1;                                                                              \
      isOper = 1;                                                                                   \
    } else {                                                                                        \
      tmp_out[i] = 0;                                                                               \
    }                                                                                               \
  *flagPtr = isOper;                                                                                \
  return out;                                                                                       \
}                                                                                                   \
                                                                                                    \
static inline type lib_VEC_ANY_SC_##operName##_##elemType##_to_##type(iss_cpu_state_t *s, type a, elemType b, int *flagPtr) { \
  elemType *tmp_a = (elemType*)&a;                                                                  \
  type out;                                                                                         \
  elemType *tmp_out = (elemType*)&out;                                                              \
  int isOper = 0;                                                                                   \
  int i;                                                                                            \
  for (i = 0; i < num_elem; i++)                                                                    \
    if (tmp_a[i] oper b) {                                                                          \
      tmp_out[i] = -1;                                                                              \
      isOper = 1;                                                                                   \
    } else {                                                                                        \
      tmp_out[i] = 0;                                                                               \
    }                                                                                               \
  *flagPtr = isOper;                                                                                \
  return out;                                                                                       \
}

VEC_ALL(EQ, int32_t, int8_t, 1, 4, ==)
VEC_ALL(EQ, int32_t, int16_t, 2, 2, ==)

VEC_ALL(GE, int32_t, int8_t, 1, 4, >=)
VEC_ALL(GE, int32_t, int16_t, 2, 2, >=)

VEC_ALL(GEU, uint32_t, uint8_t, 1, 4, >=)
VEC_ALL(GEU, uint32_t, uint16_t, 2, 2, >=)

VEC_ALL(GT, int32_t, int8_t, 1, 4, >)
VEC_ALL(GT, int32_t, int16_t, 2, 2, >)

VEC_ALL(GTU, uint32_t, uint8_t, 1, 4, >)
VEC_ALL(GTU, uint32_t, uint16_t, 2, 2, >)

VEC_ALL(LE, int32_t, int8_t, 1, 4, <=)
VEC_ALL(LE, int32_t, int16_t, 2, 2, <=)

VEC_ALL(LEU, uint32_t, uint8_t, 1, 4, <=)
VEC_ALL(LEU, uint32_t, uint16_t, 2, 2, <=)

VEC_ALL(LT, int32_t, int8_t, 1, 4, <)
VEC_ALL(LT, int32_t, int16_t, 2, 2, <)

VEC_ALL(LTU, uint32_t, uint8_t, 1, 4, <)
VEC_ALL(LTU, uint32_t, uint16_t, 2, 2, <)

VEC_ALL(NE, int32_t, int8_t, 1, 4, !=)
VEC_ALL(NE, int32_t, int16_t, 2, 2, !=)




VEC_ANY(EQ, int32_t, int8_t, 1, 4, ==)
VEC_ANY(EQ, int32_t, int16_t, 2, 2, ==)

VEC_ANY(GE, int32_t, int8_t, 1, 4, >=)
VEC_ANY(GE, int32_t, int16_t, 2, 2, >=)

VEC_ANY(GEU, uint32_t, uint8_t, 1, 4, >=)
VEC_ANY(GEU, uint32_t, uint16_t, 2, 2, >=)

VEC_ANY(GT, int32_t, int8_t, 1, 4, >)
VEC_ANY(GT, int32_t, int16_t, 2, 2, >)

VEC_ANY(GTU, uint32_t, uint8_t, 1, 4, >)
VEC_ANY(GTU, uint32_t, uint16_t, 2, 2, >)

VEC_ANY(LE, int32_t, int8_t, 1, 4, <=)
VEC_ANY(LE, int32_t, int16_t, 2, 2, <=)

VEC_ANY(LEU, uint32_t, uint8_t, 1, 4, <=)
VEC_ANY(LEU, uint32_t, uint16_t, 2, 2, <=)

VEC_ANY(LT, int32_t, int8_t, 1, 4, <)
VEC_ANY(LT, int32_t, int16_t, 2, 2, <)

VEC_ANY(LTU, uint32_t, uint8_t, 1, 4, <)
VEC_ANY(LTU, uint32_t, uint16_t, 2, 2, <)

VEC_ANY(NE, int32_t, int8_t, 1, 4, !=)
VEC_ANY(NE, int32_t, int16_t, 2, 2, !=)




VEC_CMP(EQ, int32_t, int8_t, 1, 4, ==)
VEC_CMP(EQ, int32_t, int16_t, 2, 2, ==)

VEC_CMP(GE, int32_t, int8_t, 1, 4, >=)
VEC_CMP(GE, int32_t, int16_t, 2, 2, >=)

VEC_CMP(GEU, uint32_t, uint8_t, 1, 4, >=)
VEC_CMP(GEU, uint32_t, uint16_t, 2, 2, >=)

VEC_CMP(GT, int32_t, int8_t, 1, 4, >)
VEC_CMP(GT, int32_t, int16_t, 2, 2, >)

VEC_CMP(GTU, uint32_t, uint8_t, 1, 4, >)
VEC_CMP(GTU, uint32_t, uint16_t, 2, 2, >)

VEC_CMP(LE, int32_t, int8_t, 1, 4, <=)
VEC_CMP(LE, int32_t, int16_t, 2, 2, <=)

VEC_CMP(LEU, uint32_t, uint8_t, 1, 4, <=)
VEC_CMP(LEU, uint32_t, uint16_t, 2, 2, <=)

VEC_CMP(LT, int32_t, int8_t, 1, 4, <)
VEC_CMP(LT, int32_t, int16_t, 2, 2, <)

VEC_CMP(LTU, uint32_t, uint8_t, 1, 4, <)
VEC_CMP(LTU, uint32_t, uint16_t, 2, 2, <)

VEC_CMP(NE, int32_t, int8_t, 1, 4, !=)
VEC_CMP(NE, int32_t, int16_t, 2, 2, !=)





VEC_OP(ADD, int32_t, int8_t, 1, 4, +)
VEC_OP_DIV2(ADD, int32_t, int8_t, 1, 4, +)
VEC_OP_DIV4(ADD, int32_t, int8_t, 1, 4, +)
VEC_OP(ADD, int32_t, int16_t, 2, 2, +)
VEC_OP_DIV2(ADD, int32_t, int16_t, 2, 2, +)
VEC_OP_DIV4(ADD, int32_t, int16_t, 2, 2, +)

VEC_OP(SUB, int32_t, int8_t, 1, 4, -)
VEC_OP_DIV2(SUB, int32_t, int8_t, 1, 4, -)
VEC_OP_DIV4(SUB, int32_t, int8_t, 1, 4, -)
VEC_OP(SUB, int32_t, int16_t, 2, 2, -)
VEC_OP_DIV2(SUB, int32_t, int16_t, 2, 2, -)
VEC_OP_DIV4(SUB, int32_t, int16_t, 2, 2, -)

VEC_EXPR(AVG, int32_t, int8_t, 1, 4, ((int8_t)(tmp_a[i] + tmp_b[i])>>(int8_t)1))
VEC_EXPR(AVG, int32_t, int16_t, 2, 2, ((int16_t)(tmp_a[i] + tmp_b[i])>>(int16_t)1))
VEC_EXPR_SC(AVG, int32_t, int8_t, 1, 4, ((int8_t)(tmp_a[i] + b)>>(int8_t)1))
VEC_EXPR_SC(AVG, int32_t, int16_t, 2, 2, ((int16_t)(tmp_a[i] + b)>>(int16_t)1))

VEC_EXPR(AVGU, uint32_t, uint8_t, 1, 4, ((uint8_t)(tmp_a[i] + tmp_b[i])>>(uint8_t)1))
VEC_EXPR(AVGU, uint32_t, uint16_t, 2, 2, ((uint16_t)(tmp_a[i] + tmp_b[i])>>(uint16_t)1))
VEC_EXPR_SC(AVGU, uint32_t, uint8_t, 1, 4, ((uint8_t)(tmp_a[i] + b)>>(uint8_t)1))
VEC_EXPR_SC(AVGU, uint32_t, uint16_t, 2, 2, ((uint16_t)(tmp_a[i] + b)>>(uint16_t)1))

VEC_EXPR(MIN, int32_t, int8_t, 1, 4, (tmp_a[i]>tmp_b[i] ? tmp_b[i] : tmp_a[i]))
VEC_EXPR(MIN, int32_t, int16_t, 2, 2, (tmp_a[i]>tmp_b[i] ? tmp_b[i] : tmp_a[i]))
VEC_EXPR_SC(MIN, int32_t, int8_t, 1, 4, (tmp_a[i]>b ? b : tmp_a[i]))
VEC_EXPR_SC(MIN, int32_t, int16_t, 2, 2, (tmp_a[i]>b ? b : tmp_a[i]))

VEC_EXPR(MINU, uint32_t, uint8_t, 1, 4, (tmp_a[i]>tmp_b[i] ? tmp_b[i] : tmp_a[i]))
VEC_EXPR(MINU, uint32_t, uint16_t, 2, 2, (tmp_a[i]>tmp_b[i] ? tmp_b[i] : tmp_a[i]))
VEC_EXPR_SC(MINU, uint32_t, uint8_t, 1, 4, (tmp_a[i]>b ? b : tmp_a[i]))
VEC_EXPR_SC(MINU, uint32_t, uint16_t, 2, 2, (tmp_a[i]>b ? b : tmp_a[i]))

VEC_EXPR(MAX, int32_t, int8_t, 1, 4, (tmp_a[i]>tmp_b[i] ? tmp_a[i] : tmp_b[i]))
VEC_EXPR(MAX, int32_t, int16_t, 2, 2, (tmp_a[i]>tmp_b[i] ? tmp_a[i] : tmp_b[i]))
VEC_EXPR_SC(MAX, int32_t, int8_t, 1, 4, (tmp_a[i]>b ? tmp_a[i] : b))
VEC_EXPR_SC(MAX, int32_t, int16_t, 2, 2, (tmp_a[i]>b ? tmp_a[i] : b))

VEC_EXPR(MAXU, uint32_t, uint8_t, 1, 4, (tmp_a[i]>tmp_b[i] ? tmp_a[i] : tmp_b[i]))
VEC_EXPR(MAXU, uint32_t, uint16_t, 2, 2, (tmp_a[i]>tmp_b[i] ? tmp_a[i] : tmp_b[i]))
VEC_EXPR_SC(MAXU, uint32_t, uint8_t, 1, 4, (tmp_a[i]>b ? tmp_a[i] : b))
VEC_EXPR_SC(MAXU, uint32_t, uint16_t, 2, 2, (tmp_a[i]>b ? tmp_a[i] : b))

VEC_EXPR(SRL, uint32_t, uint8_t, 1, 4, (tmp_a[i] >> (tmp_b[i] & 0x7)))
VEC_EXPR_SC(SRL, uint32_t, uint8_t, 1, 4, (tmp_a[i] >> (b & 0x7)))
VEC_EXPR(SRL, uint32_t, uint16_t, 1, 2, (tmp_a[i] >> (tmp_b[i] & 0xF)))
VEC_EXPR_SC(SRL, uint32_t, uint16_t, 1, 2, (tmp_a[i] >> (b & 0xF)))

VEC_EXPR(SRA, int32_t, int8_t, 1, 4, (tmp_a[i] >> (tmp_b[i] & 0x7)))
VEC_EXPR_SC(SRA, int32_t, int8_t, 1, 4, (tmp_a[i] >> (b & 0x7)))
VEC_EXPR(SRA, int32_t, int16_t, 1, 2, (tmp_a[i] >> (tmp_b[i] & 0xF)))
VEC_EXPR_SC(SRA, int32_t, int16_t, 1, 2, (tmp_a[i] >> (b & 0xF)))

VEC_EXPR(SLL, uint32_t, uint8_t, 1, 4, (tmp_a[i] << (tmp_b[i] & 0x7)))
VEC_EXPR_SC(SLL, uint32_t, uint8_t, 1, 4, (tmp_a[i] << (b & 0x7)))
VEC_EXPR(SLL, uint32_t, uint16_t, 1, 2, (tmp_a[i] << (tmp_b[i] & 0xF)))
VEC_EXPR_SC(SLL, uint32_t, uint16_t, 1, 2, (tmp_a[i] << (b & 0xF)))

VEC_OP(MUL, int32_t, int8_t, 1, 4, *)
VEC_OP(MUL, int32_t, int16_t, 2, 2, *)

VEC_OP(OR, int32_t, int8_t, 1, 4, |)
VEC_OP(OR, int32_t, int16_t, 2, 2, |)

VEC_OP(XOR, int32_t, int8_t, 1, 4, ^)
VEC_OP(XOR, int32_t, int16_t, 2, 2, ^)

VEC_OP(AND, int32_t, int8_t, 1, 4, &)
VEC_OP(AND, int32_t, int16_t, 2, 2, &)

static inline uint32_t lib_VEC_INS_8(iss_cpu_state_t *s, uint32_t a, uint32_t b, uint32_t shift) {
  shift *= 8;
  uint32_t mask = 0xff << shift;
  uint32_t ins = (b & 0xff) << shift;
  a &= ~mask;
  a |= ins;
  return a;
}

static inline uint32_t lib_VEC_INS_16(iss_cpu_state_t *s, uint32_t a, uint32_t b, uint32_t shift) {
  shift *= 16;
  uint32_t mask = 0xffff << shift;
  uint32_t ins = (b & 0xffff) << shift;
  a &= ~mask;
  a |= ins;
  return a;
}

static inline uint32_t lib_VEC_EXT_8(iss_cpu_state_t *s, uint32_t a, uint32_t shift) {
  shift *= 8;
  uint32_t mask = 0xff << shift;
  a &= mask;
  a >>= shift;
  return (int8_t)a;
}

static inline uint32_t lib_VEC_EXT_16(iss_cpu_state_t *s, uint32_t a, uint32_t shift) {
  shift *= 16;
  uint32_t mask = 0xffff << shift;
  a &= mask;
  a >>= shift;
  return (int16_t)a;
}

static inline uint32_t lib_VEC_EXTU_8(iss_cpu_state_t *s, uint32_t a, uint32_t shift) {
  shift *= 8;
  uint32_t mask = 0xff << shift;
  a &= mask;
  a >>= shift;
  return a;
}

static inline uint32_t lib_VEC_EXTU_16(iss_cpu_state_t *s, uint32_t a, uint32_t shift) {
  shift *= 16;
  uint32_t mask = 0xffff << shift;
  a &= mask;
  a >>= shift;
  return a;
}

static inline int32_t lib_VEC_ABS_int8_t_to_int32_t(iss_cpu_state_t *s, uint32_t a) {
  int8_t *tmp_a = (int8_t*)&a;
  int32_t out;
  int8_t *tmp_out = (int8_t *)&out;
  int i;
  for (i = 0; i < 4; i++)
    tmp_out[i] = tmp_a[i] > 0 ? tmp_a[i] : -tmp_a[i];
  return out;
}

static inline int32_t lib_VEC_ABS_int16_t_to_int32_t(iss_cpu_state_t *s, uint32_t a) {
  int16_t *tmp_a = (int16_t*)&a;
  int32_t out;
  int16_t *tmp_out = (int16_t *)&out;
  int i;
  for (i = 0; i < 2; i++)
    tmp_out[i] = tmp_a[i] > 0 ? tmp_a[i] : -tmp_a[i];
  return out;
}

static inline unsigned int getShuffleHalf(unsigned int a, unsigned int b, unsigned int pos) {
  unsigned int shift = ((b>>pos)&1) << 4;
  return ((a >> shift) & 0xffff) << pos;
}

static inline unsigned int lib_VEC_SHUFFLE_16(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return getShuffleHalf(a, b, 16) | getShuffleHalf(a, b, 0);
  unsigned int low = b & 1 ? a >> 16 : a & 0xffff;
  unsigned int high = b & (1<<16) ? a >> 16 : a & 0xffff;
  return low | (high << 16);
}

static inline unsigned int getShuffleHalfSci(unsigned int a, unsigned int b, unsigned int pos) {
  unsigned int bitPos = pos >> 4;
  unsigned int shift = ((b>>bitPos)&1) << 4;
  return ((a >> shift) & 0xffff) << pos;
}

static inline unsigned int lib_VEC_SHUFFLE_SCI_16(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return getShuffleHalfSci(a, b, 16) | getShuffleHalfSci(a, b, 0);
}

static inline unsigned int getShuffleByte(unsigned int a, unsigned int b, unsigned int pos) {
  unsigned int shift = ((b>>pos)&0x3) << 3;
  return ((a >> shift) & 0xff) << pos;
}

static inline unsigned int lib_VEC_SHUFFLE_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return getShuffleByte(a, b, 24) | getShuffleByte(a, b, 16) | getShuffleByte(a, b, 8) | getShuffleByte(a, b, 0);
}

static inline unsigned int getShuffleByteSci(unsigned int a, unsigned int b, unsigned int pos) {
  unsigned int bitPos = pos >> 2;
  unsigned int shift = ((b>>bitPos)&0x3) << 3;
  return ((a >> shift) & 0xff) << pos;
}

static inline unsigned int lib_VEC_SHUFFLE_SCI_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return getShuffleByteSci(a, b, 24) | getShuffleByteSci(a, b, 16) | getShuffleByteSci(a, b, 8) | getShuffleByteSci(a, b, 0);
}

static inline unsigned int lib_VEC_SHUFFLEI0_SCI_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (((a >> 0) & 0xff) << 24) | getShuffleByteSci(a, b, 16) | getShuffleByteSci(a, b, 8) | getShuffleByteSci(a, b, 0);
}

static inline unsigned int lib_VEC_SHUFFLEI1_SCI_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (((a >> 8) & 0xff) << 24) | getShuffleByteSci(a, b, 16) | getShuffleByteSci(a, b, 8) | getShuffleByteSci(a, b, 0);
}

static inline unsigned int lib_VEC_SHUFFLEI2_SCI_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (((a >> 16) & 0xff) << 24) | getShuffleByteSci(a, b, 16) | getShuffleByteSci(a, b, 8) | getShuffleByteSci(a, b, 0);
}

static inline unsigned int lib_VEC_SHUFFLEI3_SCI_8(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (((a >> 24) & 0xff) << 24) | getShuffleByteSci(a, b, 16) | getShuffleByteSci(a, b, 8) | getShuffleByteSci(a, b, 0);
}

static inline unsigned int lib_VEC_SHUFFLE2_16(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) {
#ifdef RISCV
  return getShuffleHalf(b&(1<<17)?a:c, b, 16) | getShuffleHalf(b&(1<<1)?a:c, b, 0);
#else
  return getShuffleHalf(b&(1<<17)?c:a, b, 16) | getShuffleHalf(b&(1<<1)?c:a, b, 0);
#endif
}

static inline unsigned int lib_VEC_SHUFFLE2_8(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) {
#ifdef RISCV
  return getShuffleByte(b&(1<<26)?a:c, b, 24) | getShuffleByte(b&(1<<18)?a:c, b, 16) | getShuffleByte(b&(1<<10)?a:c, b, 8) | getShuffleByte(b&(1<<2)?a:c, b, 0);
#else
  return getShuffleByte(b&(1<<26)?c:a, b, 24) | getShuffleByte(b&(1<<18)?c:a, b, 16) | getShuffleByte(b&(1<<10)?c:a, b, 8) | getShuffleByte(b&(1<<2)?c:a, b, 0);
#endif
}

static inline unsigned int lib_VEC_PACK_SC_16(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return ((a & 0xffff) << 16) | (b & 0xffff);
}

static inline unsigned int lib_VEC_PACKHI_SC_8(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned c) {
  return ((a & 0xff) << 24) | ((b & 0xff) << 16) | (c & 0xffff);
}

static inline unsigned int lib_VEC_PACKLO_SC_8(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned c) {
  return ((a & 0xff) << 8) | ((b & 0xff) << 0) | (c & 0xffff0000);
}


#define VEC_DOTP(operName, typeOut, typeA, typeB, elemTypeA, elemTypeB, elemSize, num_elem, oper)                \
static inline typeOut lib_VEC_##operName##_##elemSize(iss_cpu_state_t *s, typeA a, typeB b) {  \
  elemTypeA *tmp_a = (elemTypeA*)&a;                                                \
  elemTypeB *tmp_b = (elemTypeB*)&b;                                                \
  typeOut out = 0;                                                                       \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    out += tmp_a[i] oper tmp_b[i];                                          \
  return out;                                                                     \
}                                                                                 \
                                                                                  \
static inline typeOut lib_VEC_##operName##_SC_##elemSize(iss_cpu_state_t *s, typeA a, typeB b) { \
  elemTypeA *tmp_a = (elemTypeA*)&a;                                                      \
  elemTypeB *tmp_b = (elemTypeB*)&b;                                                \
  typeOut out = 0;                                                                             \
  int i;                                                                                \
  for (i = 0; i < num_elem; i++)                                                        \
    out += tmp_a[i] oper tmp_b[0];                                                       \
  return out;                                                                           \
}

VEC_DOTP(DOTSP, int32_t, int32_t, int32_t, int16_t, int16_t, 16, 2, *)
VEC_DOTP(DOTSP, int32_t, int32_t, int32_t, int8_t, int8_t, 8, 4, *)

VEC_DOTP(DOTUP, uint32_t, uint32_t, uint32_t, uint16_t, uint16_t, 16, 2, *)
VEC_DOTP(DOTUP, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, 8, 4, *)

VEC_DOTP(DOTUSP, int32_t, uint32_t, int32_t, uint16_t, int16_t, 16, 2, *)
VEC_DOTP(DOTUSP, int32_t, uint32_t, int32_t, uint8_t, int8_t, 8, 4, *)



#define VEC_SDOT(operName, typeOut, typeA, typeB, elemTypeA, elemTypeB, elemSize, num_elem, oper)                \
static inline typeOut lib_VEC_##operName##_##elemSize(iss_cpu_state_t *s, typeOut out, typeA a, typeB b) {  \
  elemTypeA *tmp_a = (elemTypeA*)&a;                                                \
  elemTypeB *tmp_b = (elemTypeB*)&b;                                                \
  int i;                                                                          \
  for (i = 0; i < num_elem; i++)                                                  \
    out += tmp_a[i] oper tmp_b[i];                                          \
  return out;                                                                     \
}                                                                                 \
                                                                                  \
static inline typeOut lib_VEC_##operName##_SC_##elemSize(iss_cpu_state_t *s, typeOut out, typeA a, typeB b) { \
  elemTypeA *tmp_a = (elemTypeA*)&a;                                                      \
  elemTypeB *tmp_b = (elemTypeB*)&b;                                                \
  int i;                                                                                \
  for (i = 0; i < num_elem; i++)                                                        \
    out += tmp_a[i] oper tmp_b[0];                                                       \
  return out;                                                                           \
}

VEC_SDOT(SDOTSP, int32_t, int32_t, int32_t, int16_t, int16_t, 16, 2, *)
VEC_SDOT(SDOTSP, int32_t, int32_t, int32_t, int8_t, int8_t, 8, 4, *)

VEC_SDOT(SDOTUP, uint32_t, uint32_t, uint32_t, uint16_t, uint16_t, 16, 2, *)
VEC_SDOT(SDOTUP, uint32_t, uint32_t, uint32_t, uint8_t, uint8_t, 8, 4, *)

VEC_SDOT(SDOTUSP, int32_t, uint32_t, int32_t, uint16_t, int16_t, 16, 2, *)
VEC_SDOT(SDOTUSP, int32_t, uint32_t, int32_t, uint8_t, int8_t, 8, 4, *)


/*
 *  HW LOOPS
 */

#if 0
static inline pc_t *handleHwLoopStub(cpu_t *cpu, pc_t *pc, int index)
{
  // This stub is here to check at the end of a HW loop if we must jump to
  // the start of the loop
  cpu->hwLoopCounter[index]--;
  if (cpu->hwLoopCounter[index] == 0) {
    return NULL;
  } else {
    return cpu->hwLoopStartPc[index];
  }  
}

static inline pc_t *lib_hwLoopStub0(cpu_t *cpu, pc_t *pc)
{
  return handleHwLoopStub(cpu, pc, 0);
}

static inline pc_t *lib_hwLoopStub1(cpu_t *cpu, pc_t *pc)
{
  return handleHwLoopStub(cpu, pc, 1);
}

static inline void lib_hwLoopSetCount(cpu_t *cpu, pc_t *pc, unsigned int index, unsigned int count)
{
  cpu->hwLoopCounter[index] = count;
}

static inline pc_t *lib_hwLoopSetEnd(cpu_t *cpu, pc_t *pc, unsigned int index, unsigned int addr)
{
  // A PC stub is inserted at the end of the HW loop in order to check HW loop count
  // and jump to beginning of loop or continue
  // No need to clean-up pending stubs as they are shared between all the cores
  if (cpu->hwLoopEndPc[index] != NULL)
  {
    destroyPcStub(cpu, cpu->hwLoopEndPc[index]);
    cpu->hwLoopEndPc[index] = NULL;
  }

  if (index == 0) cpu->hwLoopEndPc[index] = createPcCpuStub(cpu, getPc(cpu, addr), lib_hwLoopStub0, CPU_HANDLER_HWLOOP0);
  else cpu->hwLoopEndPc[index] = createPcCpuStub(cpu, getPc(cpu, addr), lib_hwLoopStub1, CPU_HANDLER_HWLOOP1);

  return cpu->hwLoopEndPc[index];
}

static inline void lib_hwLoopSetStart(cpu_t *cpu, pc_t *pc, unsigned int index, unsigned int addr)
{
  cpu->hwLoopStartPc[index] = getPc(cpu, addr);
}
#endif




// Add/Sub with normalization and rounding

static inline unsigned int lib_ADD_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) { 
  shift &= 0x1f;
  return ((int32_t)(a + b)) >> shift;
}

static inline unsigned int lib_ADD_NRU(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  return ((uint32_t)(a + b)) >> shift;
}

static inline unsigned int lib_ADD_NL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  return (a + b) << shift;
}

static inline unsigned int lib_ADD_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  if (shift > 0) return ((int32_t)(a + b + (1<<(shift-1)))) >> shift;
  else return (int32_t)(a + b);
}

static inline unsigned int lib_ADD_NR_RU(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  if (shift > 0) return ((uint32_t)(a + b + (1<<(shift-1)))) >> shift;
  else return (uint32_t)(a + b);
}

static inline unsigned int lib_SUB_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  return ((int32_t)(a - b)) >> shift;
}

static inline unsigned int lib_SUB_NRU(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  return ((uint32_t)(a - b)) >> shift;
}

static inline unsigned int lib_SUB_NL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  return (a - b) << shift;
}

static inline unsigned int lib_SUB_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  if (shift > 0) return ((int32_t)(a - b + (1<<(shift-1)))) >> shift;
  else return (int32_t)(a - b);
}

static inline unsigned int lib_SUB_NR_RU(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) {
  shift &= 0x1f;
  if (shift > 0) return ((uint32_t)(a - b + (1<<(shift-1)))) >> shift;
  else return (uint32_t)(a - b);
}

// Combined normalization and rounding
static inline int lib_MULS_NL(iss_cpu_state_t *s, int a, int b, unsigned int shift) { 
  return (a * b) << shift;
}

static inline unsigned int lib_MULU_NL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) { 
  return (a * b) << shift;
}

static inline unsigned int lib_MULUS_NL(iss_cpu_state_t *s, unsigned int a, int b, unsigned int shift) { 
  return (a * b) << shift;
}

static inline int lib_MULS_NR(iss_cpu_state_t *s, int a, int b, unsigned int shift) { 
  return ((int32_t)(a * b)) >> shift;
}

static inline unsigned int lib_MULU_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) { 
  return ((uint32_t)(a * b)) >> shift;
}

static inline unsigned int lib_MULUS_NR(iss_cpu_state_t *s, unsigned int a, int b, unsigned int shift) { 
  return ((int32_t)(a * b)) >> shift;
}

static inline int lib_MULS_NR_R(iss_cpu_state_t *s, int a, int b, unsigned int shift) { 
  if (shift > 0) return ((int32_t)(a * b + (1<<(shift-1)))) >> shift;
  else return (int32_t)(a * b);
}

static inline unsigned int lib_MULU_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int shift) { 
  if (shift > 0) return ((uint32_t)(a * b + (1<<(shift-1)))) >> shift;
  else return (uint32_t)(a * b);
}

static inline int lib_MULUS_NR_R(iss_cpu_state_t *s, unsigned int a, int b, unsigned int shift) {
  unsigned int roundValue = shift == 0 ? 0 : 1<<(shift-1);
  return ((int32_t)(a * b + roundValue)) >> shift;
}

static inline int lib_MACS_NL(iss_cpu_state_t *s, int a, int b, int c, unsigned int shift) { 
  return (a + (b * c)) << shift;
}

static inline unsigned int lib_MACU_NL(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { 
  return (a + (b * c)) << shift;
}

static inline unsigned int lib_MACUS_NL(iss_cpu_state_t *s, int a, unsigned int b, int c, unsigned int shift) { 
  return (a + (b * c)) << shift;
}

static inline int lib_MACS_NR(iss_cpu_state_t *s, int a, int b, int c, unsigned int shift) { 
  return ((int32_t)(a + b * c)) >> shift;
}

static inline unsigned int lib_MACU_NR(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { 
  return ((uint32_t)(a + b * c)) >> shift;
}

static inline unsigned int lib_MACUS_NR(iss_cpu_state_t *s, int a, unsigned int b, int c, unsigned int shift) { 
  return ((int32_t)(a + b * c)) >> shift;
}

static inline int lib_MACS_NR_R(iss_cpu_state_t *s, int a, int b, int c, unsigned int shift) { 
  if (shift > 0) return ((int32_t)(a + b * c + (1<<(shift-1)))) >> shift;
  else return (int32_t)(a + b * c);
}

static inline unsigned int lib_MACU_NR_R(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int shift) { 
  if (shift > 0) return ((uint32_t)(a + b * c + (1<<(shift-1)))) >> shift;
  else return (uint32_t)(a + b * c);
}

static inline unsigned int lib_MACUS_NR_R(iss_cpu_state_t *s, int a, unsigned int b, int c, unsigned int shift) { 
  if (shift > 0) return ((int32_t)(a + b * c + (1<<(shift-1)))) >> shift;
  else return (int32_t)(a + b * c);
}

//Clipping
static inline unsigned int lib_CLIP(iss_cpu_state_t *s, int a, int low, int high) {
  unsigned int result;
  if (a > high) result = high;
  else if (a < low) result = low;
  else result = a;
  return result;
}

// Complex numbers
static inline unsigned int lib_CPLXMULS(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long a_imm = (long long)(int16_t)(a >> 16), a_re = (long long)(int16_t)(a & 0xffff);
  long long b_imm = (long long)(int16_t)(b >> 16), b_re = (long long)(int16_t)(b & 0xffff);
// printf("a = [Re= %5d, Im= %5d], b = [Re= %5d, Im= %5d]\n", (int) a_re, (int) a_imm, (int) b_re, (int) b_imm);
  return (((uint16_t)((a_imm*b_re + a_re*b_imm) >> 15))<<16) | (uint16_t)((a_re*b_re - a_imm*b_imm) >> 15);
}

static inline unsigned int lib_CPLXMULS_DIV2(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long a_imm = (long long)(int16_t)(a >> 16), a_re = (long long)(int16_t)(a & 0xffff);
  long long b_imm = (long long)(int16_t)(b >> 16), b_re = (long long)(int16_t)(b & 0xffff);
  return (((uint16_t)((a_imm*b_re + a_re*b_imm) >> 16))<<16) | (uint16_t)((a_re*b_re - a_imm*b_imm) >> 16);
}

static inline unsigned int lib_CPLXMULS_DIV4(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long a_imm = (long long)(int16_t)(a >> 16), a_re = (long long)(int16_t)(a & 0xffff);
  long long b_imm = (long long)(int16_t)(b >> 16), b_re = (long long)(int16_t)(b & 0xffff);
  return (((uint16_t)((a_imm*b_re + a_re*b_imm) >> 17))<<16) | (uint16_t)((a_re*b_re - a_imm*b_imm) >> 17);
}

static inline unsigned int lib_CPLXMULS_SC(iss_cpu_state_t *s, unsigned int a, int b) {
  int a_imm = (int)(int16_t)(a >> 16), a_re = (int)(int16_t)(a & 0xffff);
  return (((uint16_t)((a_imm*b + a_re*b) >> 15))<<16) | ((a_re*b - a_imm*b) >> 15);
}

static inline unsigned int lib_CPLX_CONJ_16(iss_cpu_state_t *s, unsigned int a) {
  int a_imm = (int)(int16_t)(a >> 16), a_re = (int)(int16_t)(a & 0xffff);
  return (((uint16_t)(-a_imm))<<16) | ((uint16_t)a_re);
}

static inline unsigned int lib_VEC_ADD_16_ROTMJ(iss_cpu_state_t *s, unsigned int a, int b) {
  int16_t a_imm = (int16_t)(a >> 16), a_re = (int16_t)(a & 0xffff),
          b_imm = (int16_t)(b >> 16), b_re = (int16_t)(b & 0xffff);
  return (((int16_t) (b_re - a_re))<<16) | ((a_imm - b_imm) & 0x0ffff);
}

static inline unsigned int lib_VEC_ADD_16_ROTMJ_DIV2(iss_cpu_state_t *s, unsigned int a, int b) {
  int16_t a_imm = (int16_t)(a >> 16), a_re = (int16_t)(a & 0xffff),
          b_imm = (int16_t)(b >> 16), b_re = (int16_t)(b & 0xffff);
  return (((int16_t) (b_re - a_re)>>1)<<16) | (((int16_t) (a_imm - b_imm)>>1) & 0x0ffff);
}

static inline unsigned int lib_VEC_ADD_16_ROTMJ_DIV4(iss_cpu_state_t *s, unsigned int a, int b) {
  int16_t a_imm = (int16_t)(a >> 16), a_re = (int16_t)(a & 0xffff),
          b_imm = (int16_t)(b >> 16), b_re = (int16_t)(b & 0xffff);
  return (((int16_t) (b_re - a_re)>>2)<<16) | (((int16_t) (a_imm - b_imm)>>2) & 0x0ffff);
}



// Viterbi extensions

#if 0
static inline unsigned int lib_VITOP_MAX(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  int ah = (short)(a >> 16), al = (short)(a & 0xffff);
  int bh = (short)(b >> 16), bl = (short)(b & 0xffff);
  s->vf0 = ah <= bh;
  s->vf1 = al <= bl;
  return (MAX(ah, bh) << 16) | (MAX(al, bl) & 0xFFFF);
}

static inline unsigned int lib_VITOP_SEL(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  int ah = (short)(a >> 16), al = (short)(a & 0xffff);
  int bh = (short)(b >> 16), bl = (short)(b & 0xffff);
  unsigned int res = 0;
  if (s->vf0) res |= bh << 17;
  else res |= ah << 17;
  res |= s->vf0 << 16;
  if (s->vf1) res |= (unsigned short)(bl << 1);
  else res |= (unsigned short)(al << 1);
  res |= s->vf1;
  return res;
}

static inline unsigned int lib_VEC_PACK_SC_H_16(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (b >> 16) | (a & 0xffff0000);
}

static inline unsigned int lib_VEC_PACK_SC_HL_16(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return (b & 0xffff) | (a << 16);
}
#endif


#include "softfloat.h"

static inline unsigned int lib_float_add_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_add(a, b);
  return result;
}

static inline unsigned int lib_float_sub_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
    float_exception_flags = 0;
  unsigned int result = float32_sub(a, b);
  return result;
}

static inline unsigned int lib_float_mul_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_mul(a, b);
  return result;
}

static inline unsigned int lib_float_div_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_div(a, b);
  return result;
}

static inline unsigned int lib_float_itof_s(iss_cpu_state_t *s, unsigned int a) {
  float_exception_flags = 0;
  unsigned int result = int32_to_float32(a);
  return result;
}

static inline unsigned int lib_float_ftoi_s(iss_cpu_state_t *s, unsigned int a) {
  float_exception_flags = 0;
  unsigned int result = float32_to_int32(a);
  return result;
}

static inline unsigned int lib_float_rem_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_rem(a, b);
  return result;
}

/*
static inline unsigned int lib_float_madd_s(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c) {
  unsigned int result = float32_add(a, float32_mul(b, c));
  return result;
}*/

static inline unsigned int setRoundingMode(unsigned int mode)
{
  unsigned int old = float_rounding_mode;
  switch (mode) {
    case 0: float_rounding_mode = float_round_nearest_even; break;
    case 1: float_rounding_mode = float_round_to_zero; break;
    case 2: float_rounding_mode = float_round_down; break;
    case 3: float_rounding_mode = float_round_up; break;
    case 4: printf("Unimplemented roudning mode nearest ties to max magnitude"); exit(-1); break;
  }
  return old;
}

static inline void restoreRoundingMode(unsigned int mode)
{
  float_rounding_mode = mode;
}

static inline unsigned int lib_float_madd_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result;
  result = float32_mul(a, b);
  result = float32_add(result, c);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_msub_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result;
  result = float32_mul(a, b);
  result = float32_sub(result, c);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_nmadd_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result;
  result = float32_mul(a, b);
  result = float32_add(result, c);
  result = float32_sub(0, result);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_nmsub_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int c, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result;
  result = float32_mul(a, b);
  result = float32_sub(result, c);
  result = float32_sub(0, result);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_add_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result = float32_add(a, b);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_sub_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result = float32_sub(a, b);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_mul_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result = float32_mul(a, b);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_div_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int b, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result = float32_div(a, b);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_sqrt_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  unsigned int result = float32_sqrt(a);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_sgnj_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_sgnj(a, b);
}

static inline unsigned int lib_float_sgnjn_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_sgnjn(a, b);
}

static inline unsigned int lib_float_sgnjx_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_sgnjx(a, b);
}

static inline unsigned int lib_float_min_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_min(a, b);
  return result;
}

static inline unsigned int lib_float_max_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  float_exception_flags = 0;
  unsigned int result = float32_max(a, b);
  return result;
}

static inline int lib_float_cvt_w_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  int result = float32_to_int32(a);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_cvt_wu_s_round(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  long long result = float32_to_int64(a);
  restoreRoundingMode(old);
  return result;
}

static inline int lib_float_cvt_s_w_round(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  int result = int32_to_float32(a);
  restoreRoundingMode(old);
  return result;
}

static inline unsigned int lib_float_cvt_s_wu_round(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  float_exception_flags = 0;
  unsigned int old = setRoundingMode(round);
  int result = int64_to_float32(a);
  restoreRoundingMode(old);
  return result;
}

static inline int lib_float_cvt_d_s(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  return a;
}

static inline int lib_float_cvt_s_d(iss_cpu_state_t *s, unsigned int a, unsigned int round) {
  return a;
}

static inline unsigned int lib_float_fmv_x_s(iss_cpu_state_t *s, unsigned int a) {
  return a;
}

static inline unsigned int lib_float_fmv_s_x(iss_cpu_state_t *s, unsigned int a) {
  return a;
}

static inline unsigned int lib_float_eq_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_eq(a, b);
}

static inline unsigned int lib_float_lt_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_lt(a, b);
}

static inline unsigned int lib_float_le_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  return float32_le(a, b);
}

static inline unsigned int lib_float_class_s(iss_cpu_state_t *s, unsigned int a) {
  unsigned int frac = a & 0x007FFFFF;
  unsigned int exp = ( a>>23 ) & 0xFF;
  int sign = a>>31;

  if (exp == 0xff) {
    if (frac == 0) {
      if (sign) return 0; // - infinity
      else return 7; // + infinity
    } else {
      return 9; // quiet NaN
    }
  } else if (exp == 0 && frac == 0) {
    if (sign) return 3; // - 0
    else return 4; // + 0
  } else if (exp == 0 && frac != 0) {
    if (sign) return 2; // negative subnormal
    else return 5; // positive subnormal
  } else {
    if (sign) return 1; // negative number
    else return 6; // positive number
  }
}


#if 0
// Taken from spike
uint_fast16_t f32_classify( float32_t a )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;

    uA.f = a;
    uiA = uA.ui;

    uint_fast16_t infOrNaN = expF32UI( uiA ) == 0xFF;
    uint_fast16_t subnormalOrZero = expF32UI( uiA ) == 0;
    bool sign = signF32UI( uiA );

    return
        (  sign && infOrNaN && fracF32UI( uiA ) == 0 )          << 0 |
        (  sign && !infOrNaN && !subnormalOrZero )              << 1 |
        (  sign && subnormalOrZero && fracF32UI( uiA ) )        << 2 |
        (  sign && subnormalOrZero && fracF32UI( uiA ) == 0 )   << 3 |
        ( !sign && infOrNaN && fracF32UI( uiA ) == 0 )          << 7 |
        ( !sign && !infOrNaN && !subnormalOrZero )              << 6 |
        ( !sign && subnormalOrZero && fracF32UI( uiA ) )        << 5 |
        ( !sign && subnormalOrZero && fracF32UI( uiA ) == 0 )   << 4 |
        ( isNaNF32UI( uiA ) &&  softfloat_isSigNaNF32UI( uiA )) << 8 |
        ( isNaNF32UI( uiA ) && !softfloat_isSigNaNF32UI( uiA )) << 9;
}



#include "lnu/lnu.h"

static inline unsigned int lib_log_add_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuAdd(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_sub_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuSub(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_mul_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuMul(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_div_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuDiv(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_itof_s(iss_cpu_state_t *s, unsigned int a) {
  printf("%s %d\n", __FILE__, __LINE__);
  return 0;
}

static inline unsigned int lib_log_ftoi_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuF2I(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_powi_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  printf("%s %d\n", __FILE__, __LINE__);
  return 0;
}

static inline unsigned int lib_log_powi_inv_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  printf("%s %d\n", __FILE__, __LINE__);
  return 0;
}

static inline unsigned int lib_log_sqrt_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  // The spec defines sqrt with a second immediate operand but it seems to not be supported by the HW
  // The test is always using 1
  lnuSqrt(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_pow_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  // The spec defines pow with a second immediate operand but it seems to not be supported by the HW
  // The test is always using 1. This results in a multiplication, pow does not exist in lnu.h
  lnuMul(a, a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_exp_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuExp(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_log_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuLog(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_sin_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuSin(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_cos_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuCos(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_atan_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuAtn(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_ata_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuAta(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_atl_s(iss_cpu_state_t *s, unsigned int a) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuAtl(a, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_sca_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuSca(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_fma_s(iss_cpu_state_t *s, unsigned int c, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuFma(a, b, c, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_fda_s(iss_cpu_state_t *s, unsigned int c, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuFda(a, b, c, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_fms_s(iss_cpu_state_t *s, unsigned int c, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuFms(a, b, c, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_fds_s(iss_cpu_state_t *s, unsigned int c, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuFds(a, b, c, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_mex_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuMex(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_dex_s(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long res;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuDex(a, b, res, absErrLog, absErrArith, relErrLog, relErrArith);
  return res;
}

static inline unsigned int lib_log_halfToFloat(unsigned int a)
{
  // Sign
  unsigned int res = (((a >> 15) & 1) << 31);
  // The exponent is sign-extended from 5bits to 8bits
  if ((a >> 14) & 1) res |= 0x7 << 28;
  res |= (a & 0x3fff) << 13;
  return res;
}

static inline unsigned int lib_log_floatToHalf(unsigned int a)
{
  // TODO we should check for overflow or underflow
  // if the exponent sign is not equal bit 29:27 we have an overflow or underflow
  // lns_overflow  = ((~operand_a_i[30]) && (|(operand_a_i[29:27] & 3'b111))) ? 1'b1 : 1'b0;
  // lns_underflow = (( operand_a_i[30]) && ~(&(operand_a_i[29:27] | 3'b000))) ? 1'b1 : 1'b0;
  // in this case the output will be INF or ZERO
  return (((a >> 31) & 1) << 15) | (( a >> 13) & 0x7fff);
}

static inline unsigned int lib_log_add_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuAdd(lib_log_halfToFloat(ZL(a)), lib_log_halfToFloat(ZL(b)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuAdd(lib_log_halfToFloat(ZH(a)), lib_log_halfToFloat(ZH(b)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}

static inline unsigned int lib_log_sub_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuSub(lib_log_halfToFloat(ZL(a)), lib_log_halfToFloat(ZL(b)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuSub(lib_log_halfToFloat(ZH(a)), lib_log_halfToFloat(ZH(b)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}

static inline unsigned int lib_log_mul_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuMul(lib_log_halfToFloat(ZL(a)), lib_log_halfToFloat(ZL(b)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuMul(lib_log_halfToFloat(ZH(a)), lib_log_halfToFloat(ZH(b)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}

static inline unsigned int lib_log_div_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuDiv(lib_log_halfToFloat(ZL(a)), lib_log_halfToFloat(ZL(b)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuDiv(lib_log_halfToFloat(ZH(a)), lib_log_halfToFloat(ZH(b)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}

static inline unsigned int lib_log_htof_lo(iss_cpu_state_t *s, unsigned int a) {
  return lib_log_halfToFloat(ZL(a));
}

static inline unsigned int lib_log_htof_hi(iss_cpu_state_t *s, unsigned int a) {
  return lib_log_halfToFloat(ZH(a));
}

static inline unsigned int lib_log_sqrt_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuSqrt(lib_log_halfToFloat(ZL(a)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuSqrt(lib_log_halfToFloat(ZH(a)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}

static inline unsigned int lib_log_pow_vh(iss_cpu_state_t *s, unsigned int a, unsigned int b) {
  long long resL, resH;
  double absErrLog, absErrArith, relErrLog, relErrArith;
  lnuMul(lib_log_halfToFloat(ZL(a)), lib_log_halfToFloat(ZL(a)), resL, absErrLog, absErrArith, relErrLog, relErrArith);
  lnuMul(lib_log_halfToFloat(ZH(a)), lib_log_halfToFloat(ZH(a)), resH, absErrLog, absErrArith, relErrLog, relErrArith);
  return L_H_TO_W(lib_log_floatToHalf(resL), lib_log_floatToHalf(resH));
}



static inline pc_t *lf_lnu_sfeq_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) == (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}

static inline pc_t *lf_lnu_sfne_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) != (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}

static inline pc_t *lf_lnu_sfgt_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) > (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}

static inline pc_t *lf_lnu_sfge_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) >= (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}

static inline pc_t *lf_lnu_sflt_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) < (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}

static inline pc_t *lf_lnu_sfle_s_exec(cpu_t *cpu, pc_t *pc)
{
  cpu->flag = (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[0]]) <= (int32_t)lib_log_ftoi_s(&cpu->state, cpu->regs[pc->inReg[1]]);
  return pc->next;
}





static inline pc_t *lf_lnu_sfeq_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al == bl) || (ah == bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfne_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al != bl) || (ah != bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfgt_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al > bl) || (ah > bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfge_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al >= bl) || (ah >= bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sflt_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al < bl) || (ah < bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfle_vh_any_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al <= bl) || (ah <= bh);
  return pc->next;
}




static inline pc_t *lf_lnu_sfeq_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al == bl) && (ah == bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfne_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al != bl) && (ah != bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfgt_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al > bl) && (ah > bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfge_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al >= bl) && (ah >= bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sflt_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al < bl) && (ah < bh);
  return pc->next;
}

static inline pc_t *lf_lnu_sfle_vh_all_exec(cpu_t *cpu, pc_t *pc)
{
  unsigned int a = cpu->regs[pc->inReg[0]];
  unsigned int b = cpu->regs[pc->inReg[1]];
  double al = lnuLns2Double(lib_log_halfToFloat(ZL(a)));
  double ah = lnuLns2Double(lib_log_halfToFloat(ZH(a)));
  double bl = lnuLns2Double(lib_log_halfToFloat(ZL(b)));
  double bh = lnuLns2Double(lib_log_halfToFloat(ZH(b)));
  cpu->flag = (al <= bl) && (ah <= bh);
  return pc->next;
}

#endif

#endif
/*
 * Copyright (C) 2020 GreenWaves Technologies, SAS, ETH Zurich and
 *                    University of Bologna
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
 * Authors: Nazareno Bruschi, UniBo <nazareno.bruschi@unibo.it>
 *          Germain Haugou, GreenWaves Technologies (germain.haugou@greenwaves-technologies.com)
 */

#ifndef __CPU_ISS_COREV_HPP
#define __CPU_ISS_COREV_HPP


#define COREV_HWLOOP_LPSTART0 0
#define COREV_HWLOOP_LPEND0   1
#define COREV_HWLOOP_LPCOUNT0 2

#define COREV_HWLOOP_LPSTART1 3
#define COREV_HWLOOP_LPEND1   4
#define COREV_HWLOOP_LPCOUNT1 5

#define COREV_HWLOOP_LPSTART(x) (COREV_HWLOOP_LPSTART0 + (x)*3)
#define COREV_HWLOOP_LPEND(x) (COREV_HWLOOP_LPEND0 + (x)*3)
#define COREV_HWLOOP_LPCOUNT(x) (COREV_HWLOOP_LPCOUNT0 + (x)*3)


static inline iss_insn_t *corev_hwloop_check_exec(iss_t *iss, iss_insn_t *insn)
{
  iss_reg_t pc = insn->addr;

  // First execute the instructions as it is the last one of the loop body.
  // The real handler has been saved when the loop was started.
  iss_insn_t *insn_next = iss_exec_insn_handler(iss, insn, insn->hwloop_handler);

  // First check HW loop 0 as it has higher priority compared to HW loop 1
  if (iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT0] && iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPEND0] == pc)
  {
    iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT0]--;
    iss_decoder_msg(iss, "Reached end of HW loop (index: 0, loop count: %d)\n", iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT0]);

    // If counter is not zero, we must jump back to beginning of the loop.
    if (iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT0]) return iss->cpu.state.hwloop_start_insn[0];
  }

  // We get here either if HW loop 0 was not active or if the counter reached 0.
  // In both cases, HW loop 1 can jump back to the beginning of the loop.
  if (iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT1] && iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPEND1] == pc)
  {
    iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT1]--;
    // If counter is not zero, we must jump back to beginning of the loop.
    iss_decoder_msg(iss, "Reached end of HW loop (index: 1, loop count: %d)\n", iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT1]);
    if (iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT1]) return iss->cpu.state.hwloop_start_insn[1];
  }

  // In case no HW loop jumped back, just continue with the next instruction.
  return insn_next;
}


static inline void corev_hwloop_set_start(iss_t *iss, iss_insn_t *insn, int index, iss_reg_t start)
{
  iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPSTART(index)] = start;
  iss->cpu.state.hwloop_start_insn[index] = insn_cache_get(iss, start);
}


static inline void corev_hwloop_set_end(iss_t *iss, iss_insn_t *insn, int index, iss_reg_t end)
{
  iss_insn_t *end_insn = insn_cache_get_decoded(iss, end);

  if (end_insn->hwloop_handler == NULL)
  {
    end_insn->hwloop_handler = end_insn->handler;
    end_insn->handler = corev_hwloop_check_exec;
    end_insn->fast_handler = corev_hwloop_check_exec;
  }

  iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPEND(index)] = end;
}


static inline void corev_hwloop_set_count(iss_t *iss, iss_insn_t *insn, int index, iss_reg_t count)
{
  iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT(index)] = count;
}


static inline void corev_hwloop_set_all(iss_t *iss, iss_insn_t *insn, int index, iss_reg_t start, iss_reg_t end, iss_reg_t count)
{
  corev_hwloop_set_end(iss, insn, index, end);
  corev_hwloop_set_start(iss, insn, index, start);
  corev_hwloop_set_count(iss, insn, index, count);
}


static inline iss_insn_t *cv_avgu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_AVGU, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_slet_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, (int32_t)REG_GET(0) <= (int32_t)REG_GET(1));
  return insn->next;
}



static inline iss_insn_t *cv_sletu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, REG_GET(0) <= REG_GET(1));
  return insn->next;
}



static inline iss_insn_t *cv_min_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MINS, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_minu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MINU, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_max_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MAXS, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_maxu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MAXU, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_ror_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_ROR, REG_GET(0), REG_GET(1)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_ff1_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL1(lib_FF1, REG_GET(0)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_fl1_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL1(lib_FL1, REG_GET(0)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_clb_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL1(lib_CLB, REG_GET(0)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_cnt_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL1(lib_CNT, REG_GET(0)));
  //setRegDelayed(cpu, pc->outReg[0], value, 2);
  return insn->next;
}



static inline iss_insn_t *cv_exths_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, iss_get_signed_value(REG_GET(0), 16));
  return insn->next;
}



static inline iss_insn_t *cv_exthz_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, iss_get_field(REG_GET(0), 0, 16));
  return insn->next;
}



static inline iss_insn_t *cv_extbs_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, iss_get_signed_value(REG_GET(0), 8));
  return insn->next;
}



static inline iss_insn_t *cv_extbz_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, iss_get_field(REG_GET(0), 0, 8));
  return insn->next;
}


static inline iss_insn_t *cv_starti_exec(iss_t *iss, iss_insn_t *insn)
{
  corev_hwloop_set_start(iss, insn, UIM_GET(0), insn->addr + (UIM_GET(1) << 1));
  return insn->next;
}



static inline iss_insn_t *cv_endi_exec(iss_t *iss, iss_insn_t *insn)
{
  corev_hwloop_set_end(iss, insn, UIM_GET(0), insn->addr + (UIM_GET(1) << 1));
  return insn->next;
}



static inline iss_insn_t *cv_count_exec(iss_t *iss, iss_insn_t *insn)
{
  corev_hwloop_set_count(iss, insn, UIM_GET(0), REG_GET(0));
  return insn->next;
}



static inline iss_insn_t *cv_counti_exec(iss_t *iss, iss_insn_t *insn)
{
  corev_hwloop_set_count(iss, insn, UIM_GET(0), UIM_GET(1));
  return insn->next;
}



static inline iss_insn_t *cv_setup_exec(iss_t *iss, iss_insn_t *insn)
{
  int index = UIM_GET(0);
  iss_reg_t count = REG_GET(0);
  iss_reg_t start = insn->addr + insn->size;
  iss_reg_t end = insn->addr + (UIM_GET(1) << 1);

  corev_hwloop_set_all(iss, insn, index, start, end, count);

  return insn->next;
}


static inline iss_insn_t *cv_setupi_exec(iss_t *iss, iss_insn_t *insn)
{
  int index = UIM_GET(0);
  iss_reg_t count = UIM_GET(1);
  iss_reg_t start = insn->addr + insn->size;
  iss_reg_t end = insn->addr + (UIM_GET(2) << 1);

  corev_hwloop_set_all(iss, insn, index, start, end, count);

  return insn->next;
}


static inline iss_insn_t *cv_abs_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL1(lib_ABS, REG_GET(0)));
  return insn->next;
}


static inline iss_insn_t *cv_elw_exec(iss_t *iss, iss_insn_t *insn)
{
  uint32_t value = 0;
  iss->cpu.state.elw_insn = insn;
  iss_lsu_elw_perf(iss, insn, REG_GET(0) + SIM_GET(0), 4, REG_OUT(0));
  if (iss->cpu.state.insn_cycles != -1)
    iss->cpu.state.elw_insn = NULL;
  return insn->next;
}


#define CV_OP_RS_EXEC(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_int16_t_to_int32_t, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sc_h_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_int16_t_to_int32_t, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sci_h_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_int16_t_to_int32_t, REG_GET(0), SIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_int8_t_to_int32_t, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sc_b_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_int8_t_to_int32_t, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sci_b_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_int8_t_to_int32_t, REG_GET(0), SIM_GET(0))); \
  return insn->next;                                                                         \
}



#define CV_OP_RU_EXEC(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_uint16_t_to_uint32_t, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sc_h_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_uint16_t_to_uint32_t, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sci_h_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_uint16_t_to_uint32_t, REG_GET(0), UIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_uint8_t_to_uint32_t, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sc_b_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_uint8_t_to_uint32_t, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_sci_b_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_uint8_t_to_uint32_t, REG_GET(0), UIM_GET(0))); \
  return insn->next;                                                                         \
}


#define CV_OP_RS_EXEC2(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_16, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_16, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_16, REG_GET(0), SIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_8, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_8, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_8, REG_GET(0), SIM_GET(0))); \
  return insn->next;                                                                         \
}



#define CV_OP_RU_EXEC2(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_16, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_16, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_16, REG_GET(0), UIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_8, REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_8, REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL2(lib_VEC_##lib_name##_SC_8, REG_GET(0), UIM_GET(0))); \
  return insn->next;                                                                         \
}


#define CV_OP_RRS_EXEC2(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_16, REG_GET(2), REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_16, REG_GET(2), REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_16, REG_GET(0), REG_GET(1), SIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_8, REG_GET(2), REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_8, REG_GET(2), REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_8, REG_GET(0), REG_GET(1), SIM_GET(0))); \
  return insn->next;                                                                         \
}



#define CV_OP_RRU_EXEC2(insn_name,lib_name)                                           \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_16, REG_GET(2), REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_16, REG_GET(2), REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_h_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_16, REG_GET(0), REG_GET(1), UIM_GET(0))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)                \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_8, REG_GET(2), REG_GET(0), REG_GET(1)));    \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sc_exec(iss_t *iss, iss_insn_t *insn)             \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_8, REG_GET(2), REG_GET(0), REG_GET(1))); \
  return insn->next;                                                                         \
}                                                                                            \
                                                                                             \
static inline iss_insn_t *cv_##insn_name##_b_sci_exec(iss_t *iss, iss_insn_t *insn)            \
{                                                                                            \
  REG_SET(0, LIB_CALL3(lib_VEC_##lib_name##_SC_8, REG_GET(0), REG_GET(1), UIM_GET(0))); \
  return insn->next;                                                                         \
}




#define CV_OP1_RS_EXEC(insn_name,lib_name)                          \
static inline iss_insn_t *cv_##insn_name##_h_exec(iss_t *iss, iss_insn_t *insn)      \
{                                                                                  \
  REG_SET(0, LIB_CALL1(lib_VEC_##lib_name##_int16_t_to_int32_t, REG_GET(0)));    \
  return insn->next;                                                               \
}                                                                                  \
                                                                                   \
static inline iss_insn_t *cv_##insn_name##_b_exec(iss_t *iss, iss_insn_t *insn)      \
{                                                                                  \
  REG_SET(0, LIB_CALL1(lib_VEC_##lib_name##_int8_t_to_int32_t, REG_GET(0)));    \
  return insn->next;                                                               \
}



CV_OP_RS_EXEC(add,ADD)

CV_OP_RS_EXEC(sub,SUB)

CV_OP_RS_EXEC(avg,AVG)

CV_OP_RU_EXEC(avgu,AVGU)

CV_OP_RS_EXEC(min,MIN)

CV_OP_RU_EXEC(minu,MINU)

CV_OP_RS_EXEC(max,MAX)

CV_OP_RU_EXEC(maxu,MAXU)

CV_OP_RU_EXEC(srl,SRL)

CV_OP_RS_EXEC(sra,SRA)

CV_OP_RU_EXEC(sll,SLL)

CV_OP_RS_EXEC(or,OR)

CV_OP_RS_EXEC(xor,XOR)

CV_OP_RS_EXEC(and,AND)

CV_OP1_RS_EXEC(abs,ABS)


static inline iss_insn_t *cv_extractr_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_EXT_16, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_extractr_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_EXT_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_extractur_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_EXTU_16, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_extractur_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_EXTU_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_insertr_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_INS_16, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_insertr_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_INS_8, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}


CV_OP_RS_EXEC2(dotsp,DOTSP)

CV_OP_RU_EXEC2(dotup,DOTUP)

CV_OP_RS_EXEC2(dotusp,DOTUSP)

CV_OP_RRS_EXEC2(sdotsp,SDOTSP)

CV_OP_RRU_EXEC2(sdotup,SDOTUP)

CV_OP_RRS_EXEC2(sdotusp,SDOTUSP)



static inline iss_insn_t *cv_shuffle_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLE_16, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_shuffle_h_sci_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLE_SCI_16, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_shuffle_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLE_8, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_shufflei0_b_sci_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLEI0_SCI_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_shufflei1_b_sci_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLEI1_SCI_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_shufflei2_b_sci_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLEI2_SCI_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_shufflei3_b_sci_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_SHUFFLEI3_SCI_8, REG_GET(0), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_shuffle2_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_SHUFFLE2_16, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_shuffle2_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_SHUFFLE2_8, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}



static inline iss_insn_t *cv_pack_h_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_VEC_PACK_SC_16, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_packhi_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_PACKHI_SC_8, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_packlo_b_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_VEC_PACKLO_SC_8, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}



CV_OP_RS_EXEC(cmpeq,CMPEQ)

CV_OP_RS_EXEC(cmpne,CMPNE)

CV_OP_RS_EXEC(cmpgt,CMPGT)

CV_OP_RS_EXEC(cmpge,CMPGE)

CV_OP_RS_EXEC(cmplt,CMPLT)

CV_OP_RS_EXEC(cmple,CMPLE)

CV_OP_RU_EXEC(cmpgtu,CMPGTU)

CV_OP_RU_EXEC(cmpgeu,CMPGEU)

CV_OP_RU_EXEC(cmpltu,CMPLTU)

CV_OP_RU_EXEC(cmpleu,CMPLEU)



static inline iss_insn_t *cv_bneimm_exec_common(iss_t *iss, iss_insn_t *insn, int perf)
{
  if ((int32_t)REG_GET(0) != SIM_GET(1))
  {
    if (perf)
    {
      iss_pccr_account_event(iss, CSR_PCER_BRANCH, 1);
      iss_pccr_account_event(iss, CSR_PCER_TAKEN_BRANCH, 1);
    }
    iss_perf_account_taken_branch(iss);
    return insn->branch;
  }
  else
  {
    if (perf)
    {
      iss_pccr_account_event(iss, CSR_PCER_BRANCH, 1);
    }
    return insn->next;
  }
}

static inline iss_insn_t *cv_bneimm_exec_fast(iss_t *iss, iss_insn_t *insn)
{
  return cv_bneimm_exec_common(iss, insn, 0);
}

static inline iss_insn_t *cv_bneimm_exec(iss_t *iss, iss_insn_t *insn)
{
  return cv_bneimm_exec_common(iss, insn, 1);
}



static inline iss_insn_t *cv_beqimm_exec_common(iss_t *iss, iss_insn_t *insn, int perf)
{
  if ((int32_t)REG_GET(0) == SIM_GET(1))
  {
    if (perf)
    {
      iss_pccr_account_event(iss, CSR_PCER_BRANCH, 1);
      iss_pccr_account_event(iss, CSR_PCER_TAKEN_BRANCH, 1);
    }
    iss_perf_account_taken_branch(iss);
    return insn->branch;
  }
  else
  {
    if (perf)
    {
      iss_pccr_account_event(iss, CSR_PCER_BRANCH, 1);
    }
    return insn->next;
  }
}

static inline iss_insn_t *cv_beqimm_exec_fast(iss_t *iss, iss_insn_t *insn)
{
  return cv_beqimm_exec_common(iss, insn, 0);
}

static inline iss_insn_t *cv_beqimm_exec(iss_t *iss, iss_insn_t *insn)
{
  return cv_beqimm_exec_common(iss, insn, 1);
}




static inline iss_insn_t *cv_mac_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MAC, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}



static inline iss_insn_t *cv_msu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MSU, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}



static inline iss_insn_t *cv_mul_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MULS, REG_GET(0), REG_GET(1)));
  return insn->next;
}



static inline iss_insn_t *cv_muls_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MUL_SL_SL, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhs_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MUL_SH_SH, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_mulsN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_SL_SL_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhsN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_SH_SH_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_mulsRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_SL_SL_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhsRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_SH_SH_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_mulu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MUL_ZL_ZL, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL2(lib_MUL_ZH_ZH, REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_muluN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_ZL_ZL_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhuN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_ZH_ZH_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_muluRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_ZL_ZL_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_mulhhuRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MUL_ZH_ZH_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_macs_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MAC_SL_SL, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_machhs_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MAC_SH_SH, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_macsN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_SL_SL_NR, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_machhsN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_SH_SH_NR, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_macsRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_SL_SL_NR_R, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_machhsRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_SH_SH_NR_R, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_macu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MAC_ZL_ZL, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_machhu_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_MAC_ZH_ZH, REG_GET(2), REG_GET(0), REG_GET(1)));
  return insn->next;
}

static inline iss_insn_t *cv_macuN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_ZL_ZL_NR, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_machhuN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_ZH_ZH_NR, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_macuRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_ZL_ZL_NR_R, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_machhuRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL4(lib_MAC_ZH_ZH_NR_R, REG_GET(2), REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_addN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_adduN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NRU, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_addRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_adduRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR_RU, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_subN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_subuN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NRU, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_subRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR_R, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}

static inline iss_insn_t *cv_subuRN_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR_RU, REG_GET(0), REG_GET(1), UIM_GET(0)));
  return insn->next;
}



static inline iss_insn_t *cv_addNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_adduNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NRU, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_addRNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR_R, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_adduRNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_ADD_NR_RU, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}



static inline iss_insn_t *cv_subNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_subuNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NRU, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_subRNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR_R, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}

static inline iss_insn_t *cv_subuRNr_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_SUB_NR_RU, REG_GET(0), REG_GET(1), REG_GET(2)));
  return insn->next;
}



static inline iss_insn_t *cv_clip_exec(iss_t *iss, iss_insn_t *insn)
{
  int low = (int) -(1 << MAX((int)UIM_GET(0)-1, 0));
  int high = (1 << MAX((int)UIM_GET(0)-1, 0)) - 1 ;
  REG_SET(0, LIB_CALL3(lib_CLIP, REG_GET(0), low, high));
  return insn->next;
}

static inline iss_insn_t *cv_clipu_exec(iss_t *iss, iss_insn_t *insn)
{
  int low = 0;
  int high = (1 << MAX((int)UIM_GET(0)-1, 0)) - 1;
  REG_SET(0, LIB_CALL3(lib_CLIP, REG_GET(0), low, high));
  return insn->next;
}

static inline iss_insn_t *cv_clipr_exec(iss_t *iss, iss_insn_t *insn)
{
  int low = -REG_GET(1) - 1;
  int high = REG_GET(1);
  REG_SET(0, LIB_CALL3(lib_CLIP, REG_GET(0), low, high));
  return insn->next;
}

static inline iss_insn_t *cv_clipur_exec(iss_t *iss, iss_insn_t *insn)
{
  int low = 0;
  int high = REG_GET(1);
  REG_SET(0, LIB_CALL3(lib_CLIP, REG_GET(0), low, high));
  return insn->next;
}



static inline iss_insn_t *cv_bclr_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = UIM_GET(0) + 1;
  int shift = UIM_GET(1);
  REG_SET(0, LIB_CALL2(lib_BCLR, REG_GET(0), ((1ULL<<width)-1) << shift));

  return insn->next;
}



static inline iss_insn_t *cv_bclrr_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = ((REG_GET(1) >> 5) & 0x1f) + 1;
  int shift = REG_GET(1) & 0x1f;
  REG_SET(0, LIB_CALL2(lib_BCLR, REG_GET(0), ((1ULL<<width)-1) << shift));
  return insn->next;
}


static inline iss_insn_t *cv_extract_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = UIM_GET(0) + 1;
  int shift = UIM_GET(1);
  REG_SET(0, LIB_CALL3(lib_BEXTRACT, REG_GET(0), width, shift));
  return insn->next;
}



static inline iss_insn_t *cv_extractu_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = UIM_GET(0) + 1;
  int shift = UIM_GET(1);
  REG_SET(0, LIB_CALL3(lib_BEXTRACTU, REG_GET(0), ((1ULL << width) - 1) << shift, shift));
  return insn->next;
}



static inline iss_insn_t *cv_extractr_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = ((REG_GET(1) >> 5) & 0x1f) + 1;
  int shift = REG_GET(1) & 0x1f;
  REG_SET(0, LIB_CALL3(lib_BEXTRACT, REG_GET(0), width, shift));
  return insn->next;
}



static inline iss_insn_t *cv_extractur_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = ((REG_GET(1) >> 5) & 0x1f) + 1;
  int shift = REG_GET(1) & 0x1f;
  REG_SET(0, LIB_CALL3(lib_BEXTRACTU, REG_GET(0), ((1ULL << width) - 1) << shift, shift));
  return insn->next;
}



static inline iss_insn_t *cv_insert_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = UIM_GET(0) + 1;
  int shift = UIM_GET(1);
  REG_SET(0, LIB_CALL4(lib_BINSERT, REG_GET(0), REG_GET(1), ((1ULL << width) - 1) << shift, shift));
  return insn->next;
}



static inline iss_insn_t *cv_insertr_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = ((REG_GET(2) >> 5) & 0x1F) + 1;
  int shift = REG_GET(2) & 0x1F;
  REG_SET(0, LIB_CALL4(lib_BINSERT, REG_GET(0), REG_GET(1), ((1ULL << width) - 1) << shift, shift));
  return insn->next;
}



static inline iss_insn_t *cv_bset_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = UIM_GET(0) + 1;
  int shift = UIM_GET(1);
  REG_SET(0, LIB_CALL2(lib_BSET, REG_GET(0), ((1ULL << (width)) - 1) << shift));
  return insn->next;
}



static inline iss_insn_t *cv_bsetr_exec(iss_t *iss, iss_insn_t *insn)
{
  int width = ((REG_GET(1) >> 5) & 0x1f) + 1;
  int shift = REG_GET(1) & 0x1f;
  REG_SET(0, LIB_CALL2(lib_BSET, REG_GET(0), ((1ULL << (width)) - 1) << shift));
  return insn->next;
}


static inline iss_insn_t *cv_bitrev_exec(iss_t *iss, iss_insn_t *insn)
{
  REG_SET(0, LIB_CALL3(lib_BITREV, REG_GET(0), UIM_GET(0), UIM_GET(1)+1));
  return insn->next;
}


static inline void iss_isa_corev_init(iss_t *iss)
{
  iss->cpu.corev.hwloop = false;
}


static inline void iss_isa_corev_activate(iss_t *iss)
{
  iss->cpu.corev.hwloop = true;
  iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT(0)] = 0;
  iss->cpu.corev.hwloop_regs[COREV_HWLOOP_LPCOUNT(1)] = 0;

}


#endif

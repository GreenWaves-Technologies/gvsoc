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

#ifndef __VP_TRACE_TRACE_HPP__
#define __VP_TRACE_TRACE_HPP__

#include "vp/vp_data.hpp"
#include "vp/trace/vcd_dumper.hpp"
#include <stdarg.h>

namespace vp {

  #define BUFFER_SIZE (1<<16)

  class trace_engine;

  class trace
  {

    friend class component_trace;

  public:

    inline void msg(const char *fmt, ...);
    inline void user_msg(const char *fmt, ...);
    inline void warning(const char *fmt, ...);

    inline void event(uint8_t *value);
    inline void event_string(uint8_t *value, int size);

    void dump_header();
    void dump_warning_header();

    void set_active(bool active) { is_active = active; }
    void set_event_active(bool active) { is_event_active = active; }

  #ifndef VP_TRACE_ACTIVE
    inline bool get_active() { return false; }
    inline bool get_event_active() { return false; }
  #else
    inline bool get_active() { return is_active; }
    inline bool get_event_active() { return is_event_active; }
  #endif

    int width;
    int bytes;
    Vcd_trace *vcd_trace = NULL;

  protected:
    int level;
    component *comp;
    trace_engine *trace_manager;
    FILE *trace_file;
    bool is_active = false;
    bool is_event_active = false;
    string name;
    uint8_t *buffer = NULL;
    int buffer_index = 0;

  private:
    void get_event_buffer();

  };    

  inline void vp::trace::msg(const char *fmt, ...) 
  {
  #ifdef VP_TRACE_ACTIVE
    if (is_active)
    {
      dump_header();
      va_list ap;
      va_start(ap, fmt);
      if (vfprintf(stdout, fmt, ap) < 0) {}
      va_end(ap);  
    }
  #endif
  }


  void fatal(const char *fmt, ...) ;


};

#endif
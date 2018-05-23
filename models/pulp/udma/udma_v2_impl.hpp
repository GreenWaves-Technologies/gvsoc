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

#ifndef __PULP_UDMA_UDMA_V2_IMPL_HPP__
#define __PULP_UDMA_UDMA_V2_IMPL_HPP__

#include <vp/vp.hpp>
#include <vp/itf/io.hpp>
#include <vp/itf/qspim.hpp>
#include <vp/itf/uart.hpp>
#include <vp/itf/wire.hpp>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "archi/udma/udma_v2.h"

class udma;
class Udma_channel;


class Udma_transfer
{
public:
  uint32_t addr;
  uint16_t size;
  int transfer_size;
  int continuous_mode;
  uint32_t current_addr;
  int32_t remaining_size;
  uint64_t received_size;
  Udma_channel *channel;

  bool prepare_req(vp::io_req *req);
  void set_next(Udma_transfer *next) { this->next = next; }
  Udma_transfer *get_next() { return next; }
  Udma_transfer *next;
};



template<class T>
class Udma_queue
{
public:
  Udma_queue(int size) : size(size) { init(); }
  void init() { first=NULL; last=NULL; nb_cmd=0; }
  T *pop();
  inline void push(T *cmd);
  void push_from_latency(T *cmd);
  bool is_full() { return nb_cmd >= size; }
  bool is_empty() { return nb_cmd == 0; }
  T *get_first() { return first; }

private:
  T *first;    // First command of the queue, commands are popped from here
  T *last;     // Last command of the queue, commands are pushed here
  int        nb_cmd;
  int        size;
};


class Udma_channel
{
public:
  Udma_channel(udma *top, int id, std::string name);
  virtual vp::io_req_status_e req(vp::io_req *req, uint64_t offset);
  virtual void reset();
  virtual bool is_tx() { return false; }
  void set_next(Udma_channel *next) { this->next = next; }
  Udma_channel *get_next() { return next; }
  void event_handler();
  bool prepare_req(vp::io_req *req);
  void push_ready_req(vp::io_req *req);
  void handle_ready_req_end(vp::io_req *req);
  virtual bool is_busy() { return false; }

protected:
  vp::trace     trace;
  Udma_queue<vp::io_req> *ready_reqs;
  udma *top;

private:
  virtual vp::io_req_status_e saddr_req(vp::io_req *req);
  virtual vp::io_req_status_e size_req(vp::io_req *req);
  virtual vp::io_req_status_e cfg_req(vp::io_req *req); 
  void enqueue_transfer();
  void check_state();
  void handle_transfer_end();
  virtual void handle_ready_req(vp::io_req *req);
  virtual void handle_ready_reqs();

  uint32_t saddr;
  uint16_t size;
  
  int transfer_size;
  bool continuous_mode;

  int id;
  string name;
  Udma_channel *next;

  vp::clock_event *event;

  Udma_queue<Udma_transfer> *free_reqs;
  Udma_queue<Udma_transfer> *pending_reqs;

  Udma_transfer *current_cmd;
};



class Udma_rx_channel : public Udma_channel
{
public:
  Udma_rx_channel(udma *top, int id, string name) : Udma_channel(top, id, name) {}
  bool is_tx() { return false; }
};



class Udma_tx_channel : public Udma_channel
{
public:
  Udma_tx_channel(udma *top, int id, string name) : Udma_channel(top, id, name) {}
  bool is_tx() { return true; }

};



class Udma_periph 
{
public:
  Udma_periph(udma *top, int id);
  vp::io_req_status_e req(vp::io_req *req, uint64_t offset);
  virtual void reset();
  void clock_gate(bool is_on);

protected:
  Udma_channel *channel0 = NULL;
  Udma_channel *channel1 = NULL;

private:
  virtual vp::io_req_status_e custom_req(vp::io_req *req, uint64_t offset);
  bool is_on;
  udma *top;
  int id;
};




/*
 * SPIM
 */

class Spim_periph_v2;


class Spim_rx_channel : public Udma_rx_channel
{
public:
  Spim_rx_channel(udma *top, Spim_periph_v2 *periph, int id, string name) : Udma_rx_channel(top, id, name), periph(periph) {}

private:
  Spim_periph_v2 *periph;
};


class Spim_tx_channel : public Udma_tx_channel
{
public:
  Spim_tx_channel(udma *top, Spim_periph_v2 *periph, int id, string name) : Udma_tx_channel(top, id, name), periph(periph) {}
  void handle_ready_req(vp::io_req *req);

private:
  void reset();

  Spim_periph_v2 *periph;
  bool has_pending_word;
  uint32_t pending_word;
};


class Spim_periph_v2 : public Udma_periph
{
  friend class Spim_tx_channel;
  friend class Spim_rx_channel;

public:
  Spim_periph_v2(udma *top, int id, int itf_id);

protected:
  vp::qspim_master qspim_itf;
};




/*
 * UART
 */

class Uart_periph_v1;

class Uart_rx_channel : public Udma_rx_channel
{
public:
  Uart_rx_channel(udma *top, Uart_periph_v1 *periph, int id, string name) : Udma_rx_channel(top, id, name), periph(periph) {}
  bool is_busy();

private:
  Uart_periph_v1 *periph;
};


typedef enum
{
  UART_TX_STATE_START,
  UART_TX_STATE_DATA,
  UART_TX_STATE_PARITY,
  UART_TX_STATE_STOP
} uart_tx_state_e;

class Uart_tx_channel : public Udma_tx_channel
{
public:
  Uart_tx_channel(udma *top, Uart_periph_v1 *periph, int id, string name);
  void handle_ready_reqs();
  bool is_busy();

private:
  void reset();
  void check_state();
  static void handle_pending_word(void *__this, vp::clock_event *event);

  Uart_periph_v1 *periph;

  vp::clock_event *pending_word_event;

  uint32_t pending_word;
  int pending_bits;
  uart_tx_state_e state;
  vp::io_req *pending_req;
  int parity;
  int64_t next_bit_cycle;
  int stop_bits;
  int sent_bits;
};


class Uart_periph_v1 : public Udma_periph
{
  friend class Uart_tx_channel;
  friend class Uart_rx_channel;

public:
  Uart_periph_v1(udma *top, int id, int itf_id);
  vp::io_req_status_e custom_req(vp::io_req *req, uint64_t offset);
  void reset();

  int parity;
  int bit_length;
  int stop_bits;
  int tx;
  int rx;
  int clkdiv;
  int rx_pe;

protected:
  vp::uart_master uart_itf;

private:
  vp::io_req_status_e status_req(vp::io_req *req);
  vp::io_req_status_e setup_req(vp::io_req *req);
  void set_setup_reg(uint32_t value);

  uint32_t setup_reg_value;

  vp::trace     trace;

};




/*
 * UDMA
 */

template<class T>
inline void Udma_queue<T>::push(T *cmd)
{
  if (first)
    last->set_next(cmd);
  else
    first = cmd;
  cmd->set_next(NULL);
  last = cmd;
  nb_cmd++;
}



template<class T>
T *Udma_queue<T>::pop()
{
  if (!first) return NULL;
  T *cmd = first;
  first = cmd->get_next();
  nb_cmd--;

  return cmd;
}



class udma : public vp::component
{
  friend class Udma_periph;

public:

  udma(const char *config);

  void build();
  void start();

  void enqueue_ready(Udma_channel *channel);

  static void channel_handler(void *__this, vp::clock_event *event);
  void free_read_req(vp::io_req *req);

  void trigger_event(int event);

  vp::trace *get_trace() { return &this->trace; }

private:

  void reset();
  void check_state();

  vp::io_req_status_e conf_req(vp::io_req *req, uint64_t offset);
  vp::io_req_status_e periph_req(vp::io_req *req, uint64_t offset);
  static vp::io_req_status_e req(void *__this, vp::io_req *req);
  static void event_handler(void *__this, vp::clock_event *event);
  static void l2_grant(void *__this, vp::io_req *req);
  static void l2_response(void *__this, vp::io_req *req);

  vp::trace     trace;
  vp::io_slave in;
  int nb_periphs;
  int l2_read_fifo_size;
  std::vector<Udma_periph *>periphs;
  Udma_queue<Udma_channel> *ready_rx_channels;
  Udma_queue<Udma_channel> *ready_tx_channels;
  uint32_t clock_gating;
  vp::clock_event *event;
  Udma_queue<vp::io_req> *l2_read_reqs;
  Udma_queue<vp::io_req> *l2_read_waiting_reqs;
  
  vp::io_master l2_itf;
  vp::wire_master<int>    event_itf;
};


#endif
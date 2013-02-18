#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "mruby.h"
#include "mruby/irep.h"


static struct debug_state *debug_state = NULL;
static int mrb_count = 0;

struct debug_state {
  mrb_state *mrb;
  mrb_irep *irep;
  int lineno;
  void (*code_fetch_hook)(struct mrb_state* mrb, struct RProc *proc, mrb_value self, mrb_code *pc);
};

static
struct debug_state* get_debug_state(struct mrb_state* mrb) {
  int i;
  for(i = 0; i < mrb_count; ++i) {
    if(debug_state[i].mrb == mrb) {
      return &debug_state[i];
    }
  }
  return NULL;
}

static
void step_exec(struct mrb_state* mrb, mrb_irep *irep, mrb_code *pc, mrb_value *regs, const char* filename, int lineno) {
  if(filename) {
    printf("%s", filename);
  }
  else {
    printf("unknown");
  }

  if(lineno > 0) {
    printf(":%d\n", lineno);
  }
  else {
    printf("\n");
  }
  getchar();  
}

static 
void code_fetch_hook(struct mrb_state* mrb, mrb_irep *irep, mrb_code *pc, mrb_value *regs) {
  struct debug_state *debug_state = get_debug_state(mrb);
  int lineno = irep->lines ? irep->lines[(size_t)(pc-irep->iseq)] : -1;
  bool changed = false;

  if(irep->lines == NULL || irep->filename == NULL) {
    return;
  }

  if(debug_state->irep == irep) {
    if(debug_state->lineno != lineno) {
      changed = true;
    }
  }
  else {
    changed = true;
  }

  debug_state->irep = irep;
  debug_state->lineno = lineno;

  if(changed && irep->filename) {
    step_exec(mrb, irep, pc, regs, irep->filename, lineno);
  }
}

void mrb_mruby_debug_step_exec_gem_init(mrb_state* mrb) {
  debug_state = mrb_realloc(mrb, debug_state, sizeof(struct debug_state) * (mrb_count + 1));
  debug_state[mrb_count].mrb = mrb;
  debug_state[mrb_count].code_fetch_hook = mrb->code_fetch_hook;
  ++mrb_count;

  mrb->code_fetch_hook = code_fetch_hook;
}

void mrb_mruby_debug_step_exec_gem_final(mrb_state* mrb) {
  int i;
  for(i = 0; i < mrb_count; ++i) {
    if(debug_state[i].mrb == mrb) {
      mrb->code_fetch_hook = debug_state[i].code_fetch_hook;
      --mrb_count;
      memcpy(&debug_state[i], &debug_state[i + 1], (mrb_count - i));
      debug_state = mrb_realloc(mrb, debug_state, sizeof(struct debug_state) * mrb_count);
      return;
    }
  }
}

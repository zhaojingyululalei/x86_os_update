#include "irq/irq.h"
#include "cpu_instr.h"

void irq_disable_global(void) {
    cli();
}

void irq_enable_global(void) {
    sti();
}

irq_state_t irq_enter_protection (void) {
    irq_state_t state = read_eflags();
    irq_disable_global();
    return state;
}

void irq_leave_protection (irq_state_t state) {
    write_eflags(state);
}
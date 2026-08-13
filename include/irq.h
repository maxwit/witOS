#pragma once

#include <types.h>

#define IRQ_NONE       0
#define IRQ_HANDLED    1

#define IRQ_TYPE_NONE         0x00000000
#define IRQ_TYPE_RISING       0x00000001
#define IRQ_TYPE_FALLING      0x00000002
#define IRQ_TYPE_BOTH         (IRQ_TYPE_FALLING | IRQ_TYPE_RISING)
#define IRQ_TYPE_HIGH         0x00000004
#define IRQ_TYPE_LOW          0x00000008

struct int_pin;
struct irq_dev;

typedef	void (*IRQ_PIN_HANDLER)(struct int_pin *, __u32);
typedef int  (*IRQ_DEV_HANDLER)(__u32, void *);

struct int_ctrl {
	void (*ack)(__u32);
	void (*mask)(__u32);
	void (*mack)(__u32);
	void (*umask)(__u32);
	int	 (*set_trigger)(__u32, __u32);
};

struct int_pin {
	IRQ_PIN_HANDLER	 irq_handle;
	struct int_ctrl	 *intctrl;
	struct irq_dev	 *dev_list;
};

struct irq_dev {
	void             *device;
	IRQ_DEV_HANDLER  dev_isr;
	struct irq_dev   *next;
};

void irq_handle(__u32 irq);

void irq_handle_level(struct int_pin *ipin, __u32 irq);

void irq_handle_edge(struct int_pin *ipin, __u32 irq);

void irq_handle_simple(struct int_pin *ipin, __u32 irq);

void vectorirq_handle_level(struct int_pin *ipin, __u32 irq);

void irq_set_handler(__u32 irq, IRQ_PIN_HANDLER irq_handle, int chain_flag);

int irq_set_trigger(__u32 irq, __u32 type);

int irq_request(__u32 irq, IRQ_DEV_HANDLER dev_isr, void *dev);

int irq_assoc_intctl(__u32 irq, struct int_ctrl *intctrl);

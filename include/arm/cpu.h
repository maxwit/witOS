#pragma once

#define ARM_MODE_USR  0x10
#define ARM_MODE_FIQ  0x11
#define ARM_MODE_IRQ  0x12
#define ARM_MODE_SVC  0x13
#define ARM_MODE_ABT  0x17
#define ARM_MODE_UND  0x1b
#define ARM_MODE_SYS  0x1f

#define ARM_THUMB     (1 << 5)

#define ARM_FIQ_MASK  (1 << 6)
#define ARM_IRQ_MASK  (1 << 7)
#define ARM_INT_MASK  (ARM_IRQ_MASK | ARM_FIQ_MASK)

#define SVC_STACK_SIZE   0x00010000
#define UND_STACK_SIZE   SVC_STACK_SIZE
#define ABT_STACK_SIZE   SVC_STACK_SIZE
#define IRQ_STACK_SIZE   SVC_STACK_SIZE
#define FIQ_STACK_SIZE   SVC_STACK_SIZE

// FIXME
#define CONFIG_STACK_SIZE \
	(SVC_STACK_SIZE + UND_STACK_SIZE + ABT_STACK_SIZE + IRQ_STACK_SIZE + FIQ_STACK_SIZE)

#define CONFIG_HEAP_SIZE   ((CONFIG_START_MEM - SDRAM_BASE) / 4)

#ifndef __ASSEMBLY__
#ifdef CONFIG_IRQ_SUPPORT
#define irq_enable() \
	do { \
		__u32 cpsr; \
		asm volatile ("mrs %0, cpsr\n" \
			"bic %0, %0, %1\n" \
			"msr cpsr_c, %0\n" \
			: "=r" (cpsr) \
			: "i" (ARM_IRQ_MASK) \
			: "memory"); \
	} while (0)

#define irq_disable() \
	do { \
		__u32 cpsr; \
		asm volatile ("mrs %0, cpsr\n" \
			"orr %0, %0, %1\n" \
			"msr cpsr_c, %0\n" \
			: "=r" (cpsr) \
			: "i" (ARM_IRQ_MASK) \
			: "memory"); \
	} while (0)

#define lock_irq_psr(cpsr) \
	do { \
		unsigned long _________tmp; \
		asm volatile ("mrs %0, cpsr\n" \
			"orr %1, %0, %2\n" \
			"msr cpsr_c, %1\n" \
			: "=r" (cpsr), "=r"(_________tmp) \
			: "i" (ARM_IRQ_MASK) \
			: "memory"); \
	} while (0)

#define unlock_irq_psr(cpsr) \
	do { \
		asm volatile ("msr cpsr_c, %0\n" \
			: \
			: "r" (cpsr) \
			: "memory"); \
	} while (0)

#define fiq_enable() \
	do { \
		unsigned long cpsr; \
		asm volatile ("mrs %0, cpsr\n" \
			"bic %0, %0, %1\n" \
			"msr cpsr_c, %0\n" \
			: "=r" (cpsr) \
			: "i" (ARM_FIQ_MASK) \
			: "memory"); \
	} while (0)
#else // FIXME
#define irq_enable()
#define irq_disable()
#define lock_irq_psr(cpsr)
#define unlock_irq_psr(cpsr)
#define fiq_enable()
#endif
#endif

#define GTH_MAGIC          (('G' << 24) | ('B' << 16) | (('t' - 'a') << 8) | 'h')
#define GTH_MAGIC_OFFSET    32

#define GBH_MAGIC          (('G' << 24) | ('B' << 16) | (('b' - 'a') << 8) | 'h')
#define GBH_MAGIC_OFFSET    32

#define GBH_SIZE_OFFSET     20

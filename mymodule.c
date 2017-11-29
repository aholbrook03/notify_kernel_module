/* notify.c
 * Andrew Holbrook
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

// function prototypes
static int mykthread(void *unused);
void daemonize(const char *name, ...);
int allow_signal(int sig);
static void run_umode_handler(int event_id);
static int read_func_proc(char *, char**, off_t, int, int *, void *);
static int read_func_mem(char *, char**, off_t, int, int *, void *);
static int write_func_proc(struct file*, const char*, unsigned long, void*);
static int write_func_mem(struct file*, const char*, unsigned long, void*);

// proc variables
static struct proc_dir_entry *proc_max;
static struct proc_dir_entry *proc_mem;

extern rwlock_t myevent_lock;
extern unsigned int myevent_id2;
extern wait_queue_head_t myevent_waitqueue;
extern unsigned int max_mem_percent;
extern unsigned int max_proc;

// sysctl variables
static char myevent_str[256] = {0};
static struct ctl_table_header *_sysctl_header;
static struct ctl_table my_table = {
	.procname = "myevent_handler",
	.mode = 0644,
	.data = myevent_str,
	.maxlen = 256,
	.proc_handler = proc_dostring,
};

static int mymodule_init(void)
{
	int ret;

	_sysctl_header = register_sysctl_table(&my_table);

	proc_max = create_proc_entry("myevent_max_proc", 0644, NULL);
	proc_max->read_proc = read_func_proc;
	proc_max->write_proc = write_func_proc;

	proc_mem = create_proc_entry("myevent_max_mem", 0644, NULL);
	proc_mem->read_proc = read_func_mem;
	proc_mem->write_proc = write_func_mem;

	ret = kernel_thread(mykthread, NULL, 0);

	return 0;
}

static void mymodule_exit(void)
{
	unregister_sysctl_table(_sysctl_header);
	remove_proc_entry("myevent_max_proc", NULL);
	remove_proc_entry("myevent_max_mem", NULL);
}

/*
 * mykthread sleeps until the kernel wakes it up.
 */
static int mykthread(void *unused)
{
	unsigned int event_id = 0;
	DECLARE_WAITQUEUE(wait, current);

	daemonize("mykthread");

	allow_signal(SIGKILL);

	add_wait_queue(&myevent_waitqueue, &wait);

	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		schedule();

		if (signal_pending(current)) break;

		read_lock(&myevent_lock);

		if (myevent_id2) {
			event_id = myevent_id2;
			read_unlock(&myevent_lock);
			run_umode_handler(event_id);
		} else {
			read_unlock(&myevent_lock);
		}

	}

	set_current_state(TASK_RUNNING);
	remove_wait_queue(&myevent_waitqueue, &wait);
	return 0;
}

static void run_umode_handler(int event_id)
{
	int i = 0;
	char *argv[2], *envp[4], *buffer = NULL;
	int value;

	if (myevent_str[0] == 0) return;

	argv[i++] = myevent_str;

	if (!(buffer = kmalloc(32, GFP_KERNEL))) return;

	sprintf(buffer, "TROUBLED_DS=%d", event_id);

	argv[i] = 0;

	i = 0;
	envp[i++] = "HOME=/";
	envp[i++] = "PATH=/sbin:/usr/sbin:/bin:/usr/bin";
	envp[i++] = buffer; envp[i] = 0;
	value = call_usermodehelper(argv[0], argv, envp, 0);

	kfree(buffer);
}

// read max number of processes before notifying the user
static int read_func_proc(char *page, char **start, off_t off, int count, int *eof,
					 	  void *data)
{
	int len;
	len = sprintf(page, "%u\n", max_proc);
	return len;
}

// write max number of processes before notifying the user
static int write_func_proc(struct file* file, const char* buffer,
					  	   unsigned long count, void* data)
{
	char buf[count];
	char *endp;

	copy_from_user(buf, buffer, count);

	max_proc = simple_strtoul(buf, &endp, 10);

	return count;
}

// read percent of memory usage allowed before notifying the user
static int read_func_mem(char *page, char **start, off_t off, int count, int *eof,
					 	 void *data)
{
	int len;
	len = sprintf(page, "%u\n", max_mem_percent);
	return len;
}

// write percent of memory usage allowed before notifying the user
static int write_func_mem(struct file* file, const char* buffer,
					  	  unsigned long count, void* data)
{
	char buf[count];
	char *endp;

	copy_from_user(buf, buffer, count);

	max_mem_percent = simple_strtoul(buf, &endp, 10);

	return count;
}

module_init(mymodule_init);
module_exit(mymodule_exit);

MODULE_LICENSE("GPL");

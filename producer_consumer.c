#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/semaphore.h>
#include <linux/kthread.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/ktime.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/slab.h>
#include <linux/math64.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Campbell");
MODULE_DESCRIPTION("Prints task list");

struct task_struct *task;
static uint64_t s_time = 0;
static uint64_t totalTime = 0;
static uint64_t hh;
static uint64_t mm;
static uint64_t ss;

static int uid = 0;		//user id
static int buff_size = 0;	//buffer size
static int p = 0;		//num of producer (0 or 1)
static int c = 0;		//num of comsumers (0 or 1) 


static int buffer[500] = { 0 };

module_param(uid, int, 0);
module_param(buff_size, int, 0);
module_param(p, int, 0);
module_param(c, int, 0);

struct semaphore empty;
struct semaphore full;
//struct mutex my_mutex;
static int in = 0;
static int out = 0;
static int pCount = 0;
static int cCount = 0;

struct task_struct *task1;
struct task_struct *task2;
struct task_struct *p1;
struct task_struct *c1;
int err = 0;

static int producer(void *arg) {
	for_each_process(task1) {
		if (down_interruptible(&empty)) { break; }
		
		if (uid == task1->cred->uid.val)
		{
			//insert item into buffer
			//insertTask(count, task->pid, task->start_time);
			pCount++;
			buffer[in] = task1->pid;
			printk(KERN_INFO "[Producer-1] Produced Item#-%i at buffer index:%i for PID:%i\n", pCount, in, task1->pid);
			in = (in + 1) % buff_size;
		}
		
		up(&full);
	}
	return 0;
}

static int consumer(void *arg)
{
	do {	
		if(kthread_should_stop()) { break; }
		
		for_each_process(task2) {
		
			if(kthread_should_stop()) { break; }
			
			if (down_interruptible(&full)) { break; }
			
			if (task2->pid == buffer[out]) {
				cCount++;
				s_time = ktime_get_ns() - task2->start_time;
				totalTime += s_time;
				
				s_time = div64_u64(s_time,1000000000);
				hh = div_u64(s_time, 3600);
				div64_u64_rem(s_time, 3600, &mm);
				mm = div_u64(mm, 60);
				div64_u64_rem(s_time, 60, &ss);
				printk(KERN_INFO "[Consumer-1] Consumed Item#-%i at buffer index:%i for PID:%i Elapsed Time-%lli:%lli:%lli\n", cCount, out, task2->pid, hh, mm, ss);
				out = (out + 1) % buff_size;
			}
			
		
			up(&empty);
			
			//free(temp);
		}
			
	} while(1);
	return 0;
}

static int __init prod_con_init(void)
{
	sema_init(&empty, buff_size);
	sema_init(&full, 0);
	//mutex_init(&my_mutex);
	
	
	
	
	if (p != 0) {
		printk(KERN_INFO "Running p1 as p=%i\n", p);
		p1 = kthread_run(producer, NULL, "Producer-1");
	
		if (IS_ERR(p1))
		{
			printk(KERN_INFO "Error cannot create thread p1\n");
			err = PTR_ERR(p1);
			p1 = NULL;
			return err;
		}
	}
	if (c != 0) {
		printk(KERN_INFO "Running c1 as c=%i\n", c);
		c1 = kthread_run(consumer, NULL, "Consumer-1");
	
		if (IS_ERR(c1))
		{
			printk(KERN_INFO "Error cannot create thread c1\n");
			err = PTR_ERR(c1);
			c1 = NULL;
			return err;
		}
	}
	return 0;
}

static void __exit prod_con_cleanup(void) {
	up(&empty);
	up(&full);
	
	totalTime = div64_u64(s_time,1000000000);
	hh = div_u64(totalTime, 3600);
	div64_u64_rem(totalTime, 3600, &mm);
	mm = div_u64(mm, 60);
	div64_u64_rem(totalTime, 60, &ss);
	printk(KERN_INFO "Total Elapsed Time-%lli:%lli:%lli\n", hh, mm, ss);
	
	if (c != 0) {
		kthread_stop(c1);
	}
}

module_init(prod_con_init);
module_exit(prod_con_cleanup);

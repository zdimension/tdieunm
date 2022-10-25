#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tom Niget");
MODULE_DESCRIPTION("Module qui affiche la liste des processus");

static const char* process_policy(int policy)
{
    switch (policy)
    {
        case SCHED_NORMAL:
            return "NORMAL";
        case SCHED_FIFO:
            return "FIFO";
        case SCHED_RR:
            return "RR";
        default:
            return "UNKNOWN";
    }
}

static void display_tree(struct task_struct *task, int level)
{
    struct task_struct *child;
    printk(KERN_INFO "%*c %s (pid=%d) %s, priority=%d\n", level * 2, ' ', task->comm, task->pid, process_policy(task->policy), task->prio);
    list_for_each_entry(child, &task->children, sibling)
    {
        display_tree(child, level + 1);
    }
}

static int __init td7_init(void)
{
    display_tree(&init_task, 1);
    return 0;
}

static void __exit td7_exit(void)
{
    printk(KERN_INFO "Module td7 unloaded\n");
}

module_init(td7_init);
module_exit(td7_exit);
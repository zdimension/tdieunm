#include <linux/module.h>

MODULE_LICENSE("GPL");

static int n = 1;
static char* str = "cruel world!";

module_param(n, int, 0660);
module_param(str, charp, 0660);

static int __init init(void)
{
	for (int i = n; i--; )
		printk("Hello World!\n");
	return 0;
}

static void __exit cleanup(void)
{
	printk("Goodbye %s\n", str);
}

module_init(init);
module_exit(cleanup);

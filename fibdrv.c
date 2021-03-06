#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uaccess.h>


MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 100

static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

struct BigN {
    unsigned long long lower, upper;
};

static inline void addBigN(struct BigN *output, struct BigN x, struct BigN y)
{
    output->upper = x.upper + y.upper;
    if (y.lower > ~x.lower)
        output->upper++;
    output->lower = x.lower + y.lower;
}

/*static long long fib_sequence(long long k)
{
    // FIXME: use clz/ctz and fast algorithms to speed up
    long long f[k + 2];

    f[0] = 0;
    f[1] = 1;

    for (int i = 2; i <= k; i++) {
        f[i] = f[i - 1] + f[i - 2];
    }

    return f[k];
}*/

/*static struct BigN  *init_BigN(struct BigN *x)
{
    x->lower = 0;
    x->upper = 0;
    return x;
}*/

/*static struct BigN *fib_sequence(long long k)
{
    struct BigN *front = (struct BigN *) kmalloc(sizeof(struct BigN),
GFP_KERNEL); struct BigN *back = (struct BigN *) kmalloc(sizeof(struct BigN),
GFP_KERNEL); struct BigN *rs = (struct BigN *) kmalloc(sizeof(struct BigN),
GFP_KERNEL);

    front = init_BigN(front);
    front->lower = 1;
    back = init_BigN(back);
    rs = init_BigN(rs);

    for (int i = 2; i <= k; i++) {
        //addBigN(rs,*front,*back);
        addBigN(rs,*back,*front);
        back = front;
        front = rs;
    }

    return rs; // search receiving part and check the type
}*/

static struct BigN fib_sequence(long long k)
{
    /* FIXME: use clz/ctz and fast algorithms to speed up */
    struct BigN f[k + 2];

    f[0].upper = 0;
    f[0].lower = 0;

    f[1].upper = 0;
    f[1].lower = 1;

    for (int i = 2; i <= k; i++) {
        addBigN(&f[i], f[i - 1], f[i - 2]);
    }

    // printk("%lld , %llu , %llu", k, f[k].upper, f[k].lower);
    return f[k];
}


/*static long long fib_doubling(long long k)
{
    if (k == 0)
        return 0;

    long long t0 = 1, t1 = 1, t3 = 1, t4 = 0;

    long long i = 1;
    while (i < k) {
        if ((i << 1) <= k) {
            t4 = t1 * t1 + t0 * t0;
            t3 = t0 * (2 * t1 - t0);
            t0 = t3;
            t1 = t4;
            i <<= 1;
        } else {
            t0 = t3;
            t3 = t4;
            t4 = t0 + t4;
            i++;
        }
    }
    return t3;
}*/

static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}

/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    char kbuffer[100] = {0};

    ktime_t kt1 = ktime_get();
    struct BigN result64 = fib_sequence(*offset);
    kt1 = ktime_sub(ktime_get(), kt1);
    printk("%lld : %llu", *offset, result64.lower);
    snprintf(kbuffer, sizeof(kbuffer), "%llu", result64.lower);
    copy_to_user(buf, kbuffer, 100);

    /*ktime_t kt2 = ktime_get();
    fib_doubling(*offset);
    kt2 = ktime_sub(ktime_get(), kt2);*/

    // printk(KERN_INFO "%lld %lld", ktime_to_ns(kt1), ktime_to_ns(kt2));
    // printk(KERN_INFO "%lld ", ktime_to_ns(kt1));
    return result64.upper;
}

/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return 1;
}

static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    cdev_init(fib_cdev, &fib_fops);
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);

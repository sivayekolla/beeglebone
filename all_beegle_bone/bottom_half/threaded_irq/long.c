
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/timekeeping.h>
#include <linux/slab.h>
#include <asm/io.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Long-press (hold) detection using threaded IRQ");


/* Module parameters */
static unsigned int long_press_ms = 2000;   /* hold threshold in milliseconds */
module_param(long_press_ms, uint, 0444);
MODULE_PARM_DESC(long_press_ms, "Long press threshold in milliseconds");

static unsigned int target_scancode = 1;    /* default: ESC (make code 0x01) */
module_param(target_scancode, uint, 0444);
MODULE_PARM_DESC(target_scancode, "Target scancode (make code) to detect long press; default 1 = ESC");

/* Path to log file */
static char *log_path = "/var/log/alert.log";
module_param(log_path, charp, 0444);
MODULE_PARM_DESC(log_path, "Path to write alert message");

/* Keyboard I/O */
#define KBD_DATA_REG        0x60
#define KBD_SCANCODE_MASK   0x7f
#define KBD_STATUS_MASK     0x80

/* IRQ and dev id */
static int kbd_irq = 1;         /* keyboard IRQ on x86 */
static int dev_id = 0xAA;

/* Press state array for low 7-bit scancodes (0..127). Protected by spinlock */
static bool pressed[128];
static DEFINE_SPINLOCK(pressed_lock);

/* Helper: write alert message to file (runs in threaded IRQ context -> process context) */
static void write_alert_to_file(const char *path, const char *msg)
{
    struct file *f = NULL;
    loff_t pos = 0;
    ssize_t written;
    struct timespec64 ts;
    struct tm tm;
    char buf[256];
    int len;

    /* Compose timestamped message */
    ktime_get_real_ts64(&ts);
    time64_to_tm(ts.tv_sec, 0, &tm);
    /* format: YYYY-MM-DD HH:MM:SS */
    len = snprintf(buf, sizeof(buf), "%04ld-%02d-%02d %02d:%02d:%02d %s\n",
                   (long)tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec, msg);

    /* Open file with append mode */
    f = filp_open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (IS_ERR(f)) {
        pr_err("kbd_longpress: filp_open failed for %s\n", path);
        return;
    }

    /* kernel_write is safe in process context (threaded IRQ) */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,14,0)
    written = kernel_write(f, buf, len, &pos);
#else
    /* Older kernels might have different kernel_write signature; try vfs_write fallback */
    written = vfs_write(f, buf, len, &pos);
#endif

    if (written < 0) {
        pr_err("kbd_longpress: kernel write failed (%zd)\n", written);
    } else {
        pr_info("kbd_longpress: alert written to %s\n", path);
    }

    filp_close(f, NULL);
}

/* Top half: hard IRQ handler (very small). Return IRQ_WAKE_THREAD for presses of target key */
static irqreturn_t kbd_top_handler(int irq, void *dev)
{
    unsigned char sc;
    unsigned int idx;
    unsigned long flags;
    bool is_press;

    sc = inb(KBD_DATA_REG);
    idx = sc & KBD_SCANCODE_MASK;
    is_press = !(sc & KBD_STATUS_MASK); /* if high bit set -> release */

    /* update pressed state under spinlock */
    spin_lock_irqsave(&pressed_lock, flags);
    if (is_press)
        pressed[idx] = true;
    else
        pressed[idx] = false;
    spin_unlock_irqrestore(&pressed_lock, flags);

    /* We only want the threaded handler to run for target key *press* events */
    if (is_press && idx == target_scancode)
        return IRQ_WAKE_THREAD;

    /* otherwise handled quickly here */
    return IRQ_HANDLED;
}

/* Threaded handler: runs in process context, can sleep */
static irqreturn_t kbd_thread_fn(int irq, void *dev)
{
    unsigned long sleep_ms = long_press_ms;
    unsigned long flags;
    bool still_pressed;

    /* Sleep for required hold duration */
    msleep(sleep_ms);

    /* After sleeping, check whether target scancode is still pressed */
    spin_lock_irqsave(&pressed_lock, flags);
    still_pressed = pressed[target_scancode];
    spin_unlock_irqrestore(&pressed_lock, flags);

    if (still_pressed) {
        /* Trigger emergency action: write to file */
        write_alert_to_file(log_path, "EMERGENCY ALERT TRIGGERED");
    } else {
        pr_info("kbd_longpress: target key released before threshold (%u ms)\n", (unsigned)long_press_ms);
    }

    return IRQ_HANDLED;
}

/* Module init/exit */
static int __init kbd_longpress_init(void)
{
    int ret;

    pr_info("kbd_longpress: init (target_scancode=%u, long_press_ms=%u, log=%s)\n",
            target_scancode, long_press_ms, log_path);

    /* Ensure pressed[] is cleared */
    memset(pressed, 0, sizeof(pressed));

    /* request threaded IRQ: top handler + thread function */
    ret = request_threaded_irq(kbd_irq,
                               kbd_top_handler,      /* primary/top handler (hard IRQ) */
                               kbd_thread_fn,        /* thread handler (process context) */
                               IRQF_SHARED,          /* flags: keyboard line is often shared */
                               "kbd_longpress_thread", &dev_id);
    if (ret) {
        pr_err("kbd_longpress: request_threaded_irq failed: %d\n", ret);
        return ret;
    }

    pr_info("kbd_longpress: loaded\n");
    return 0;
}

static void __exit kbd_longpress_exit(void)
{
    pr_info("kbd_longpress: exit\n");

    /* free IRQ and ensure thread is stopped */
    free_irq(kbd_irq, &dev_id);
    pr_info("kbd_longpress: unloaded\n");
}

module_init(kbd_longpress_init);
module_exit(kbd_longpress_exit);


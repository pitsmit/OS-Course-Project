#include <linux/kprobes.h>
int register_kprobe(struct kprobe *kp);
int register_kretprobe(struct kretprobe *rp);
void unregister_kprobe(struct kprobe *kp);
void unregister_kretprobe(struct kretprobe *rp);
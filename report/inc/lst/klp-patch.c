struct klp_patch {
	struct module *mod;
	struct klp_object *objs;
	...
	bool replace;
	struct list_head list;
	...
	bool enabled;
	...
};
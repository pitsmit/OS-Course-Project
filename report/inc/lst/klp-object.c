struct klp_object {
	const char *name;
	struct klp_func *funcs;
	...
	struct module *mod;
	...
	bool patched;
};
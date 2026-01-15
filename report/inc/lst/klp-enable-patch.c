int klp_enable_patch(struct klp_patch *patch)
{
    int ret;
    // Проверка, что модуль является livepatch
    if (!is_livepatch_module(patch->mod))
        return -EINVAL;
    mutex_lock(&klp_mutex);
    // Проверка совместимости с другими патчами
    if (!klp_is_patch_compatible(patch)) {
        mutex_unlock(&klp_mutex);
        return -EINVAL;
    }
    // Инициализация патча
    ret = klp_init_patch(patch);
    if (ret)
        goto err;
    // Включение патча
    ret = __klp_enable_patch(patch);
    if (ret)
        goto err;
    mutex_unlock(&klp_mutex);
    return 0;
err:
    mutex_unlock(&klp_mutex);
    return ret;
}
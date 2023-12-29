
#define NVS_STORAGE "nvs"
#include "nvs_helper.h"
void save_string(char* key, char *str)
{
    nvs_handle_t my_nvs_handle;
    nvs_open(NVS_STORAGE, NVS_READWRITE, &my_nvs_handle);
    nvs_set_str(my_nvs_handle, key, str);
    nvs_commit(my_nvs_handle);
    nvs_close(my_nvs_handle);
}
void load_string(char *key, char *str)
{
    nvs_handle_t my_nvs_handle;
    size_t read_size=0;

    nvs_open(NVS_STORAGE, NVS_READONLY, &my_nvs_handle);
    nvs_get_str(my_nvs_handle, key, NULL, &read_size);

    if (read_size>0)
        nvs_get_str(my_nvs_handle, key, str, &read_size);

    nvs_close(my_nvs_handle);
}

void load_int(char *key, int16_t *value)
{
    nvs_handle_t my_nvs_handle;
    size_t read_size=0;

    nvs_open(NVS_STORAGE, NVS_READONLY, &my_nvs_handle);

    esp_err_t err = nvs_get_i16(my_nvs_handle,key, value);

    if (err == 0)
    {
        printf("loaded value %s, %d\n", key, *value);
    }else
    {
        printf("error reading %s, %x\n", key, err);
    }
    nvs_close(my_nvs_handle);
}

void save_int(char *key, int16_t *value)
{
    nvs_handle_t my_nvs_handle;
    nvs_open(NVS_STORAGE, NVS_READWRITE, &my_nvs_handle);
    esp_err_t err = nvs_set_i16(my_nvs_handle, key, *value);

    if (err>0)
    {
        printf("error saving %s, %x\n", key, err);
    }
    nvs_commit(my_nvs_handle);
    nvs_close(my_nvs_handle);
}
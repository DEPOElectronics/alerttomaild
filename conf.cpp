#include <string.h>

#include <conf.hpp>
#include <iostream>

int xopen_ini(const char* filename, xvalue* arr);

// получения значения по указанному ключу с учётом регистра
char* xget_value(xvalue* vals, const char* key)
{
    int i;
    for (i = 0; i < MAX_INI_RECORDS; i++)
    {
        if (!strcmp(vals[i].key, key))
            return vals[i].val;
    }
    return NULL;
}

// открытие файла - ini
int xopen_ini(const char* filename, xvalue* arr)
{
    xvalue val;
    FILE* fp;
    int cnt;
    if (!(fp = fopen(filename, "r")))
        return 0;
    memset((void*)&val, 0, sizeof(xvalue));

    for (cnt = 0; fscanf(fp, "%[^=]=%[^\n]%*1c", val.key, val.val) > 1; cnt++)
        memcpy((void*)&arr[cnt], (const void*)&val, sizeof(xvalue));
    fclose(fp);
    return 1;
}

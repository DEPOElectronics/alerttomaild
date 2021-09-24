/*
 * conf.hpp
 *
 *  Created on: 17 сент. 2021 г.
 *      Author: glukhov_mikhail
 */

#ifndef CONF_HPP_
#define CONF_HPP_

#define BUF_KEY 32        // размер буфера для ключа
#define BUF_VAL 255       // размер буфера для значения
#define MAX_INI_RECORDS 7 // кол-во записей в ini-файле

typedef struct _value
{
    char key[BUF_KEY];
    char val[BUF_VAL];
} xvalue;

int xopen_ini(const char* filename, xvalue* arr);
char* xget_value(xvalue* vals, const char* key);

#endif /* CONF_HPP_ */

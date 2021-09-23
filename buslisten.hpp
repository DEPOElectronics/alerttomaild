/*
 * bus.hpp
 *
 *  Created on: 20 сент. 2021 г.
 *      Author: glukhov_mikhail
 */

#ifndef BUSLISTEN_HPP_
#define BUSLISTEN_HPP_

#include <sdbusplus/bus/match.hpp>

int BusListen(const char*, const char *SendMail, const char *PostMail,
		bool ColorMail, bool TestRun);

#endif /* BUS_HPP_ */

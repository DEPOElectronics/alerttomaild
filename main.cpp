#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <conf.hpp>

using namespace std;
#define RESERROR -1

void WriteHelp()
{
	printf("Help: \n-h --help - Write this help and exit\n-c --config - "
			"Set config file\n-d --daemon - run as daemon\n");
	return;
}

bool Daemon = false;
char *SendTo;

int LoadConf(const char *Conf)
{
	xvalue *vals = NULL;

	struct stat buffer;
	if (stat(Conf, &buffer) == 0)
	{
		vals = (xvalue*) malloc(sizeof(xvalue) * MAX_INI_RECORDS);
		if (vals == NULL)
			return RESERROR;

		// если ini-файл будет открыт, массив заполниться записиями из файла
		if (!xopen_ini(Conf, vals))
		{
			free(vals);
			return RESERROR;
		}

		SendTo = xget_value(vals, "SendTo");
		if (SendTo != NULL)
		{
			free(vals);
			return RESERROR;
		}
		free(vals);
	}

	else
		return RESERROR;
	return 0;
}

char *ConfFile = strdup("/etc/alerttomaild.conf");

int ReadParam(int argc, char **argv)
{
	int optIdx;

	static struct option long_opt[] =
	{
	{ "daemon", no_argument, nullptr, 'd' },
	{ "help", no_argument, nullptr, 'h' },
	{ "config", required_argument, nullptr, 'c' },
	{ 0, 0, 0, 0 } };
	int c;

	while ((c = getopt_long(argc, argv, "dc:h", long_opt, &optIdx)) >= 0)
	{
		switch (c)
		{
		case 'c':
			printf("option 'c' selected, filename: %s\n", optarg);
			ConfFile = optarg;
			break;

		case 'd':
			printf("this is daemon!\n");
			break;

		case 'h':
			WriteHelp();
			_Exit(0);
			break;

		default:
			WriteHelp();
			_Exit(-1);
		}
	}
	return 0;
}
;

int main(int argc, char **argv)
{
	if (ReadParam(argc, argv) < 0)
	{
		printf("Parse param error");
		return -1;
	}
	if (LoadConf(ConfFile) < 0)
	{
		printf("Parse conf file error");
		return -1;
	}
	if (Daemon)
	{
		//SetDaemon();
	}
	return 0;
}


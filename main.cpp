#include <iostream>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/stat.h>
#include <conf.hpp>
#include <daemon.hpp>
#include <buslisten.hpp>

using namespace std;
#define RESERROR -1

void WriteHelp()
{
	printf("Help: \n-H --help - Write this help and exit\n-C --config - "
			"Set config file\n-D --daemon - run as daemon\n");
	return;
}

bool Daemon = false;
char* SendTo;
char* SendMail=strdup("msmtp-enqueue.sh");
char* PostMail=strdup("msmtp-runqueue.sh");


int LoadConf(const char *Conf)
{
	xvalue *vals = NULL;

	struct stat buffer;
	if (stat(Conf, &buffer) == 0)
	{
		vals = (xvalue*) malloc(sizeof(xvalue) * MAX_INI_RECORDS);
		if (vals == NULL)
			return RESERROR;

		// если ini-файл будет открыт, массив заполнится записями из файла
		if (!xopen_ini(Conf, vals))
		{
			free(vals);
			return RESERROR;
		}

		char* FSendTo = xget_value(vals, "SendTo");
		if (FSendTo == NULL)
		{
			printf("SendTo settings not found!\n");
			free(vals);
			return RESERROR;
		}
		else
		{
			SendTo=strdup(FSendTo);
		}

		char* SetSendMail=xget_value(vals, "SendMail");
		if (SetSendMail!=NULL)
		{
			SendMail=strdup(SetSendMail);
		}

		char* SetPostMail=xget_value(vals, "PostMail");
		if (SetPostMail!=NULL)
		{
			PostMail=strdup(SetPostMail);
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
	{ "daemon", no_argument, nullptr, 'D' },
	{ "help", no_argument, nullptr, 'H' },
	{ "config", required_argument, nullptr, 'C' },
	{ 0, 0, 0, 0 } };
	int c;

	while ((c = getopt_long(argc, argv, "DC:H", long_opt, &optIdx)) >= 0)
	{
		switch (c)
		{
		case 'C':
			printf("option 'C' selected, filename: %s\n", optarg);
			ConfFile = optarg;
			break;

		case 'D':
			Daemon = true;
			break;

		case 'H':
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

int main(int argc, char **argv)
{
	if (ReadParam(argc, argv) < 0)
	{
		printf("Parse param error");
		return EXIT_FAILURE;
	}
	if (LoadConf(ConfFile) < 0)
	{
		printf("Parse conf file error\n");
		return EXIT_FAILURE;
	}
	if (Daemon)
	{
		printf("Run as daemon\n");
		if (SetDaemon(0) != EXIT_SUCCESS)
		{
			return EXIT_FAILURE;
		}
	}

	BusListen(SendTo, SendMail, PostMail);

	return EXIT_SUCCESS;
}

/*int MonitorProc()
{
    int      pid;
    int      status;
    int      need_start = 1;
    sigset_t sigset;
    siginfo_t siginfo;

    // настраиваем сигналы которые будем обрабатывать
    sigemptyset(&sigset);

    // сигнал остановки процесса пользователем
    sigaddset(&sigset, SIGQUIT);

    // сигнал для остановки процесса пользователем с терминала
    sigaddset(&sigset, SIGINT);

    // сигнал запроса завершения процесса
    sigaddset(&sigset, SIGTERM);

    // сигнал посылаемый при изменении статуса дочернего процесса
    sigaddset(&sigset, SIGCHLD);

    // пользовательский сигнал который мы будем использовать для обновления конфига
    sigaddset(&sigset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    // данная функция создаст файл с нашим PID'ом
    SetPidFile(PID_FILE);

    // бесконечный цикл работы
    for (;;)
    {
        // если необходимо создать потомка
        if (need_start)
        {
            // создаём потомка
            pid = fork();
        }

        need_start = 1;

        if (pid == -1) // если произошла ошибка
        {
            // запишем в лог сообщение об этом
            WriteLog("[MONITOR] Fork failed (%s)\n", strerror(errno));
        }
        else if (!pid) // если мы потомок
        {
            // данный код выполняется в потомке

            // запустим функцию отвечающую за работу демона
            status = WorkProc();

            // завершим процесс
            exit(status);
        }
        else // если мы родитель
        {
            // данный код выполняется в родителе

            // ожидаем поступление сигнала
            sigwaitinfo(&sigset, &siginfo);

            // если пришел сигнал от потомка
            if (siginfo.si_signo == SIGCHLD)
            {
                // получаем статус завершение
                wait(&status);

                // преобразуем статус в нормальный вид
                status = WEXITSTATUS(status);

                 // если потомок завершил работу с кодом говорящем о том, что нет нужды дальше работать
                if (status == CHILD_NEED_TERMINATE)
                {
                    // запишем в лог сообщени об этом
                    WriteLog("[MONITOR] Child stopped\n");

                    // прервем цикл
                    break;
                }
                else if (status == CHILD_NEED_WORK) // если требуется перезапустить потомка
                {
                    // запишем в лог данное событие
                    WriteLog("[MONITOR] Child restart\n");
                }
            }
            else if (siginfo.si_signo == SIGUSR1) // если пришел сигнал что необходимо перезагрузить конфиг
            {
                kill(pid, SIGUSR1); // перешлем его потомку
                need_start = 0; // установим флаг что нам не надо запускать потомка заново
            }
            else // если пришел какой-либо другой ожидаемый сигнал
            {
                // запишем в лог информацию о пришедшем сигнале
                WriteLog("[MONITOR] Signal %s\n", strsignal(siginfo.si_signo));

                // убьем потомка
                kill(pid, SIGTERM);
                status = 0;
                break;
            }
        }
    }

    // запишем в лог, что мы остановились
    WriteLog("[MONITOR] Stop\n");

    // удалим файл с PID'ом
    unlink(PID_FILE);

    return status;
}
*/

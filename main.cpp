#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <buslisten.hpp>
#include <conf.hpp>
#include <daemon.hpp>
#include <sstream>

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
char* SendMail = strdup("msmtp-enqueue.sh");
char* PostMail = strdup("msmtp-runqueue.sh");
char* PID_FILE = strdup("/var/run/alerttomaild.pid");
char* Severity = strdup("Critical");
bool ColorMail = false;
bool TestRun = false;

bool strtobool(std::string str)
{
    std::istringstream is(str);
    bool b;
    is >> std::boolalpha >> b;
    return b;
}

int LoadConf(const char* Conf)
{
    xvalue* vals = NULL;

    struct stat buffer;
    if (stat(Conf, &buffer) == 0)
    {
        vals = (xvalue*)malloc(sizeof(xvalue) * MAX_INI_RECORDS);
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
            SendTo = strdup(FSendTo);
        }

        char* SetSendMail = xget_value(vals, "SendMail");
        if (SetSendMail != NULL)
        {
            SendMail = strdup(SetSendMail);
        }

        char* SetPostMail = xget_value(vals, "PostMail");
        if (SetPostMail != NULL)
        {
            PostMail = strdup(SetPostMail);
        }

        char* SetPIDFILE = xget_value(vals, "PID");
        if (SetPIDFILE != NULL)
        {
            PID_FILE = strdup(SetPIDFILE);
        }

        char* SetSeverity = xget_value(vals, "Severity");
        if (SetSeverity != NULL)
        {
            Severity = SetSeverity;
        }

        char* SetColor = xget_value(vals, "ColorMail");
        if (SetColor != NULL)
        {
            ColorMail = strtobool(SetColor);
        }

        free(vals);
    }

    else
        return RESERROR;
    return 0;
}

char* ConfFile = strdup("/etc/alerttomaild.conf");

int ReadParam(int argc, char** argv)
{
    int optIdx;

    static struct option long_opt[] = {
        {"daemon", no_argument, nullptr, 'D'},
        {"help", no_argument, nullptr, 'H'},
        {"config", required_argument, nullptr, 'C'},
        {"test", no_argument, nullptr, 'T'},
        {0, 0, 0, 0}};
    int c;

    while ((c = getopt_long(argc, argv, "DC:HT", long_opt, &optIdx)) >= 0)
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

            case 'T':
                TestRun = true;
                break;

            default:
                WriteHelp();
                _Exit(-1);
        }
    }
    return 0;
}

int main(int argc, char** argv)
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
        if (SetDaemon(0, PID_FILE) != EXIT_SUCCESS)
        {
            return EXIT_FAILURE;
        }
    }

    BusListen(SendTo, SendMail, PostMail, Severity, ColorMail, TestRun);

    return EXIT_SUCCESS;
}

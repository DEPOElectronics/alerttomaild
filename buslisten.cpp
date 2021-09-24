/*
 * bus.cpp
 *
 *  Created on: 20 сент. 2021 г.
 *      Author: glukhov_mikhail
 */

#include <unistd.h>

#include <buslisten.hpp>
#include <regex>
#include <sdbusplus/bus/match.hpp>
#include <sstream>

bool runstop = false;

using namespace std;

const char* FSendTo;
const char* FSendMail;
const char* FPostMail;
const char* FSeverityLevel;
bool FTestRun;
bool FColorMail = false;

const char* match = strdup(
    "type='signal',path=/xyz/openbmc_project/logging,member=InterfacesAdded");

const char* ENTRY = strdup("xyz.openbmc_project.Logging.Entry");
const char* FILEPATH = strdup("xyz.openbmc_project.Common.FilePath");

enum TSeverity
{
    TS_None,
    TS_Debug,
    TS_Informational,
    TS_Notice,
    TS_Warning,
    TS_Error,
    TS_Critical,
    TS_Alert,
    TS_Emergency,
};

char* HostName()
{
    const size_t LEN = 255;
    char buffer[LEN] = "";
    if (gethostname(buffer, LEN) == 0)
    {
        return strdup(buffer);
    }
    else
    {
        return strdup("");
    }
}

string LastWord(std::string s)
{
    std::smatch m;
    std::regex e("([^.]+$)"); // matches words beginning by "sub"

    if (std::regex_search(s, m, e))
    {
        return m.str();
    }
    else
    {
        return "";
    }
}

TSeverity GetSeverity(string sSeverity)
{
    static std::unordered_map<std::string, TSeverity> const table = {
        {"Emergency", TS_Emergency},
        {"Alert", TS_Alert},
        {"Critical", TS_Critical},
        {"Error", TS_Error},
        {"Warning", TS_Warning},
        {"Notice", TS_Notice},
        {"Informational", TS_Informational},
        {"Debug", TS_Debug}};
    auto it = table.find(sSeverity);
    if (it != table.end())
    {
        return it->second;
    }
    else
    {
        return TS_None;
    }
}

string GetColorState(TSeverity Severity)
{
    if (FColorMail)
    {
        switch (Severity)
        {
            case TS_Emergency:
            case TS_Alert:
            case TS_Critical:
            case TS_Error:
                return "🔴";
            case TS_Warning:
                return "🟠";
            case TS_Notice:
            case TS_Informational:
            case TS_Debug:
                return "🟢";
            default:;
        }
    }
    return "";
}

string TimeStampToString(uint64_t TimeStamp)
{
    time_t rawtime = TimeStamp / 1000;

    tm* timestruct = localtime((&rawtime));
    char arcString[32];
    if (strftime(&arcString[0], 20, "%Y-%m-%d %H:%M:%S", timestruct) != 0)
    {
        return (char*)&(arcString[0]);
    }
    else
    {
        return "1970-01-01 00:00:00";
    }
}

string GetMailText(string SendTo, uint32_t Id, uint64_t Timestamp,
                   string ShSeverity, const char* Message,
                   string AdditionalData)
{
    stringstream Res;
    TSeverity Severity = GetSeverity(ShSeverity);
    string CharState = GetColorState(Severity);

    Res << "Subject: " << CharState << " BMC " << HostName() << " "
        << ShSeverity << "\n";
    Res << "To: " << SendTo << "\n";
    Res << "Content-Type: text/plain; charset="
           "utf-8"
           "\n";
    Res << "\r\n";
    Res << "ID=#" << Id << " " << TimeStampToString(Timestamp) << " "
        << ShSeverity << "\n";
    Res << "Description: " << Message << "\n";
    Res << "Additional data:\n" << AdditionalData << "\n";
    return Res.str();
}

int Res(const int r, const char* s)
{
    if (r < 0)
    {
        printf("%s: Failed! %s", s, strerror(-r));
    }
    /*else
     printf("%s Ok\n", s);*/
    return r;
}

int LogEntry(sd_bus_message* m, uint32_t* Id, uint64_t* Timestamp,
             const char** Severity, const char** Message,
             string* AdditionalData)
{
    // read a{sv}
    string spropname;

    Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}"),
        "Enter subArray");
    while (sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0)
    {
        const char* propname;
        Res(sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &propname),
            "read propname");
        spropname = propname;

        if (spropname == "Id")
        {
            sd_bus_message_read(m, "v", "u", Id);
        }
        else if (spropname == "Timestamp")
        {
            sd_bus_message_read(m, "v", "t", Timestamp);
        }
        else if (spropname == "Severity")
        {
            sd_bus_message_read(m, "v", "s", Severity);
        }
        else if (spropname == "Message")
        {
            sd_bus_message_read(m, "v", "s", Message);
        }
        else if (spropname == "AdditionalData")
        {
            Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_VARIANT, "as"),
                "Enter variant");
            Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "s"),
                "Enter variant array");
            while (true)
            {
                const char* AddData;
                if (Res(sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING,
                                                  &AddData),
                        "Read variant s") <= 0)
                    break;
                if (*AdditionalData == "")
                    *AdditionalData = AddData;
                else
                    *AdditionalData = *AdditionalData + "\n" + AddData;
            };
            Res(sd_bus_message_exit_container(m), "Exit variant array");
            Res(sd_bus_message_exit_container(m), "Exit variant");
        }

        else
        {
            sd_bus_message_skip(m, "v");
        }

        Res(sd_bus_message_exit_container(m), "Exit subarray"); // array
    }
    Res(sd_bus_message_exit_container(m), "Exit subarray"); // array
    return 0;
}

int GetEventInfo(sd_bus_message* m, uint32_t* Id, uint64_t* Timestamp,
                 const char** Severity, const char** Message,
                 string* AdditionalData)
{
    // oa{sa{sv}}
    int r;
    int Result = -1;
    bool Allow = true;

    Res(sd_bus_message_skip(m, "o"), "Read path");
    Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sa{sv}}"),
        "Enter array");

    while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                               "sa{sv}")) > 0)
    {
        const char* intf;
        Res(sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &intf),
            "Read intf");
        if (strcmp(intf, ENTRY) == 0)
        {
            Result =
                LogEntry(m, Id, Timestamp, Severity, Message, AdditionalData);
        }
        else
        {
            if (strcmp(intf, FILEPATH) == 0)
            {
                const char* FilePath;
                printf("Read FilePath\n");
                Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY,
                                                   "{sv}"),
                    "FP_ENTER_ARRAY");
                Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                                   "sv"),
                    "FP_ENTER_DICT");
                Res(sd_bus_message_skip(m, "s"), "FP_skip_s");
                Res(sd_bus_message_read(m, "v", "s", &FilePath),
                    "FP_read_variant");
                printf("FilePath=%s\n", FilePath);
                if (strlen(FilePath) != 0)
                {
                    printf("not Allow\n");
                    Allow = false;
                }
                sd_bus_message_exit_container(m);
                sd_bus_message_exit_container(m);
            }
            else
            {
                Res(sd_bus_message_skip(m, "a{sv}"), "Skip array");
            }
        }
        Res(sd_bus_message_exit_container(m), "Exit dict"); // dict
    }

    Res(sd_bus_message_exit_container(m), "Exit array");
    if (r < 0)
    {
        printf("r=%s\n", strerror(-r));
    }

    if (Allow)
    {
        return Result;
    }
    else
    {
        return EXIT_FAILURE;
    }
}

int SendMail(string SendTo, const char* program, string TxT)
{
    std::stringstream stream;
    stream << "echo -e '" << TxT.c_str() << "'|" << program << " " << SendTo;
    printf("Send mail\n");
    return system(stream.str().c_str());
}

int PostMail(const char* RunScript)
{
    string FullRun;
    FullRun = RunScript;
    if (!FTestRun)
    {
        FullRun = FullRun + " &";
    }
    return system(FullRun.c_str());
}

TSeverity GetSevLevel(const char* SevLevel)
{
    static std::unordered_map<std::string, TSeverity> const table = {
        {"Critical", TS_Error}, {"Warning", TS_Warning}, {"Ok", TS_Notice}};
    auto it = table.find(SevLevel);
    if (it != table.end())
    {
        return it->second;
    }
    else
    {
        return TS_Critical;
    }
}

bool NeedSend(TSeverity Severity)
{
    TSeverity SevLevel = GetSevLevel(FSeverityLevel);
    return Severity >= SevLevel;
}

static int callback(sd_bus_message* m, void* user, sd_bus_error* error)
{
    uint32_t Id;
    uint64_t Timestamp;
    const char* FullSeverity;
    const char* Message;
    string AdditionalData = "";
    if (GetEventInfo(m, &Id, &Timestamp, &FullSeverity, &Message,
                     &AdditionalData) == 0)
    {
        string ShSeverity = LastWord(FullSeverity);
        TSeverity RealSeverity = GetSeverity(ShSeverity);

        if (NeedSend(RealSeverity))
        {
            string TxT = GetMailText(FSendTo, Id, Timestamp, ShSeverity,
                                     Message, AdditionalData);
            SendMail(FSendTo, FSendMail, TxT);
            if (FPostMail != NULL)
            {
                PostMail(FPostMail);
            }
        }
    }

    if (FTestRun)
    {
        runstop = true;
    }
    // runstop++;
    return 0;
}

void RunMonitor()
{
    int r = system(
        "dbus-monitor --system "
        "type='signal',path=/xyz/openbmc_project/logging,member=InterfacesAdded"
        " > /home/root/monitor &");
    if (r < 0)
    {
        printf("fail run monitor");
    }
}

int BusListen(const char* SendTo, const char* SendMail, const char* PostMail,
              const char* Severity, bool ColorMail, bool TestRun)
{
    // RunMonitor();
    // Settings
    FSendTo = strdup(SendTo);
    FSendMail = strdup(SendMail);
    FPostMail = strdup(PostMail);
    FSeverityLevel = strdup(Severity);
    FColorMail = ColorMail;
    FTestRun = TestRun;
    sd_bus* conn = NULL;
    sd_bus_slot* slot = NULL;

    // Action signal add  //oa{sa{sv}}
    sd_event* loop = NULL;
    int r = 0;

    sd_bus_default_system(&conn);
    sd_event_default(&loop);

    r = sd_bus_add_match(conn, &slot, match, callback, loop);

    if (r < 0)
    {
        fprintf(stderr, "Failed: %d sd_bus_add_match: %s\n", __LINE__,
                strerror(-r));
        goto finish;
    }

    for (;;)
    {
        /* Process requests */
        r = sd_bus_process(conn, NULL);
        if (r < 0)
        {
            fprintf(stderr, "Failed to process bus: %s\n", strerror(-r));
            goto finish;
        }
        if (r > 0)
        {
            continue;
        }

        if (runstop > 0)
        {
            goto finish;
        }

        r = sd_bus_wait(conn, (uint64_t)-1);
        if (r < 0)
        {
            fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
            goto finish;
        }
        if (runstop)
        {
            goto finish;
        }
    }

finish:
    sd_bus_slot_unref(slot);
    sd_bus_unref(conn);
    return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

/*
 * bus.cpp
 *
 *  Created on: 20 сент. 2021 г.
 *      Author: glukhov_mikhail
 */

#include <buslisten.hpp>
#include <iostream>
#include <sstream>

int callback_error = 0;

using namespace std;

const char *FSendTo;
const char *FSendMail;
const char *FPostMail;

string GetMailText(string SendTo, uint32_t Id, uint64_t Timestamp,
		const char *Severity, const char *Message, string AdditionalData)
{
	stringstream Res;
	Res << "Subject: BMC\n";
	Res << "To: " << SendTo << "\n";
	Res << "Content-Type: text/plain; charset="
			"utf-8"
			"\n";
	Res << "\r\n";
	Res << "ID=#" << Id << " Timestamp=" << Timestamp << "\n";
	Res << Severity << "\n";
	Res << Message << "\n";
	Res << AdditionalData << "\n";
	return Res.str();
}

int Res(const int r, const char *s)
{
	if (r < 0)
	{
		std::cout << s << ": " << "Failed! " << strerror(-r) << std::endl;
	}
	/*else
	 printf("%s Ok\n", s);*/
	return r;
}

void LogEntry(sd_bus_message *m, uint32_t *Id, uint64_t *Timestamp,
		const char **Severity, const char **Message, string *AdditionalData)
{
	// read a{sv}
	string spropname;

	Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sv}"),
			"Enter subArray");
	while (sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY, "sv") > 0)
	{
		const char *propname;
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
				const char *AddData;
				if (Res(
						sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING,
								&AddData), "Read variant s") <= 0)
					break;
				if (*AdditionalData == "")
					*AdditionalData = AddData;
				else
					*AdditionalData = *AdditionalData + "\n" + AddData;
			}
			;
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
}

int GetEventInfo(sd_bus_message *m, uint32_t *Id, uint64_t *Timestamp,
		const char **Severity, const char **Message, string *AdditionalData)
{
	// oa{sa{sv}}
	int r;

	Res(sd_bus_message_skip(m, "o"), "Read path");
	Res(sd_bus_message_enter_container(m, SD_BUS_TYPE_ARRAY, "{sa{sv}}"),
			"Enter array");

	while ((r = sd_bus_message_enter_container(m, SD_BUS_TYPE_DICT_ENTRY,
			"sa{sv}")) > 0)
	{
		const char *intf;
		Res(sd_bus_message_read_basic(m, SD_BUS_TYPE_STRING, &intf),
				"Read intf");


		if (strcmp(intf, "xyz.openbmc_project.Logging.Entry") == 0)
		{
			LogEntry(m, Id, Timestamp, Severity, Message, AdditionalData);
		}
		else
		{
			Res(sd_bus_message_skip(m, "a{sv}"), "Skip array");
		}
		Res(sd_bus_message_exit_container(m), "Exit dict"); // dict
	}

	Res(sd_bus_message_exit_container(m), "Exit array");
	if (r < 0)
	{
		printf("r=%s\n", strerror(-r));
	}
	return 0;
}

int SendMail(string SendTo, const char *program, string TxT)
{
	std::stringstream stream;
	stream << "echo -e '" << TxT.c_str() << "'|" << program << " " << SendTo;
	printf("Send mail\n");
	return system(stream.str().c_str());
}

int PostMail(const char *RunScript)
{
	string FullRun;
	FullRun = RunScript;
	FullRun = FullRun + " &";
	return system(FullRun.c_str());
}

static int callback(sd_bus_message *m, void *user, sd_bus_error *error)
{
	uint32_t Id;
	uint64_t Timestamp;
	const char *Severity;
	const char *Message;
	string AdditionalData = "";
	GetEventInfo(m, &Id, &Timestamp, &Severity, &Message, &AdditionalData);
	string TxT = GetMailText(FSendTo, Id, Timestamp, Severity, Message,
			AdditionalData);
	SendMail(FSendTo, FSendMail, TxT);
	if (FPostMail != NULL)
	{
		PostMail(FPostMail);
	}

//    callback_count++;
	return 0;
}

void RunMonitor()
{
	int r=system("dbus-monitor --system ""type='signal',path=/xyz/openbmc_project/logging,member=InterfacesAdded"" > /home/root/monitor &");
	if (r<0)
	{
		printf("fail run monitor");
	}
}

int BusListen(const char *SendTo, const char *SendMail, const char *PostMail)
{
	RunMonitor();
	//Settings
	FSendTo = strdup(SendTo);
	FSendMail = strdup(SendMail);
	FPostMail = strdup(PostMail);
	sd_bus *conn = NULL;
	sd_bus_slot *slot = NULL;
	static const size_t LEN = 256;
	// Action signal add  //oa{sa{sv}}
	const char match[LEN] =
			"type='signal',path=/xyz/openbmc_project/logging,member=InterfacesAdded";
	sd_event *loop = NULL;
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

		if (callback_error > 0)
		{
			goto finish;
		}

		r = sd_bus_wait(conn, (uint64_t) -1);
		if (r < 0)
		{
			fprintf(stderr, "Failed to wait on bus: %s\n", strerror(-r));
			goto finish;
		}
		if (callback_error != 0)
		{
			r = callback_error;
			goto finish;
		}
	}

	finish: sd_bus_slot_unref(slot);
	sd_bus_unref(conn);
	return r < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

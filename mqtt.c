
#define _CRT_SECURE_NO_DEPRECATE

#ifndef NOMQTT

#include "MQTTAsync.h"
#ifndef WIN32
#include <jansson.h>
#endif

#include "CHeaders.h"
#include "asmstrucs.h"
#include "mqtt.h"

extern int MQTT_Connecting;
extern int MQTT_Connected;


DllExport int APIENTRY MQTTSend(char * topic, char * Msg, int MsgLen);

MQTTAsync client = NULL;

time_t MQTTLastStatus = 0;

void MQTTSendStatus()
{
	char topic[256];
	char payload[128];
	
	sprintf(topic, "PACKETNODE/%s", NODECALLLOPPED);
	strcpy(payload,"{\"status\":\"online\"}");

	MQTTSend(topic, payload, strlen(payload));
	MQTTLastStatus = time(NULL);
}

void MQTTTimer()
{
	if (MQTT_Connecting == 0 && MQTT_Connected == 0)
		MQTTConnect(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);

	if ((time(NULL) - MQTTLastStatus) > 1800)
		MQTTSendStatus();

}


void MQTTDisconnect()
{
	if (MQTT_Connected)
	{
		MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;

		MQTTAsync_disconnect(client, &disc_opts);

		MQTT_Connecting = MQTT_Connected = 0;

		// Try to recconect. If it fails system will rety every minute

		MQTTConnect(MQTT_HOST, MQTT_PORT, MQTT_USER, MQTT_PASS);
	}
}

DllExport int APIENTRY MQTTSend(char * topic, char * Msg, int MsgLen)
{
	int rc;
	
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	pubmsg.payload = Msg;
	pubmsg.payloadlen = MsgLen;
	rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);

	if (rc)
		MQTTDisconnect();

	return rc;
}



void onConnect(void* context, MQTTAsync_successData* response)
{
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	MQTT_Connecting = 0;
	MQTT_Connected = 1;

	printf("Successful MQTT connection\n");

	// Send start up message

	MQTTSendStatus();

}

void onConnectFailure(void* context, MQTTAsync_failureData* response)
{
	printf("MQTT connection failed, rc %d\n", response ? response->code : 0);
	MQTT_Connecting = 0;
}



char* jsonEncodeMessage(MESSAGE *msg)
{
	char From[10];
	char To[10];
	
	char buffer[1024];
	unsigned long long SaveMMASK = MMASK;
	BOOL SaveMTX = MTX;
	BOOL SaveMCOM = MCOM;
	BOOL SaveMUI = MUIONLY;
	int len;
	char *msg_str;
	char payload_timestamp[16];

	struct tm * TM = localtime(&msg->Timestamp);
	sprintf(payload_timestamp, "%02d:%02d:%02d", TM->tm_hour, TM->tm_min, TM->tm_sec);

	
	IntSetTraceOptionsEx(MMASK, TRUE, TRUE, FALSE);
	From[ConvFromAX25(msg->ORIGIN, From)] = 0;
	To[ConvFromAX25(msg->DEST, To)] = 0;

	len = IntDecodeFrame(msg, buffer, msg->Timestamp, 0xffffffffffffffff, FALSE, FALSE);
	IntSetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

	buffer[len] = 0;

#ifdef WIN32

	msg_str = zalloc(2048);

	sprintf(msg_str, "{\"from\": \"%s\", \"to\": \"%s\", \"payload\": \"%s\", \"port\": %d, \"timestamp\": \"%s\"}",
		From, To, buffer, msg->PORT, payload_timestamp);

#else

	json_t *root;

	root = json_object();

	json_object_set_new(root, "from", json_string(From));
	json_object_set_new(root, "to", json_string(To));


	json_object_set_new(root, "payload", json_string(buffer));
	json_object_set_new(root, "port", json_integer(msg->PORT));
	sprintf(payload_timestamp, "%02d:%02d:%02d", TM->tm_hour, TM->tm_min, TM->tm_sec);
	json_object_set_new(root, "timestamp", json_string(payload_timestamp));
	msg_str = json_dumps(root, 0);
	json_decref(root);

#endif

	return msg_str;
}

void MQTTKISSTX(void *message)
{
	MESSAGE *msg = (MESSAGE *)message;
	char topic[256];
	char *msg_str;

	sprintf(topic, "PACKETNODE/ax25/trace/bpqformat/%s/sent/%d", NODECALLLOPPED, msg->PORT);

	msg_str = jsonEncodeMessage(msg);

	MQTTSend(topic, msg_str, strlen(msg_str));

	free(msg_str);
}

void MQTTKISSTX_RAW(char* buffer, int bufferLength, void* PORT) 
{
	PPORTCONTROL PPORT = (PPORTCONTROL)PORT;
	char topic[256];

	sprintf(topic, "PACKETNODE/kiss/%s/sent/%d", NODECALLLOPPED, PPORT->PORTNUMBER);

	MQTTSend(topic, buffer, bufferLength);
}


void MQTTKISSRX(void *message)
{
	MESSAGE *msg = (MESSAGE *)message;
	char topic[256];
	char *msg_str;


	sprintf(topic, "PACKETNODE/ax25/trace/bpqformat/%s/rcvd/%d", NODECALLLOPPED, msg->PORT);
	msg_str = jsonEncodeMessage(msg);

	MQTTSend(topic, msg_str, strlen(msg_str));
	
	free(msg_str);
}

void MQTTKISSRX_RAW(char* buffer, int bufferLength, void* PORT)
{
	PPORTCONTROL PPORT = (PPORTCONTROL)PORT;
	char topic[256];

	sprintf(topic, "PACKETNODE/kiss/%s/rcvd/%d", NODECALLLOPPED, PPORT->PORTNUMBER);

	MQTTSend(topic, buffer, bufferLength);

}

void MQTTReportSession(char * Msg)
{
	char topic[256];
	sprintf(topic, "PACKETNODE/stats/session/%s", NODECALLLOPPED);

	MQTTSend(topic, Msg, strlen(Msg));
}


char* replace(char* str, char* a, char* b)
{
	int len  = strlen(str);
	int lena = strlen(a), lenb = strlen(b);
	char * p;

	for (p = str; p = strstr(p, a); p) {
		if (lena != lenb) // shift end as needed
			memmove(p + lenb, p + lena,
				len - (p - str) + lenb);
		memcpy(p, b, lenb);
	}
	return str;
}

int MQTTPublish(void *message, char *topic)
{
	MESSAGE *msg = (MESSAGE *)message;
	char From[10];
	char To[10];
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

	unsigned long long SaveMMASK = MMASK;
	BOOL SaveMTX = MTX;
	BOOL SaveMCOM = MCOM;
	BOOL SaveMUI = MUIONLY;
	int len;
	char* replaced_buffer;
	char buffer[1024];

	time_t timestamp = msg->Timestamp;


	From[ConvFromAX25(msg->ORIGIN, From)] = 0;
	To[ConvFromAX25(msg->DEST, To)] = 0;


	IntSetTraceOptionsEx(8, TRUE, TRUE, FALSE);
	len = IntDecodeFrame(msg, buffer, timestamp, 1, FALSE, FALSE);
	IntSetTraceOptionsEx(SaveMMASK, SaveMTX, SaveMCOM, SaveMUI);

	// MQTT _really_ doesn't like \r, so replace it with something
	// that is at least human readable
	
	replaced_buffer = replace(buffer, "\r", "\r\n");

	pubmsg.payload = replaced_buffer;
	pubmsg.payloadlen = strlen(replaced_buffer);

	printf("%s\n", replaced_buffer);

	return MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}

int MQTTConnect(char* host, int port, char* user, char* pass)
{
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	int rc;
	char hostString[256];

	sprintf(hostString, "tcp://%s:%d", host, port);

	printf("MQTT Connect to %s\n", hostString);

	rc = MQTTAsync_create(&client, hostString, NODECALLLOPPED, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	if (rc != MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client, return code %d\n", rc);
		return rc;
	}

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.username = user;
	conn_opts.password = pass;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;
//	conn_opts.automaticReconnect = 1;
//	conn_opts.minRetryInterval = 30;
//	conn_opts.maxRetryInterval = 300;

	rc = MQTTAsync_connect(client, &conn_opts);

	if (rc != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		return rc;
	}

	MQTT_Connecting = 1;

	return 0;
}

/*
void MQTTMessageEvent(void* message)
{
	struct MsgInfo* msg = (struct MsgInfo *)message;
	char *msg_str;
	char * ptr;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	char topic[256];

	json_t *root = json_object();
	json_object_set_new(root, "id", json_integer(msg->number));
	json_object_set_new(root, "size", json_integer(msg->length));
	json_object_set_new(root, "type", json_string(msg->type == 'P' ? "P" : "B"));
	json_object_set_new(root, "to", json_string(msg->to));
	json_object_set_new(root, "from", json_string(msg->from));
	json_object_set_new(root, "subj", json_string(msg->title));

	switch(msg->status) {
		case 'N':
			json_object_set_new(root, "event", json_string("newmsg"));
			break;
		case 'F':
			json_object_set_new(root, "event", json_string("fwded"));
			break;
		case 'R':
			json_object_set_new(root, "event", json_string("read"));
			break;
		case 'K':
			json_object_set_new(root, "event", json_string("killed"));
			break;
	}

	msg_str = json_dumps(root, 0);

	pubmsg.payload = msg_str;
	pubmsg.payloadlen = strlen(msg_str);


	sprintf(topic, "PACKETNODE/event/%s/pmsg", NODECALLLOPPED);

	MQTTAsync_sendMessage(client, topic, &pubmsg, &opts);
}
*/

#else

// Dummies ofr build without MQTT libraries

int MQTTConnect(char* host, int port, char* user, char* pass)
{
	return 0;
}

void MQTTKISSTX(void *message) {};
void MQTTKISSTX_RAW(char* buffer, int bufferLength, void* PORT) {};
void MQTTKISSRX(void *message) {};
void MQTTKISSRX_RAW(char* buffer, int bufferLength, void* PORT) {};
void MQTTTimer() {};
void MQTTReportSession(char * Msg) {};

#endif


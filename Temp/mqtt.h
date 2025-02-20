int MQTTConnect(char* host, int port, char* user, char* pass);
int MQTTPublish(void * msg, char *topic);

void MQTTKISSTX(void *message);
void MQTTKISSTX_RAW(char* buffer, int bufferLength, void* PORT);
void MQTTKISSRX(void *message);
void MQTTKISSRX_RAW(char* buffer, int bufferLength, void* PORT);

void MQTTMessageEvent(void *message);


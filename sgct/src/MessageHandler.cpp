#include "sgct/MessageHandler.h"
#include <stdlib.h>
#include <stdio.h>

sgct::MessageHandler * sgct::MessageHandler::mInstance = NULL;

sgct::MessageHandler::MessageHandler(void)
{
}

void sgct::MessageHandler::decode(const char * receivedData, int receivedLenght, int clientIndex)
{
	fprintf(stderr, "Message: %s from client %d\n", receivedData, clientIndex);
}
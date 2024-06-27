#include "SMTPClient.h"

const char* SMTPClient::commandEHLO_GP = "EHLO GP\r\n";
const char* SMTPClient::loginCommand = "AUTH LOGIN\r\n";
const char* SMTPClient::data = "DATA\r\n";
const char* SMTPClient::dot = ".\r\n";
const char* SMTPClient::quit = "QUIT\r\n";

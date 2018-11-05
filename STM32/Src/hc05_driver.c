/*
 * hc05_driver.c
 *
 *  Created on: 03.11.2018
 *      Author: Kowalik
 */

#include "hc05_driver.h"
#include <string.h>
#include <stdlib.h>

static HC05Driver_Status_TypeDef HC05Driver_resetNormalMode(HC05Driver_TypeDef* pSelf);
static HC05Driver_Status_TypeDef HC05Driver_setATMode(HC05Driver_TypeDef* pSelf);
static HC05Driver_Status_TypeDef HC05Driver_setDataMode(HC05Driver_TypeDef* pSelf);

HC05Driver_State_TypeDef HC05Driver_init(HC05Driver_TypeDef* pSelf, HC05Driver_Role_TypeDef role, \
		UartDriver_TypeDef* pUartDriver, DigitalOutDriver_TypeDef* pKeyPinDriver, uint32_t baudRate,
		char* name, uint16_t password)
{

	if (pSelf->state != HC05Driver_State_UnInitialized){
		return HC05Driver_Status_Error;
	}

	if (role != HC05Driver_Role_Slave){
		return HC05Driver_Status_UnsuportedError;
	}

	HC05Driver_Status_TypeDef ret = HC05Driver_Status_OK;

	pSelf->state = HC05Driver_State_HardAT;
	pSelf->pUartDriver = pUartDriver;
	pSelf->pKeyPinDriver = pKeyPinDriver;
	pSelf->dataBaudRate = baudRate;

	memset(pSelf->buffer, 0, HC05_BUFFER_SIZE);

	uint32_t	tmpUartBaudRate;
	if (UartDriver_getBaudRate(pSelf->pUartDriver, &tmpUartBaudRate) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	if (tmpUartBaudRate != HC05_HARD_AT_BAUDRATE){
		if (UartDriver_setBaudRate(pSelf->pUartDriver, HC05_HARD_AT_BAUDRATE) != UartDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
	}

	if ((ret = HC05Driver_sendTestATCommand(pSelf)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_setDeviceName(pSelf, name)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_setBaudRate(pSelf, pSelf->dataBaudRate)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_setPassword(pSelf, password)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_setDeviceRole(pSelf, role)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_resetNormalMode(pSelf)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_sendTestATCommand(pSelf)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_setDataMode(pSelf)) != HC05Driver_Status_OK){
		return ret;
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_sendTestATCommand(HC05Driver_TypeDef* pSelf){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_sendRestoreDefualtCommand(HC05Driver_TypeDef* pSelf){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_RESTORE_ORGL_AT_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_getBaudRate(HC05Driver_TypeDef* pSelf, uint32_t* pRetBaudRate){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_GET_UART_AT_COMMNAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_GET_UART_AT_COMMNAND_RESPONSE, strlen(HC05_GET_UART_AT_COMMNAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, ",");
	*pRetBaudRate = atoi((char*)(pSelf->buffer + strlen(HC05_GET_UART_AT_COMMNAND_RESPONSE)));
	//TODO tutaj sprawdzic parity i bity stopu

	if (UartDriver_receiveBytes(pSelf->pUartDriver, pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_setBaudRate(HC05Driver_TypeDef* pSelf, uint32_t baudRate){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	uint32_t	currentBaudRate;
	if ((ret = HC05Driver_getBaudRate(pSelf, &currentBaudRate)) != HC05Driver_Status_OK){
		return ret;
	}

	if (baudRate != currentBaudRate){

							strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
		uint16_t length = 	strlcat((char*)pSelf->buffer, HC05_SET_UART_AT_COMMNAND, HC05_BUFFER_SIZE);
		length += 			sprintf ((char*)pSelf->buffer + length, "%lu", baudRate);
		length += 			sprintf ((char*)pSelf->buffer + length, ",%lu", HC05_STOP_BIT_SETUP);
		length += 			sprintf ((char*)pSelf->buffer + length, ",%lu", HC05_PARITY_SETUP);
							strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

		if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
		if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
			return HC05Driver_Status_Error;
		}
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_getPassword(HC05Driver_TypeDef* pSelf, uint32_t* pRetPassword){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_GET_PSWD_AT_COMMNAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_GET_PSWD_AT_COMMNAND_RESPONSE, strlen(HC05_GET_PSWD_AT_COMMNAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	*pRetPassword = atoi((char*)(pSelf->buffer + strlen(HC05_GET_PSWD_AT_COMMNAND_RESPONSE)));

	if (UartDriver_receiveBytes(pSelf->pUartDriver, pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_setPassword(HC05Driver_TypeDef* pSelf, uint32_t password){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	uint32_t	currentPassword;
	if ((ret = HC05Driver_getPassword(pSelf, &currentPassword)) != HC05Driver_Status_OK){
		return ret;
	}

	if (password != currentPassword){

							strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
		uint16_t length = 	strlcat((char*)pSelf->buffer, HC05_SET_PSWD_AT_COMMNAND, HC05_BUFFER_SIZE);
		length += 			sprintf ((char*)pSelf->buffer + length, "%lu", password);
							strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

		if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
		if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
			return HC05Driver_Status_Error;
		}
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_getDeviceName(HC05Driver_TypeDef* pSelf, char* pRetDeviceName){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_GET_NAME_AT_COMMNAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_GET_NAME_AT_COMMNAND_RESPONSE, strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	strncpy(pRetDeviceName, (char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_MAX_NAME_LENGTH);

	if (UartDriver_receiveBytes(pSelf->pUartDriver, pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_setDeviceName(HC05Driver_TypeDef* pSelf, char* deviceName){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	char currentName[HC05_MAX_NAME_LENGTH];
	if ((ret = HC05Driver_getDeviceName(pSelf, currentName)) != HC05Driver_Status_OK){
		return ret;
	}

	if (strcmp(currentName, deviceName) != 0){

		strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
		strlcat((char*)pSelf->buffer, HC05_SET_NAME_AT_COMMNAND, HC05_BUFFER_SIZE);
		strlcat((char*)pSelf->buffer, deviceName,  HC05_BUFFER_SIZE);
		strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

		if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
		if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
			return HC05Driver_Status_Error;
		}
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_getDeviceRole(HC05Driver_TypeDef* pSelf, HC05Driver_Role_TypeDef* pRetRole){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_GET_ROLE_AT_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_GET_ROLE_AT_COMMNAND_RESPONSE, strlen(HC05_GET_ROLE_AT_COMMNAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	*pRetRole = atoi((char*)(pSelf->buffer + strlen(HC05_GET_ROLE_AT_COMMNAND_RESPONSE)));

	if (UartDriver_receiveBytes(pSelf->pUartDriver, pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_setDeviceRole(HC05Driver_TypeDef* pSelf, HC05Driver_Role_TypeDef role){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	HC05Driver_Role_TypeDef	currentRole;
	if ((ret = HC05Driver_getDeviceRole(pSelf, &currentRole)) != HC05Driver_Status_OK){
		return ret;
	}

	if (role != currentRole){

							strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
		uint16_t length = 	strlcat((char*)pSelf->buffer, HC05_SET_PSWD_AT_COMMNAND, HC05_BUFFER_SIZE);
		length += 			sprintf ((char*)pSelf->buffer + length, "%du", role);
							strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

		if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
		if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
			return HC05Driver_Status_Error;
		}
	}

	return HC05Driver_Status_OK;

}

HC05Driver_Status_TypeDef HC05Driver_getState(HC05Driver_TypeDef* pSelf, HC05Driver_State_TypeDef* pRetState){

	if (pSelf->state == HC05Driver_State_UnInitialized || pSelf->state == HC05Driver_State_HardAT){
		*pRetState = pSelf->state;
		return HC05Driver_Status_OK;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setATMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_GET_STATE_AT_COMMNAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_GET_STATE_AT_COMMNAND_RESPONSE, strlen(HC05_GET_STATE_AT_COMMNAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_INITIALIZED_RESPONSE, strlen(HC05_INITIALIZED_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Initialized;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_READY_RESPONSE, strlen(HC05_READY_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Ready;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_PAIRABLE_RESPONSE, strlen(HC05_PAIRABLE_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Pairable;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_PAIRED_RESPONSE, strlen(HC05_PAIRED_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Paired;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_INQUIRING_RESPONSE, strlen(HC05_PAIRED_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Inquiring;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_CONNECTING_RESPONSE, strlen(HC05_CONNECTING_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Connecting;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_CONNECTED_RESPONSE, strlen(HC05_CONNECTED_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Connected;
	} else if (strncmp((char*)(pSelf->buffer+strlen(HC05_GET_NAME_AT_COMMNAND_RESPONSE)), HC05_DISCONNECTED_RESPONSE, strlen(HC05_DISCONNECTED_RESPONSE)) == 0){
		*pRetState = HC05Driver_State_Disconnected;
	} else {
		return HC05Driver_Status_Error;
	}

	if (UartDriver_receiveBytes(pSelf->pUartDriver, pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

HC05Driver_Status_TypeDef HC05Driver_sendData(HC05Driver_TypeDef* pSelf, uint8_t* data, uint16_t bytes){

	if (pSelf->state == HC05Driver_State_UnInitialized){
		return HC05Driver_Status_UnInitializedError;
	}

	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_setDataMode(pSelf)) != HC05Driver_Status_OK) {
		return ret;
	}


}

HC05Driver_Status_TypeDef HC05Driver_sendAndReceiveDataTerminationSign(HC05Driver_TypeDef* pSelf, uint8_t* pSendData, uint16_t bytesToSend, \
		uint8_t* pReceiveBuffer, uint16_t bufferSize, uint8_t terminationSign){

}

HC05Driver_Status_TypeDef HC05Driver_sendAndReceiveDataNBytes(HC05Driver_TypeDef* pSelf, uint8_t* pSendData, uint16_t bytesToSend, \
		uint8_t* pReceiveBuffer, uint16_t bytesToReceive){

}

static HC05Driver_Status_TypeDef HC05Driver_resetNormalMode(HC05Driver_TypeDef* pSelf){

	// assume module initialized
	HC05Driver_State_TypeDef state;
	HC05Driver_Status_TypeDef ret;

	if ((ret = HC05Driver_getState(pSelf, &state)) != HC05Driver_Status_OK){
		return ret;
	}

	if (state != HC05Driver_State_HardAT){
		return HC05Driver_Status_OK;
	}

	if (DigitalOutDriver_Off(pSelf->pKeyPinDriver) != DigitalOutDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strlcpy((char*)pSelf->buffer, HC05_AT_PREFIX_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_AT_RESET_COMMAND, HC05_BUFFER_SIZE);
	strlcat((char*)pSelf->buffer, HC05_COMMAND_TERMINATION, HC05_BUFFER_SIZE);

	if (UartDriver_sendAndReceive(pSelf->pUartDriver, pSelf->buffer, strlen((char*)pSelf->buffer), pSelf->buffer, HC05_BUFFER_SIZE, HC05_COMMAND_TRIM_SIGN) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	strtok ((char*)pSelf->buffer, "\r\n");

	if (strncmp((char*)pSelf->buffer, HC05_SET_OK_COMMAND_RESPONSE, strlen(HC05_SET_OK_COMMAND_RESPONSE)) != 0){
		return HC05Driver_Status_Error;
	}

	pSelf->state = HC05Driver_State_Data;

	uint32_t	tmpUartBaudRate;
	if (UartDriver_getBaudRate(pSelf->pUartDriver, &tmpUartBaudRate) != UartDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	if (tmpUartBaudRate != pSelf->dataBaudRate){
		if (UartDriver_setBaudRate(pSelf->pUartDriver, pSelf->dataBaudRate) != UartDriver_Status_OK){
			return ret;
		}
	}

	HAL_Delay(HC05_START_UP_DELAY_MS);

	if ((ret = HC05Driver_sendTestATCommand(pSelf)) != HC05Driver_Status_OK){
		return ret;
	}

	if ((ret = HC05Driver_getState(pSelf, &state)) != HC05Driver_Status_OK){
		return ret;
	}

	if (state == HC05Driver_State_HardAT){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}

static HC05Driver_Status_TypeDef HC05Driver_setATMode(HC05Driver_TypeDef* pSelf){

	if (pSelf->state != HC05Driver_State_HardAT){
		if (DigitalOutDriver_On(pSelf->pKeyPinDriver) != DigitalOutDriver_Status_OK){
			return HC05Driver_Status_Error;
		}
	}

	return HC05Driver_Status_OK;
}

static HC05Driver_Status_TypeDef HC05Driver_setDataMode(HC05Driver_TypeDef* pSelf){

	if (pSelf->state == HC05Driver_State_HardAT){
		return HC05Driver_resetNormalMode(pSelf);
	}

	if (DigitalOutDriver_Off(pSelf->pKeyPinDriver) != DigitalOutDriver_Status_OK){
		return HC05Driver_Status_Error;
	}

	return HC05Driver_Status_OK;
}
#include "coprocessor/simpleProtocol/simpleProtocol.h"
#include "coprocessor/cp_i2c.h"

// uint8_t* SProto_pktDistributeArr[2] = {0};
// uint8_t SProto_pktTxBuffer[256];
uint16_t SProto_bufferReadIndex = 0;
uint16_t SProto_bufferWriteIdx, SProto_bufferWriteIdx_old = 0;
uint8_t SProto_rxBuffer[SPROTO_RB_SIZE];

// uint16_t PktHandler_PktData(un_PktData *src, uint8_t *dst)
// {
// 	uint16_t numToTransmit = src->pack.VALIDNUM;
// 	uint16_t currentPkt = src->pack.CURRENT;
// 	uint16_t totalPkt = src->pack.TOTAL;
// 	uint16_t addrOffset = currentPkt * DATA_PKT_DATA_NUM;
// 	for (uint16_t i = 0; i < numToTransmit; i ++)
// 	{
// 		memcpy((uint8_t*)(dst + addrOffset), src->pack.PAYLOAD, numToTransmit);
// 	}

// 	return(totalPkt - currentPkt);
// }

// int8_t SProto_IsValidMsg()
// {
// 	uint16_t MsgRxLength;
// 	uint8_t CHK;

// 	SProto_bufferWriteIdx = DMA_CH0->ST & 0xFFFU;
// 	un_MsgHeader header;
// 	while (1) {
// 		if (SProto_bufferReadIndex == SProto_bufferWriteIdx) {
// 			return MSG_ERR_NO_HEADER;
// 		}

// 		// Find Header1
// 		while (SProto_bufferReadIndex != SProto_bufferWriteIdx &&
//            	SProto_rxBuffer[SProto_bufferReadIndex] != SPROTO_MSG_HEADER_SOM) {
// 				SProto_bufferReadIndex = RXRB_INDEX(SProto_bufferReadIndex, 1);
// 		}
// 		// Current SProto_bufferReadIndex is pointing SOM if noting wrong
// 		// Calculate remain data length
// 		if (SProto_bufferReadIndex < SProto_bufferWriteIdx) {
// 			MsgRxLength = SProto_bufferWriteIdx - SProto_bufferReadIndex;
// 		} else {
// 			MsgRxLength = (SProto_bufferWriteIdx + sizeof(SProto_rxBuffer)) - SProto_bufferReadIndex;
// 		}
// 		// Check valid Header length
// 		if (MsgRxLength < 6) {
//       		return MSG_ERR_HEADER_LEN_NOT_MATCH;
//     	}
// 		// Copy Header data
// 		for (int i = 0; i < 6; i ++)
// 		{
// 			header.array[i] = SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, i)];
// 		}
// 		// Check valid Msg length
// 		if (MsgRxLength < header.pack.SIZE) {
//       		return MSG_ERR_PACKET_LEN_NOT_MATCH;
//     	}
// 		if (SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, header.pack.SIZE - 1)] != SPROTO_MSG_PKT_EOP) {
// 			SProto_bufferReadIndex = SProto_bufferWriteIdx;	//Reset SProto_bufferReadIndex
//       		return MSG_ERR_NO_EOP;
//     	}

// 		// Checksum
// 		CHK = 0;
// 		for (uint16_t i = 0; i < header.pack.SIZE - 2; i ++) {
// 			CHK += SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, i)];
// 		}
// 		if (CHK != SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, header.pack.SIZE - 2)]) {
// 			SProto_bufferReadIndex = SProto_bufferWriteIdx;	//Reset SProto_bufferReadIndex
//       		return MSG_ERR_CHKSUM_UNMATCH;
// 		}

// 		// Transfer pkt to union
// 		uint8_t *packArr = SProto_pktDistributeArr[header.pack.TYPE];
// 		for (int i = 0; i < header.pack.SIZE; i ++)
// 		{
// 			packArr[i] = SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, i + 6)];
// 		}
// 		SProto_bufferReadIndex = SProto_bufferWriteIdx;	//Reset SProto_bufferReadIndex
// 		return header.pack.TYPE;
// 	}
// }

// int8_t SProto_TLEReader(char*merged, char**lines, uint16_t size)
// {
// 	uint16_t MsgRxLength;
// 	int8_t flag = 0;
// 	char tempBuffer[256] = {0}; // 临时缓冲区，用于存储原始数据
// 	int lineCount = 0;

// 	SProto_bufferWriteIdx = DMA_CH0->ST & 0xFFFU; // DMA指针
// 	if (SProto_bufferWriteIdx == SProto_bufferWriteIdx_old) return MSG_ERR_NO_HEADER;
// 	SProto_bufferWriteIdx_old = SProto_bufferWriteIdx;

// 	while (1) {
// 		// 查找TLE+头
// 		while (SProto_bufferReadIndex != SProto_bufferWriteIdx &&
// 				(SProto_rxBuffer[SProto_bufferReadIndex] != 'T' ||
// 				SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, 1)] != 'L' ||
// 				SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, 2)] != 'E' ||
// 				SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, 3)] != '+')) {
// 			SProto_bufferReadIndex = RXRB_INDEX(SProto_bufferReadIndex, 1);
// 		}

// 		if (SProto_bufferReadIndex == SProto_bufferWriteIdx) {
// 			return MSG_ERR_NO_HEADER;
// 		}

// 		// 计算消息长度
// 		if (SProto_bufferReadIndex < SProto_bufferWriteIdx) {
// 			MsgRxLength = SProto_bufferWriteIdx - SProto_bufferReadIndex;
// 		} else {
// 			MsgRxLength = (SProto_bufferWriteIdx + sizeof(SProto_rxBuffer)) - SProto_bufferReadIndex;
// 		}

// 		if (MsgRxLength < 10) {
// 			return MSG_ERR_HEADER_LEN_NOT_MATCH;
// 		}

// 		// 查找结束符'$'
// 		int endPos = -1;
// 		for (int i = 0; i < MsgRxLength; i++) {
// 			if (SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, i)] == '$') {
// 				endPos = i;
// 				break;
// 			}
// 		}

// 		if (endPos == -1) {
// 			return MSG_ERR_NO_EOP;
// 		}

// 		// 将数据复制到临时缓冲区
// 		int tempIdx = 0;
// 		for (int i = 4; i < endPos; i++) { // 跳过"TLE+"
// 			tempBuffer[tempIdx++] = SProto_rxBuffer[RXRB_INDEX(SProto_bufferReadIndex, i)];
// 			if (tempIdx >= sizeof(tempBuffer) - 1) break; // 防止溢出
// 		}
// 		tempBuffer[tempIdx] = '\0';

// 		// 解析数据行
// 		memset(lines, 0, 71*5);
// 		char*line = strtok(tempBuffer, "\r\n");
// 		while (line != NULL && lineCount < 5) {
// 			lines[lineCount++] = line;
// 			line = strtok(NULL, "\r\n");
// 		}

// 		// 检查是否有足够的数据行
// 		if (lineCount != 5) {
// 			return MSG_ERR_DATA_FORMAT;
// 		}

//         // 手动构建输出字符串，避免使用sprintf
//         memset(merged, 0, size);
//         uint16_t pos = 0;

//         // 添加“TLE+”前缀
//         if (pos + 4 < size) {
//             merged[pos++] = 'T';
//             merged[pos++] = 'L';
//             merged[pos++] = 'E';
//             merged[pos++] = '+';
//         } else {
//             return MSG_ERR_BUFFER_OVERFLOW;
//         }

//         // 添加每一行数据和分隔符
//         for (int i = 0; i < lineCount; i++) {
//             char*src = lines[i];
//             // 复制当前行
//             while (*src && pos < size - 1) {
//                 merged[pos++] =*src++;
//             }

//             // 添加分隔符'$'
//             if (pos < size - 1) {
//                 merged[pos++] = '$';
//             } else {
//                 return MSG_ERR_BUFFER_OVERFLOW;
//             }
//         }

//         // 确保字符串以null结尾
//         if (pos > 0 && pos < size) {
//             merged[pos] = '\0';
//         }

// 		// 更新读取指针
// 		SProto_bufferReadIndex = RXRB_INDEX(SProto_bufferReadIndex, endPos + 1);

// 		return 0; // 成功
// 	}
// }

// void SProto_PktSend_RPRT(uint8_t rprtValue)
// {
// 	un_MsgHeader header;
// 	un_PktRprt pkt;
// 	header.pack.TYPE = SPROTO_MSG_TYPE_RPRT;
// 	header.pack.SIZE = 10;
// 	header.pack.SOM = SPROTO_MSG_HEADER_SOM;
// 	pkt.pack.SOP = SPROTO_MSG_PKT_SOP;
// 	pkt.pack.RPRT = rprtValue;
// 	pkt.pack.EOP = SPROTO_MSG_PKT_EOP;
// 	pkt.pack.CHK = 0;
// 	for (int i = 0; i < 6; i ++)
// 	{
// 		pkt.pack.CHK += header.array[i];
// 	}
// 	for (int i = 0; i < 2; i ++)
// 	{
// 		pkt.pack.CHK += pkt.array[i];
// 	}
// 	memcpy(SProto_pktTxBuffer, header.array, 6);
// 	memcpy(&SProto_pktTxBuffer[6], pkt.array, 4);
// 	UART_TRANSMIT(SProto_pktTxBuffer, 10);
// }

// void SProto_PktConstruct_DATA(uint8_t *srcStart, uint8_t *dst, uint16_t total, uint16_t current, uint16_t valid)
// {
// 	un_MsgHeader header;
// 	un_PktData pkt;
// 	header.pack.TYPE = SPROTO_MSG_TYPE_DATA;
// 	header.pack.SIZE = DATA_PKT_DATA_NUM + 16;
// 	header.pack.SOM = SPROTO_MSG_HEADER_SOM;
// 	pkt.pack.SOP = SPROTO_MSG_PKT_SOP;
// 	pkt.pack.TOTAL = total;
// 	pkt.pack.CURRENT = current;
// 	pkt.pack.VALIDNUM = valid;
// 	pkt.pack.EOP = SPROTO_MSG_PKT_EOP;

// 	uint16_t currentPtrOffset = current * DATA_PKT_DATA_NUM;
// 	memcpy(pkt.pack.PAYLOAD, &srcStart[currentPtrOffset], valid);

// 	pkt.pack.CHK = 0x00;
// 	for (int i = 0; i < 6; i ++)
// 	{
// 		pkt.pack.CHK += (uint8_t)header.array[i];
// 	}
// 	for (int i = 0; i < DATA_PKT_DATA_NUM + 8; i ++)
// 	{
// 		pkt.pack.CHK += (uint8_t)pkt.array[i];
// 	}

// 	memcpy(dst, header.array, 6);
// 	memcpy(&dst[6], pkt.array, DATA_PKT_DATA_NUM + 10);
// }

// void SProto_PktSend_DATA(uint8_t *srcStart, uint32_t len)
// {
// 	uint16_t currentSubPack = 0;
// 	uint16_t totalSubPack = len / DATA_PKT_DATA_NUM;
// 	if (len % DATA_PKT_DATA_NUM == 0) totalSubPack -= 1;
// 	uint16_t validLen = len - (currentSubPack * DATA_PKT_DATA_NUM);
// 	validLen = validLen > DATA_PKT_DATA_NUM ? DATA_PKT_DATA_NUM : validLen;
	
// 	for (currentSubPack = 0; currentSubPack < totalSubPack; currentSubPack ++)
// 	{
// 		SProto_PktConstruct_DATA(srcStart, SProto_pktTxBuffer, totalSubPack, currentSubPack, validLen);
// 		UART_TRANSMIT(SProto_pktTxBuffer, DATA_PKT_DATA_NUM + 16);
// 	}
// }

void SProto_UART_to_I2C_Passthrough(void)
{
    uint8_t dataLength;
    uint16_t i2cAddress = 0x0081;  // I2C设备寄存器地址
    uint8_t tmp[256];

    // 获取DMA当前写入位置
    SProto_bufferWriteIdx = DMA_CH0->ST & 0xFFFU;

    // 检查是否有新数据
    if (SProto_bufferWriteIdx == SProto_bufferReadIndex) 
	{
        return 0;  // 无新数据
    }
	while (SProto_bufferReadIndex != SProto_bufferWriteIdx)
	{
        tmp[dataLength] = SProto_rxBuffer[SProto_bufferReadIndex];
        SProto_bufferReadIndex = RXRB_INDEX(SProto_bufferReadIndex, 1);
        dataLength ++;
	}
    CP_I2C_Write(i2cAddress, tmp, dataLength);

    return dataLength;  // 返回成功传输的字节数
}
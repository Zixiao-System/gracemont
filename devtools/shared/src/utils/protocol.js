/**
 * Modbus 功能码
 */
export const MODBUS_FUNCTIONS = {
  READ_COILS: 0x01,
  READ_DISCRETE_INPUTS: 0x02,
  READ_HOLDING_REGISTERS: 0x03,
  READ_INPUT_REGISTERS: 0x04,
  WRITE_SINGLE_COIL: 0x05,
  WRITE_SINGLE_REGISTER: 0x06,
  WRITE_MULTIPLE_COILS: 0x0F,
  WRITE_MULTIPLE_REGISTERS: 0x10
}

/**
 * Modbus 异常码
 */
export const MODBUS_EXCEPTIONS = {
  0x01: '非法功能',
  0x02: '非法数据地址',
  0x03: '非法数据值',
  0x04: '从站设备故障',
  0x05: '确认',
  0x06: '从站设备忙',
  0x08: '内存奇偶校验错误',
  0x0A: '网关路径不可用',
  0x0B: '网关目标设备响应失败'
}

/**
 * 解析 Modbus 异常
 * @param {number} code - 异常码
 * @returns {string}
 */
export function parseModbusException(code) {
  return MODBUS_EXCEPTIONS[code] || `未知异常 (0x${code.toString(16)})`
}

/**
 * 构建 Modbus TCP 帧
 * @param {number} transactionId - 事务标识
 * @param {number} unitId - 单元标识
 * @param {number} functionCode - 功能码
 * @param {Uint8Array} data - 数据
 * @returns {Uint8Array}
 */
export function buildModbusTCPFrame(transactionId, unitId, functionCode, data) {
  const length = 2 + data.length // unitId + functionCode + data
  const frame = new Uint8Array(7 + data.length + 1)

  // MBAP Header
  frame[0] = (transactionId >> 8) & 0xFF
  frame[1] = transactionId & 0xFF
  frame[2] = 0 // Protocol ID (Modbus)
  frame[3] = 0
  frame[4] = (length >> 8) & 0xFF
  frame[5] = length & 0xFF
  frame[6] = unitId

  // PDU
  frame[7] = functionCode
  frame.set(data, 8)

  return frame
}

/**
 * 构建 Modbus RTU 帧
 * @param {number} address - 从站地址
 * @param {number} functionCode - 功能码
 * @param {Uint8Array} data - 数据
 * @returns {Uint8Array}
 */
export function buildModbusRTUFrame(address, functionCode, data) {
  const frame = new Uint8Array(4 + data.length)

  frame[0] = address
  frame[1] = functionCode
  frame.set(data, 2)

  // 计算 CRC16 (Modbus)
  const crc = calculateModbusCRC(frame.subarray(0, frame.length - 2))
  frame[frame.length - 2] = crc & 0xFF
  frame[frame.length - 1] = (crc >> 8) & 0xFF

  return frame
}

/**
 * 计算 Modbus CRC16
 * @param {Uint8Array} data - 数据
 * @returns {number}
 */
export function calculateModbusCRC(data) {
  let crc = 0xFFFF

  for (let i = 0; i < data.length; i++) {
    crc ^= data[i]
    for (let j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc = (crc >> 1) ^ 0xA001
      } else {
        crc >>= 1
      }
    }
  }

  return crc
}

/**
 * OPC UA 数据类型
 */
export const OPCUA_DATATYPES = {
  Boolean: 1,
  SByte: 2,
  Byte: 3,
  Int16: 4,
  UInt16: 5,
  Int32: 6,
  UInt32: 7,
  Int64: 8,
  UInt64: 9,
  Float: 10,
  Double: 11,
  String: 12,
  DateTime: 13,
  ByteString: 15
}

/**
 * OPC UA 状态码
 */
export const OPCUA_STATUS_CODES = {
  0x00000000: 'Good',
  0x80000000: 'Bad',
  0x40000000: 'Uncertain',
  0x80010000: 'BadUnexpectedError',
  0x80020000: 'BadInternalError',
  0x80030000: 'BadOutOfMemory',
  0x80040000: 'BadResourceUnavailable',
  0x80050000: 'BadCommunicationError',
  0x80060000: 'BadEncodingError',
  0x80070000: 'BadDecodingError',
  0x80080000: 'BadEncodingLimitsExceeded',
  0x80090000: 'BadRequestTooLarge',
  0x800A0000: 'BadResponseTooLarge',
  0x800B0000: 'BadUnknownResponse',
  0x800C0000: 'BadTimeout',
  0x800D0000: 'BadServiceUnsupported',
  0x800E0000: 'BadShutdown',
  0x800F0000: 'BadServerNotConnected',
  0x80100000: 'BadServerHalted'
}

/**
 * 解析 OPC UA 状态码
 * @param {number} code - 状态码
 * @returns {string}
 */
export function parseOpcuaStatusCode(code) {
  // 检查精确匹配
  if (OPCUA_STATUS_CODES[code]) {
    return OPCUA_STATUS_CODES[code]
  }

  // 检��状态类别
  if ((code & 0x80000000) === 0) return 'Good'
  if ((code & 0xC0000000) === 0x40000000) return 'Uncertain'
  if ((code & 0xC0000000) === 0x80000000) return 'Bad'

  return `Unknown (0x${code.toString(16)})`
}

/**
 * 解析 OPC UA NodeId 字符串
 * @param {string} nodeIdStr - NodeId 字符串
 * @returns {Object}
 */
export function parseNodeId(nodeIdStr) {
  // 格式: ns=<namespaceIndex>;<identifierType>=<identifier>
  const parts = nodeIdStr.split(';')
  const result = { namespaceIndex: 0, identifierType: 's', identifier: '' }

  for (const part of parts) {
    const [key, value] = part.split('=')
    if (key === 'ns') {
      result.namespaceIndex = parseInt(value)
    } else if (['i', 's', 'g', 'b'].includes(key)) {
      result.identifierType = key
      result.identifier = key === 'i' ? parseInt(value) : value
    }
  }

  return result
}

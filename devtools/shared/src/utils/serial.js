/**
 * 串口配置选项
 */
export const BAUD_RATES = [
  300, 600, 1200, 2400, 4800, 9600, 14400, 19200,
  28800, 38400, 57600, 115200, 230400, 460800, 921600
]

export const DATA_BITS = [5, 6, 7, 8]

export const PARITY_OPTIONS = [
  { value: 'none', label: '无' },
  { value: 'odd', label: '奇校验' },
  { value: 'even', label: '偶校验' },
  { value: 'mark', label: '标记' },
  { value: 'space', label: '空格' }
]

export const STOP_BITS = [1, 1.5, 2]

export const FLOW_CONTROL = [
  { value: 'none', label: '无' },
  { value: 'xon/xoff', label: 'XON/XOFF' },
  { value: 'rts/cts', label: 'RTS/CTS' }
]

/**
 * 默认串口配置
 */
export const DEFAULT_SERIAL_CONFIG = {
  baudRate: 115200,
  dataBits: 8,
  parity: 'none',
  stopBits: 1,
  flowControl: 'none'
}

/**
 * 计算校验和
 * @param {Uint8Array} data - 数据
 * @param {string} type - 校验类型 'sum' | 'xor' | 'crc16'
 * @returns {number}
 */
export function calculateChecksum(data, type = 'sum') {
  switch (type) {
    case 'sum':
      return data.reduce((a, b) => (a + b) & 0xFF, 0)

    case 'xor':
      return data.reduce((a, b) => a ^ b, 0)

    case 'crc16':
      return calculateCRC16(data)

    default:
      throw new Error(`Unknown checksum type: ${type}`)
  }
}

/**
 * 计算 CRC16 (CCITT)
 * @param {Uint8Array} data - 数据
 * @returns {number}
 */
export function calculateCRC16(data) {
  let crc = 0xFFFF

  for (let i = 0; i < data.length; i++) {
    crc ^= data[i] << 8

    for (let j = 0; j < 8; j++) {
      if (crc & 0x8000) {
        crc = (crc << 1) ^ 0x1021
      } else {
        crc <<= 1
      }
      crc &= 0xFFFF
    }
  }

  return crc
}

/**
 * 计算 CRC32
 * @param {Uint8Array} data - 数据
 * @returns {number}
 */
export function calculateCRC32(data) {
  let crc = 0xFFFFFFFF

  for (let i = 0; i < data.length; i++) {
    crc ^= data[i]
    for (let j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >>> 1) ^ 0xEDB88320
      } else {
        crc >>>= 1
      }
    }
  }

  return (crc ^ 0xFFFFFFFF) >>> 0
}

/**
 * 转义特殊字符
 * @param {Uint8Array} data - 数据
 * @param {Object} escapeMap - 转义映射 { char: escaped }
 * @returns {Uint8Array}
 */
export function escapeData(data, escapeMap = { 0x7E: [0x7D, 0x5E], 0x7D: [0x7D, 0x5D] }) {
  const result = []

  for (const byte of data) {
    if (escapeMap[byte]) {
      result.push(...escapeMap[byte])
    } else {
      result.push(byte)
    }
  }

  return new Uint8Array(result)
}

/**
 * 反转义特殊字符
 * @param {Uint8Array} data - 数据
 * @returns {Uint8Array}
 */
export function unescapeData(data) {
  const result = []
  let i = 0

  while (i < data.length) {
    if (data[i] === 0x7D && i + 1 < data.length) {
      result.push(data[i + 1] ^ 0x20)
      i += 2
    } else {
      result.push(data[i])
      i++
    }
  }

  return new Uint8Array(result)
}

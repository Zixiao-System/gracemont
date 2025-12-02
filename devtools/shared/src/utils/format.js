/**
 * 格式化字节大小
 * @param {number} bytes - 字节数
 * @param {number} decimals - 小数位数
 * @returns {string}
 */
export function formatBytes(bytes, decimals = 2) {
  if (bytes === 0) return '0 B'

  const k = 1024
  const dm = decimals < 0 ? 0 : decimals
  const sizes = ['B', 'KB', 'MB', 'GB', 'TB']

  const i = Math.floor(Math.log(bytes) / Math.log(k))

  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i]
}

/**
 * 格式化十六进制
 * @param {number} value - 数值
 * @param {number} digits - 位数
 * @returns {string}
 */
export function formatHex(value, digits = 2) {
  return value.toString(16).toUpperCase().padStart(digits, '0')
}

/**
 * 格式化二进制
 * @param {number} value - 数值
 * @param {number} digits - 位数
 * @returns {string}
 */
export function formatBinary(value, digits = 8) {
  return value.toString(2).padStart(digits, '0')
}

/**
 * 格式化时间戳
 * @param {Date|number} date - 日期或时间戳
 * @param {string} format - 格式
 * @returns {string}
 */
export function formatTime(date, format = 'HH:mm:ss') {
  const d = date instanceof Date ? date : new Date(date)

  const pad = (n) => n.toString().padStart(2, '0')

  const tokens = {
    'YYYY': d.getFullYear(),
    'MM': pad(d.getMonth() + 1),
    'DD': pad(d.getDate()),
    'HH': pad(d.getHours()),
    'mm': pad(d.getMinutes()),
    'ss': pad(d.getSeconds()),
    'SSS': d.getMilliseconds().toString().padStart(3, '0')
  }

  let result = format
  for (const [token, value] of Object.entries(tokens)) {
    result = result.replace(token, value)
  }

  return result
}

/**
 * 格式化持续时间
 * @param {number} ms - 毫秒数
 * @returns {string}
 */
export function formatDuration(ms) {
  if (ms < 1000) return `${ms}ms`
  if (ms < 60000) return `${(ms / 1000).toFixed(1)}s`
  if (ms < 3600000) {
    const m = Math.floor(ms / 60000)
    const s = Math.floor((ms % 60000) / 1000)
    return `${m}m ${s}s`
  }
  const h = Math.floor(ms / 3600000)
  const m = Math.floor((ms % 3600000) / 60000)
  return `${h}h ${m}m`
}

/**
 * 格式化速度
 * @param {number} bytesPerSecond - 每秒字节数
 * @returns {string}
 */
export function formatSpeed(bytesPerSecond) {
  return formatBytes(bytesPerSecond) + '/s'
}

/**
 * 格式化地址
 * @param {number} address - 地址
 * @param {number} bits - 位数 (32 或 64)
 * @returns {string}
 */
export function formatAddress(address, bits = 32) {
  const digits = bits / 4
  return '0x' + formatHex(address, digits)
}

/**
 * 解析十六进制字符串
 * @param {string} hexString - 十六进制字符串
 * @returns {Uint8Array}
 */
export function parseHexString(hexString) {
  const hex = hexString.replace(/\s/g, '')
  const bytes = new Uint8Array(hex.length / 2)

  for (let i = 0; i < bytes.length; i++) {
    bytes[i] = parseInt(hex.substr(i * 2, 2), 16)
  }

  return bytes
}

/**
 * 转换为十六进制字符串
 * @param {Uint8Array} bytes - 字节数组
 * @param {string} separator - 分隔符
 * @returns {string}
 */
export function toHexString(bytes, separator = ' ') {
  return Array.from(bytes)
    .map(b => formatHex(b, 2))
    .join(separator)
}

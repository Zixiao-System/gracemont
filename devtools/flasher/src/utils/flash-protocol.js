/**
 * 固件烧录协议
 * 简单的命令-响应协议
 */

// 命令定义
export const CMD = {
  ERASE: 0x01,      // 擦除 Flash
  WRITE: 0x02,      // 写入数据
  VERIFY: 0x03,     // 校验数据
  RESET: 0x04,      // 复位设备
  INFO: 0x05        // 获取设备信息
}

// 响应码
export const RESP = {
  OK: 0x00,
  ERROR: 0xFF,
  BUSY: 0xFE
}

export class FlashProtocol {
  constructor(serialAPI) {
    this.serialAPI = serialAPI
    this.responseBuffer = []
    this.responseCallback = null
    this.timeout = null

    // 监听串口数据
    this.serialAPI.onSerialData((data) => {
      this.handleData(data)
    })
  }

  /**
   * 处理接收到的数据
   */
  handleData(data) {
    this.responseBuffer.push(...data)

    if (this.responseCallback) {
      // 简单协议：响应格式 [CMD, STATUS, ...DATA]
      if (this.responseBuffer.length >= 2) {
        const response = {
          cmd: this.responseBuffer[0],
          status: this.responseBuffer[1],
          data: this.responseBuffer.slice(2)
        }

        clearTimeout(this.timeout)
        this.responseCallback(response)
        this.responseCallback = null
        this.responseBuffer = []
      }
    }
  }

  /**
   * 发送命令并等待响应
   */
  async sendCommand(cmd, data = [], timeoutMs = 5000) {
    return new Promise((resolve, reject) => {
      // 构建数据包: [CMD, LEN, ...DATA]
      const packet = [cmd, data.length, ...data]

      this.responseCallback = resolve
      this.timeout = setTimeout(() => {
        this.responseCallback = null
        reject(new Error('Command timeout'))
      }, timeoutMs)

      // 发送数据
      this.serialAPI.writePort(new Uint8Array(packet))
        .catch(reject)
    })
  }

  /**
   * 获取设备信息
   */
  async getDeviceInfo() {
    const response = await this.sendCommand(CMD.INFO)

    if (response.status !== RESP.OK) {
      throw new Error('Failed to get device info')
    }

    return {
      chipId: (response.data[0] << 24) | (response.data[1] << 16) |
              (response.data[2] << 8) | response.data[3],
      flashSize: (response.data[4] << 24) | (response.data[5] << 16) |
                 (response.data[6] << 8) | response.data[7],
      ramSize: (response.data[8] << 24) | (response.data[9] << 16) |
               (response.data[10] << 8) | response.data[11]
    }
  }

  /**
   * 擦除 Flash
   */
  async eraseFlash(address, size) {
    const addrBytes = [
      (address >> 24) & 0xFF,
      (address >> 16) & 0xFF,
      (address >> 8) & 0xFF,
      address & 0xFF
    ]

    const sizeBytes = [
      (size >> 24) & 0xFF,
      (size >> 16) & 0xFF,
      (size >> 8) & 0xFF,
      size & 0xFF
    ]

    const response = await this.sendCommand(CMD.ERASE, [...addrBytes, ...sizeBytes], 30000)

    if (response.status !== RESP.OK) {
      throw new Error('Flash erase failed')
    }
  }

  /**
   * 写入数据块
   */
  async writeBlock(address, data, maxChunkSize = 256) {
    let offset = 0

    while (offset < data.length) {
      const chunkSize = Math.min(maxChunkSize, data.length - offset)
      const chunk = data.slice(offset, offset + chunkSize)

      const addrBytes = [
        (address >> 24) & 0xFF,
        (address >> 16) & 0xFF,
        (address >> 8) & 0xFF,
        address & 0xFF
      ]

      const payload = [...addrBytes, ...chunk]
      const response = await this.sendCommand(CMD.WRITE, payload, 10000)

      if (response.status !== RESP.OK) {
        throw new Error(`Write failed at address 0x${address.toString(16)}`)
      }

      offset += chunkSize
      address += chunkSize
    }
  }

  /**
   * 校验数据
   */
  async verifyBlock(address, size) {
    const addrBytes = [
      (address >> 24) & 0xFF,
      (address >> 16) & 0xFF,
      (address >> 8) & 0xFF,
      address & 0xFF
    ]

    const sizeBytes = [
      (size >> 24) & 0xFF,
      (size >> 16) & 0xFF,
      (size >> 8) & 0xFF,
      size & 0xFF
    ]

    const response = await this.sendCommand(CMD.VERIFY, [...addrBytes, ...sizeBytes], 10000)

    if (response.status !== RESP.OK) {
      throw new Error('Verification failed')
    }

    // 返回 CRC32
    return (response.data[0] << 24) | (response.data[1] << 16) |
           (response.data[2] << 8) | response.data[3]
  }

  /**
   * 复位设备
   */
  async resetDevice() {
    const response = await this.sendCommand(CMD.RESET)

    if (response.status !== RESP.OK) {
      throw new Error('Device reset failed')
    }
  }

  /**
   * 清理
   */
  destroy() {
    if (this.timeout) {
      clearTimeout(this.timeout)
    }
    this.serialAPI.removeSerialDataListener()
  }
}

import crypto from 'crypto-js'

/**
 * 固件文件解��器
 * 支持 BIN, ELF, HEX 格式
 */
export class FirmwareParser {
  /**
   * 解析固件文件
   * @param {Buffer} buffer - 文件数据
   * @param {string} filename - 文件名
   * @returns {Object} 解析结果
   */
  static parse(buffer, filename) {
    const ext = filename.split('.').pop().toLowerCase()

    switch (ext) {
      case 'bin':
        return this.parseBIN(buffer)
      case 'elf':
        return this.parseELF(buffer)
      case 'hex':
        return this.parseHEX(buffer)
      default:
        throw new Error(`Unsupported file type: ${ext}`)
    }
  }

  /**
   * 解析 BIN 文件（原始二进制）
   */
  static parseBIN(buffer) {
    const baseAddr = 0x40000000 // ARM64 RAM 起始地址

    return {
      type: 'bin',
      size: buffer.length,
      checksum: this.calculateCRC32(buffer),
      segments: [
        {
          address: baseAddr,
          size: buffer.length,
          data: buffer
        }
      ]
    }
  }

  /**
   * 解析 ELF 文件
   */
  static parseELF(buffer) {
    // ELF 魔数检查
    if (buffer[0] !== 0x7F || buffer[1] !== 0x45 ||
        buffer[2] !== 0x4C || buffer[3] !== 0x46) {
      throw new Error('Invalid ELF file')
    }

    const is64bit = buffer[4] === 2
    const isLittleEndian = buffer[5] === 1

    if (!is64bit) {
      throw new Error('Only 64-bit ELF files are supported')
    }

    // 读取 ELF header
    const view = new DataView(buffer.buffer)
    const e_phoff = Number(view.getBigUint64(32, isLittleEndian)) // Program header offset
    const e_phentsize = view.getUint16(54, isLittleEndian) // Program header entry size
    const e_phnum = view.getUint16(56, isLittleEndian) // Number of program headers

    const segments = []
    let totalSize = 0

    // 解析 Program Headers
    for (let i = 0; i < e_phnum; i++) {
      const phOffset = e_phoff + (i * e_phentsize)
      const p_type = view.getUint32(phOffset, isLittleEndian)

      // PT_LOAD = 1 (可加载段)
      if (p_type === 1) {
        const p_offset = Number(view.getBigUint64(phOffset + 8, isLittleEndian))
        const p_vaddr = Number(view.getBigUint64(phOffset + 16, isLittleEndian))
        const p_filesz = Number(view.getBigUint64(phOffset + 32, isLittleEndian))

        if (p_filesz > 0) {
          const data = buffer.slice(p_offset, p_offset + p_filesz)
          segments.push({
            address: p_vaddr,
            size: p_filesz,
            data: data
          })
          totalSize += p_filesz
        }
      }
    }

    return {
      type: 'elf',
      size: totalSize,
      checksum: this.calculateCRC32(buffer),
      segments: segments
    }
  }

  /**
   * 解析 Intel HEX 文件
   */
  static parseHEX(buffer) {
    const text = new TextDecoder().decode(buffer)
    const lines = text.split('\n').filter(line => line.trim().startsWith(':'))

    const data = new Map() // address => bytes
    let extendedAddr = 0

    for (const line of lines) {
      const byteCount = parseInt(line.slice(1, 3), 16)
      const address = parseInt(line.slice(3, 7), 16)
      const recordType = parseInt(line.slice(7, 9), 16)
      const dataBytes = line.slice(9, 9 + byteCount * 2)

      switch (recordType) {
        case 0x00: // Data record
          {
            const fullAddr = extendedAddr + address
            const bytes = []
            for (let i = 0; i < dataBytes.length; i += 2) {
              bytes.push(parseInt(dataBytes.slice(i, i + 2), 16))
            }
            data.set(fullAddr, bytes)
          }
          break

        case 0x02: // Extended Segment Address
          {
            const segment = parseInt(dataBytes, 16)
            extendedAddr = segment * 16
          }
          break

        case 0x04: // Extended Linear Address
          {
            const extended = parseInt(dataBytes, 16)
            extendedAddr = extended << 16
          }
          break

        case 0x01: // End of File
          break

        default:
          console.warn(`Unknown HEX record type: 0x${recordType.toString(16).padStart(2, '0')}`)
      }
    }

    // 合并连续地址的数据
    const sortedAddrs = Array.from(data.keys()).sort((a, b) => a - b)
    const segments = []
    let currentSegment = null

    for (const addr of sortedAddrs) {
      const bytes = data.get(addr)

      if (!currentSegment || addr !== currentSegment.address + currentSegment.data.length) {
        // 开始新段
        if (currentSegment) {
          segments.push(currentSegment)
        }
        currentSegment = {
          address: addr,
          size: bytes.length,
          data: new Uint8Array(bytes)
        }
      } else {
        // 扩展当前段
        const newData = new Uint8Array(currentSegment.data.length + bytes.length)
        newData.set(currentSegment.data)
        newData.set(bytes, currentSegment.data.length)
        currentSegment.data = newData
        currentSegment.size = newData.length
      }
    }

    if (currentSegment) {
      segments.push(currentSegment)
    }

    const totalSize = segments.reduce((sum, seg) => sum + seg.size, 0)

    return {
      type: 'hex',
      size: totalSize,
      checksum: this.calculateCRC32(buffer),
      segments: segments
    }
  }

  /**
   * 计算 CRC32 校验和
   */
  static calculateCRC32(buffer) {
    const wordArray = this.bufferToWordArray(buffer)
    const hash = crypto.MD5(wordArray)
    return hash.toString().toUpperCase().slice(0, 8)
  }

  /**
   * Buffer 转 CryptoJS WordArray
   */
  static bufferToWordArray(buffer) {
    const words = []
    for (let i = 0; i < buffer.length; i += 4) {
      words.push(
        (buffer[i] << 24) |
        (buffer[i + 1] << 16) |
        (buffer[i + 2] << 8) |
        (buffer[i + 3] || 0)
    )
    }
    return crypto.lib.WordArray.create(words, buffer.length)
  }
}

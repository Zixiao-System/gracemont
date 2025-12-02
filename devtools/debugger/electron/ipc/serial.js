const { ipcMain } = require('electron')
const { SerialPort } = require('serialport')

let serialPort = null

// 列出串口
ipcMain.handle('serial:list', async () => {
  try {
    const ports = await SerialPort.list()
    return { success: true, ports }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 打开串口
ipcMain.handle('serial:open', async (event, config) => {
  try {
    if (serialPort && serialPort.isOpen) {
      serialPort.close()
    }

    serialPort = new SerialPort({
      path: config.port,
      baudRate: config.baudRate || 115200,
      dataBits: config.dataBits || 8,
      parity: config.parity || 'none',
      stopBits: config.stopBits || 1
    })

    // 监听数据
    serialPort.on('data', (data) => {
      event.sender.send('serial:data', {
        type: 'rx',
        data: data.toString(),
        raw: Array.from(data)
      })
    })

    serialPort.on('error', (err) => {
      event.sender.send('serial:data', {
        type: 'error',
        data: err.message
      })
    })

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 关闭串口
ipcMain.handle('serial:close', async () => {
  try {
    if (serialPort && serialPort.isOpen) {
      serialPort.close()
      serialPort = null
    }
    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 写入数据
ipcMain.handle('serial:write', async (event, data) => {
  try {
    if (!serialPort || !serialPort.isOpen) {
      return { success: false, error: 'Serial port not open' }
    }

    // 支持十六进制字符串
    let buffer
    if (data.hex) {
      buffer = Buffer.from(data.hex.replace(/\s/g, ''), 'hex')
    } else {
      buffer = Buffer.from(data.text || data)
    }

    serialPort.write(buffer)

    event.sender.send('serial:data', {
      type: 'tx',
      data: buffer.toString(),
      raw: Array.from(buffer)
    })

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

const { SerialPort } = require('serialport')

let port = null
let mainWindow = null

function setupSerialHandlers(ipcMain) {
  // 获取可用串口列表
  ipcMain.handle('serial:list', async () => {
    try {
      const ports = await SerialPort.list()
      return {
        success: true,
        ports: ports.map(p => ({
          path: p.path,
          manufacturer: p.manufacturer,
          serialNumber: p.serialNumber,
          pnpId: p.pnpId,
          vendorId: p.vendorId,
          productId: p.productId
        }))
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 打开串口
  ipcMain.handle('serial:open', async (event, path, options = {}) => {
    mainWindow = event.sender

    try {
      // 关闭已打开的端口
      if (port && port.isOpen) {
        await new Promise((resolve) => port.close(resolve))
      }

      const baudRate = options.baudRate || 115200
      const dataBits = options.dataBits || 8
      const stopBits = options.stopBits || 1
      const parity = options.parity || 'none'

      port = new SerialPort({
        path,
        baudRate,
        dataBits,
        stopBits,
        parity,
        autoOpen: false
      })

      // 打开端口
      await new Promise((resolve, reject) => {
        port.open((err) => {
          if (err) reject(err)
          else resolve()
        })
      })

      // 监听数据
      port.on('data', (data) => {
        mainWindow.send('serial:data', data)
      })

      // 监听错误
      port.on('error', (error) => {
        mainWindow.send('serial:error', error.message)
      })

      return {
        success: true,
        info: {
          path,
          baudRate,
          dataBits,
          stopBits,
          parity
        }
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 关闭串口
  ipcMain.handle('serial:close', async () => {
    try {
      if (port && port.isOpen) {
        await new Promise((resolve) => port.close(resolve))
      }
      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 写入数据
  ipcMain.handle('serial:write', async (_event, data) => {
    try {
      if (!port || !port.isOpen) {
        throw new Error('Serial port is not open')
      }

      const buffer = Buffer.from(data)
      await new Promise((resolve, reject) => {
        port.write(buffer, (err) => {
          if (err) reject(err)
          else resolve()
        })
      })

      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })
}

module.exports = { setupSerialHandlers }

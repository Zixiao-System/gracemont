const { dialog } = require('electron')
const fs = require('fs').promises

function setupFileHandlers(ipcMain) {
  // 打开文件对话框
  ipcMain.handle('file:open', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openFile'],
      filters: [
        { name: 'Firmware Files', extensions: ['bin', 'elf', 'hex'] },
        { name: 'All Files', extensions: ['*'] }
      ]
    })

    if (result.canceled) {
      return null
    }

    const filePath = result.filePaths[0]
    const stats = await fs.stat(filePath)

    return {
      path: filePath,
      name: filePath.split('/').pop(),
      size: stats.size
    }
  })

  // 读取文件内容
  ipcMain.handle('file:read', async (_event, filePath) => {
    try {
      const buffer = await fs.readFile(filePath)
      return {
        success: true,
        data: buffer,
        size: buffer.length
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 保存文件
  ipcMain.handle('file:save', async (_event, filePath, data) => {
    try {
      await fs.writeFile(filePath, data)
      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })
}

module.exports = { setupFileHandlers }

const { dialog } = require('electron')
const fs = require('fs').promises
const path = require('path')

function setupFileHandlers(ipcMain) {
  // 打开文件对话框
  ipcMain.handle('file:open', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openFile'],
      filters: [
        { name: 'PLC Files', extensions: ['st', 'ld', 'fbd', 'sfc'] },
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
      name: path.basename(filePath),
      size: stats.size,
      ext: path.extname(filePath)
    }
  })

  // 读取文件内容
  ipcMain.handle('file:read', async (_event, filePath) => {
    try {
      const content = await fs.readFile(filePath, 'utf-8')
      return {
        success: true,
        content: content
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 保存文件
  ipcMain.handle('file:save', async (_event, filePath, content) => {
    try {
      await fs.writeFile(filePath, content, 'utf-8')
      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 读取目录
  ipcMain.handle('file:readDir', async (_event, dirPath) => {
    try {
      const entries = await fs.readdir(dirPath, { withFileTypes: true })
      const items = await Promise.all(
        entries.map(async (entry) => {
          const fullPath = path.join(dirPath, entry.name)
          const stats = await fs.stat(fullPath)
          return {
            name: entry.name,
            path: fullPath,
            isDirectory: entry.isDirectory(),
            size: stats.size,
            modified: stats.mtime
          }
        })
      )
      return {
        success: true,
        items: items
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 创建文件
  ipcMain.handle('file:create', async (_event, filePath, content = '') => {
    try {
      await fs.writeFile(filePath, content, 'utf-8')
      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 删除文件
  ipcMain.handle('file:delete', async (_event, filePath) => {
    try {
      await fs.unlink(filePath)
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

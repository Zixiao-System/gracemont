const { app, BrowserWindow, ipcMain } = require('electron')
const path = require('path')
const { setupFileHandlers } = require('./ipc/file')
const { setupProcessHandlers } = require('./ipc/process')
const { setupProjectHandlers } = require('./ipc/project')

let mainWindow

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1000,
    minHeight: 700,
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      nodeIntegration: false,
      contextIsolation: true,
    },
    titleBarStyle: 'hidden',
    titleBarOverlay: {
      color: '#1f1f1f',
      symbolColor: '#ffffff'
    }
  })

  // 开发环境加载 Vite 服务器，生产环境加载打包文件
  if (process.env.NODE_ENV === 'development') {
    mainWindow.loadURL('http://localhost:5176')
    mainWindow.webContents.openDevTools()
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'))
  }

  mainWindow.on('closed', () => {
    mainWindow = null
  })
}

app.whenReady().then(() => {
  createWindow()

  // 设置 IPC 处理器
  setupFileHandlers(ipcMain)
  setupProcessHandlers(ipcMain)
  setupProjectHandlers(ipcMain)

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) {
      createWindow()
    }
  })
})

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

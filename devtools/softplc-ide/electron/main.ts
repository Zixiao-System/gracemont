import { app, BrowserWindow, ipcMain, Menu } from 'electron'
import { join } from 'path'

let mainWindow: BrowserWindow | null = null

function createWindow() {
  mainWindow = new BrowserWindow({
    width: 1400,
    height: 900,
    minWidth: 1024,
    minHeight: 768,
    webPreferences: {
      preload: join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration: false
    },
    titleBarStyle: 'hiddenInset',
    frame: process.platform === 'darwin'
  })

  // 开发环境加载 vite 服务器
  if (process.env.NODE_ENV === 'development') {
    mainWindow.loadURL('http://localhost:5173')
    mainWindow.webContents.openDevTools()
  } else {
    mainWindow.loadFile(join(__dirname, '../dist/index.html'))
  }

  createMenu()
}

function createMenu() {
  const template: Electron.MenuItemConstructorOptions[] = [
    {
      label: '文件',
      submenu: [
        { label: '新建项目', accelerator: 'CmdOrCtrl+N', click: () => mainWindow?.webContents.send('menu:new-project') },
        { label: '打开项目', accelerator: 'CmdOrCtrl+O', click: () => mainWindow?.webContents.send('menu:open-project') },
        { type: 'separator' },
        { label: '保存', accelerator: 'CmdOrCtrl+S', click: () => mainWindow?.webContents.send('menu:save') },
        { label: '全部保存', accelerator: 'CmdOrCtrl+Shift+S', click: () => mainWindow?.webContents.send('menu:save-all') },
        { type: 'separator' },
        { label: '导出...', click: () => mainWindow?.webContents.send('menu:export') },
        { type: 'separator' },
        { label: '退出', accelerator: 'CmdOrCtrl+Q', role: 'quit' }
      ]
    },
    {
      label: '编辑',
      submenu: [
        { label: '撤销', accelerator: 'CmdOrCtrl+Z', role: 'undo' },
        { label: '重做', accelerator: 'CmdOrCtrl+Y', role: 'redo' },
        { type: 'separator' },
        { label: '剪切', accelerator: 'CmdOrCtrl+X', role: 'cut' },
        { label: '复制', accelerator: 'CmdOrCtrl+C', role: 'copy' },
        { label: '粘贴', accelerator: 'CmdOrCtrl+V', role: 'paste' },
        { type: 'separator' },
        { label: '查找', accelerator: 'CmdOrCtrl+F', click: () => mainWindow?.webContents.send('menu:find') },
        { label: '替换', accelerator: 'CmdOrCtrl+H', click: () => mainWindow?.webContents.send('menu:replace') }
      ]
    },
    {
      label: 'PLC',
      submenu: [
        { label: '编译', accelerator: 'F7', click: () => mainWindow?.webContents.send('menu:compile') },
        { label: '全部编译', accelerator: 'Ctrl+F7', click: () => mainWindow?.webContents.send('menu:compile-all') },
        { type: 'separator' },
        { label: '下载', accelerator: 'F8', click: () => mainWindow?.webContents.send('menu:download') },
        { label: '上载', click: () => mainWindow?.webContents.send('menu:upload') },
        { type: 'separator' },
        { label: '运行', accelerator: 'F5', click: () => mainWindow?.webContents.send('menu:run') },
        { label: '停止', accelerator: 'Shift+F5', click: () => mainWindow?.webContents.send('menu:stop') },
        { type: 'separator' },
        { label: '在线监控', accelerator: 'F6', click: () => mainWindow?.webContents.send('menu:monitor') }
      ]
    },
    {
      label: '视图',
      submenu: [
        { label: '项目浏览器', accelerator: 'Ctrl+1', click: () => mainWindow?.webContents.send('menu:view-project') },
        { label: '变量表', accelerator: 'Ctrl+2', click: () => mainWindow?.webContents.send('menu:view-variables') },
        { label: '交叉引用', accelerator: 'Ctrl+3', click: () => mainWindow?.webContents.send('menu:view-crossref') },
        { type: 'separator' },
        { label: '输出窗口', accelerator: 'Ctrl+`', click: () => mainWindow?.webContents.send('menu:view-output') },
        { type: 'separator' },
        { label: '全屏', accelerator: 'F11', role: 'togglefullscreen' },
        { label: '开发者工具', accelerator: 'F12', role: 'toggleDevTools' }
      ]
    },
    {
      label: '帮助',
      submenu: [
        { label: 'IEC 61131-3 参考', click: () => mainWindow?.webContents.send('menu:help-iec') },
        { label: '指令集参考', click: () => mainWindow?.webContents.send('menu:help-instructions') },
        { type: 'separator' },
        { label: '关于', click: () => mainWindow?.webContents.send('menu:about') }
      ]
    }
  ]

  const menu = Menu.buildFromTemplate(template)
  Menu.setApplicationMenu(menu)
}

app.whenReady().then(() => {
  createWindow()

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

// IPC 处理
ipcMain.handle('get-app-path', () => app.getAppPath())

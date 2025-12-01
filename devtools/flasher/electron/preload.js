const { contextBridge, ipcRenderer } = require('electron')

// 暴露安全的 API 到渲染进程
contextBridge.exposeInMainWorld('electronAPI', {
  // 文件操作
  openFile: () => ipcRenderer.invoke('file:open'),
  saveFile: (path, data) => ipcRenderer.invoke('file:save', path, data),
  readFile: (path) => ipcRenderer.invoke('file:read', path),

  // 串口操作
  listPorts: () => ipcRenderer.invoke('serial:list'),
  openPort: (path, options) => ipcRenderer.invoke('serial:open', path, options),
  closePort: () => ipcRenderer.invoke('serial:close'),
  writePort: (data) => ipcRenderer.invoke('serial:write', data),

  // 串口数据监听
  onSerialData: (callback) => {
    ipcRenderer.on('serial:data', (_event, data) => callback(data))
  },
  onSerialError: (callback) => {
    ipcRenderer.on('serial:error', (_event, error) => callback(error))
  },

  // 移除监听器
  removeSerialDataListener: () => {
    ipcRenderer.removeAllListeners('serial:data')
  },
  removeSerialErrorListener: () => {
    ipcRenderer.removeAllListeners('serial:error')
  }
})

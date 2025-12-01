const { contextBridge, ipcRenderer } = require('electron')

// 暴露安全的 API 到渲染进程
contextBridge.exposeInMainWorld('electronAPI', {
  // 文件操作
  openFile: () => ipcRenderer.invoke('file:open'),
  saveFile: (path, data) => ipcRenderer.invoke('file:save', path, data),
  readFile: (path) => ipcRenderer.invoke('file:read', path),
  readDir: (path) => ipcRenderer.invoke('file:readDir', path),
  createFile: (path, content) => ipcRenderer.invoke('file:create', path, content),
  deleteFile: (path) => ipcRenderer.invoke('file:delete', path),
  watchFile: (path) => ipcRenderer.invoke('file:watch', path),

  // 项目操作
  createProject: (path, config) => ipcRenderer.invoke('project:create', path, config),
  openProject: () => ipcRenderer.invoke('project:open'),
  saveProject: (path, data) => ipcRenderer.invoke('project:save', path, data),
  getProjectFiles: (path) => ipcRenderer.invoke('project:getFiles', path),

  // 进程管理（编译、构建）
  spawn: (cmd, args, options) => ipcRenderer.invoke('process:spawn', cmd, args, options),
  kill: (pid) => ipcRenderer.invoke('process:kill', pid),

  // 进程输出监听
  onProcessOutput: (callback) => {
    ipcRenderer.on('process:output', (_event, data) => callback(data))
  },
  onProcessError: (callback) => {
    ipcRenderer.on('process:error', (_event, error) => callback(error))
  },
  onProcessExit: (callback) => {
    ipcRenderer.on('process:exit', (_event, code) => callback(code))
  },

  // 文件变化监听
  onFileChange: (callback) => {
    ipcRenderer.on('file:change', (_event, data) => callback(data))
  },

  // 移除监听器
  removeProcessListeners: () => {
    ipcRenderer.removeAllListeners('process:output')
    ipcRenderer.removeAllListeners('process:error')
    ipcRenderer.removeAllListeners('process:exit')
  },
  removeFileListeners: () => {
    ipcRenderer.removeAllListeners('file:change')
  }
})

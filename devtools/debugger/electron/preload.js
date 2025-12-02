const { contextBridge, ipcRenderer } = require('electron')

// 暴露安全的 API 到渲染进程
contextBridge.exposeInMainWorld('electronAPI', {
  // GDB 控制
  gdbStart: (config) => ipcRenderer.invoke('gdb:start', config),
  gdbStop: () => ipcRenderer.invoke('gdb:stop'),
  gdbCommand: (cmd) => ipcRenderer.invoke('gdb:command', cmd),
  gdbContinue: () => ipcRenderer.invoke('gdb:continue'),
  gdbStep: () => ipcRenderer.invoke('gdb:step'),
  gdbStepOver: () => ipcRenderer.invoke('gdb:step-over'),
  gdbStepOut: () => ipcRenderer.invoke('gdb:step-out'),
  gdbPause: () => ipcRenderer.invoke('gdb:pause'),
  onGDBOutput: (callback) => ipcRenderer.on('gdb:output', (_, data) => callback(data)),
  onGDBStateChange: (callback) => ipcRenderer.on('gdb:state', (_, data) => callback(data)),
  onGDBBreakpoint: (callback) => ipcRenderer.on('gdb:breakpoint-hit', (_, data) => callback(data)),

  // 断点管理
  addBreakpoint: (bp) => ipcRenderer.invoke('breakpoint:add', bp),
  removeBreakpoint: (id) => ipcRenderer.invoke('breakpoint:remove', id),
  enableBreakpoint: (id, enabled) => ipcRenderer.invoke('breakpoint:enable', { id, enabled }),
  listBreakpoints: () => ipcRenderer.invoke('breakpoint:list'),

  // 变量监视
  getVariables: () => ipcRenderer.invoke('gdb:variables'),
  getCallStack: () => ipcRenderer.invoke('gdb:callstack'),
  getRegisters: () => ipcRenderer.invoke('gdb:registers'),
  evaluateExpression: (expr) => ipcRenderer.invoke('gdb:evaluate', expr),

  // 串口调试
  serialList: () => ipcRenderer.invoke('serial:list'),
  serialOpen: (config) => ipcRenderer.invoke('serial:open', config),
  serialClose: () => ipcRenderer.invoke('serial:close'),
  serialWrite: (data) => ipcRenderer.invoke('serial:write', data),
  onSerialData: (callback) => ipcRenderer.on('serial:data', (_, data) => callback(data)),

  // 移除所有监听器
  removeAllListeners: () => {
    ipcRenderer.removeAllListeners('gdb:output')
    ipcRenderer.removeAllListeners('gdb:state')
    ipcRenderer.removeAllListeners('gdb:breakpoint-hit')
    ipcRenderer.removeAllListeners('serial:data')
  }
})

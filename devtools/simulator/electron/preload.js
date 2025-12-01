const { contextBridge, ipcRenderer } = require('electron')

// 暴露安全的 API 到渲染进程
contextBridge.exposeInMainWorld('electronAPI', {
  // QEMU 控制
  startQEMU: (config) => ipcRenderer.invoke('qemu:start', config),
  stopQEMU: () => ipcRenderer.invoke('qemu:stop'),
  getQEMUStatus: () => ipcRenderer.invoke('qemu:status'),
  onQEMUOutput: (callback) => ipcRenderer.on('qemu:output', (_, data) => callback(data)),
  onQEMUStateChange: (callback) => ipcRenderer.on('qemu:state', (_, data) => callback(data)),

  // I/O 操作
  setDigitalOutput: (pin, value) => ipcRenderer.invoke('io:digital-set', { pin, value }),
  setAnalogOutput: (channel, value) => ipcRenderer.invoke('io:analog-set', { channel, value }),
  getDigitalInput: (pin) => ipcRenderer.invoke('io:digital-get', pin),
  getAnalogInput: (channel) => ipcRenderer.invoke('io:analog-get', channel),
  onIOChange: (callback) => ipcRenderer.on('io:change', (_, data) => callback(data)),

  // 协议客户端
  modbusConnect: (config) => ipcRenderer.invoke('modbus:connect', config),
  modbusDisconnect: () => ipcRenderer.invoke('modbus:disconnect'),
  modbusRead: (request) => ipcRenderer.invoke('modbus:read', request),
  modbusWrite: (request) => ipcRenderer.invoke('modbus:write', request),

  opcuaConnect: (config) => ipcRenderer.invoke('opcua:connect', config),
  opcuaDisconnect: () => ipcRenderer.invoke('opcua:disconnect'),
  opcuaRead: (nodeId) => ipcRenderer.invoke('opcua:read', nodeId),
  opcuaWrite: (nodeId, value) => ipcRenderer.invoke('opcua:write', { nodeId, value }),
  onOpcuaDataChange: (callback) => ipcRenderer.on('opcua:data-change', (_, data) => callback(data)),

  // 移除所有监听器
  removeAllListeners: () => {
    ipcRenderer.removeAllListeners('qemu:output')
    ipcRenderer.removeAllListeners('qemu:state')
    ipcRenderer.removeAllListeners('io:change')
    ipcRenderer.removeAllListeners('opcua:data-change')
  }
})

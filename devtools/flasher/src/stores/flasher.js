import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useFlasherStore = defineStore('flasher', () => {
  // 固件文件信息
  const firmwareFile = ref(null)
  const firmwareData = ref(null)
  const fileInfo = ref({
    name: '',
    size: 0,
    checksum: '',
    type: '' // 'bin', 'elf', 'hex'
  })

  // 串口信息
  const ports = ref([])
  const selectedPort = ref(null)
  const portConfig = ref({
    baudRate: 115200,
    dataBits: 8,
    stopBits: 1,
    parity: 'none'
  })
  const portOpen = ref(false)

  // 烧录状态
  const flashing = ref(false)
  const progress = ref(0)
  const speed = ref(0) // KB/s
  const bytesWritten = ref(0)
  const startTime = ref(null)

  // 日志
  const logs = ref([])

  // Computed
  const remainingTime = computed(() => {
    if (!flashing.value || speed.value === 0) return 0
    const remaining = fileInfo.value.size - bytesWritten.value
    return Math.ceil(remaining / (speed.value * 1024))
  })

  const progressPercent = computed(() => {
    if (!fileInfo.value.size) return 0
    return Math.floor((bytesWritten.value / fileInfo.value.size) * 100)
  })

  // Actions
  function addLog(message, type = 'info') {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    logs.value.push({
      time: timestamp,
      message,
      type // 'info', 'success', 'error', 'warning'
    })
  }

  function clearLogs() {
    logs.value = []
  }

  function setFirmwareFile(file) {
    firmwareFile.value = file
  }

  function setFirmwareData(data) {
    firmwareData.value = data
  }

  function setFileInfo(info) {
    fileInfo.value = { ...fileInfo.value, ...info }
  }

  function setPorts(portList) {
    ports.value = portList
  }

  function setSelectedPort(port) {
    selectedPort.value = port
  }

  function setPortConfig(config) {
    portConfig.value = { ...portConfig.value, ...config }
  }

  function setPortOpen(isOpen) {
    portOpen.value = isOpen
  }

  function startFlashing() {
    flashing.value = true
    progress.value = 0
    bytesWritten.value = 0
    startTime.value = Date.now()
    addLog('开始烧录...', 'info')
  }

  function updateProgress(bytes) {
    bytesWritten.value = bytes
    const elapsed = (Date.now() - startTime.value) / 1000 // seconds
    speed.value = elapsed > 0 ? (bytes / 1024) / elapsed : 0
  }

  function stopFlashing(success = false) {
    flashing.value = false
    if (success) {
      addLog('烧录完成！', 'success')
    } else {
      addLog('烧录已停止', 'warning')
    }
  }

  function reset() {
    firmwareFile.value = null
    firmwareData.value = null
    fileInfo.value = {
      name: '',
      size: 0,
      checksum: '',
      type: ''
    }
    flashing.value = false
    progress.value = 0
    speed.value = 0
    bytesWritten.value = 0
    startTime.value = null
  }

  return {
    // State
    firmwareFile,
    firmwareData,
    fileInfo,
    ports,
    selectedPort,
    portConfig,
    portOpen,
    flashing,
    progress,
    speed,
    bytesWritten,
    logs,

    // Computed
    remainingTime,
    progressPercent,

    // Actions
    addLog,
    clearLogs,
    setFirmwareFile,
    setFirmwareData,
    setFileInfo,
    setPorts,
    setSelectedPort,
    setPortConfig,
    setPortOpen,
    startFlashing,
    updateProgress,
    stopFlashing,
    reset
  }
})

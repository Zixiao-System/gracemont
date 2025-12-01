import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useSimulatorStore = defineStore('simulator', () => {
  // QEMU 状态
  const qemuRunning = ref(false)
  const qemuConfig = ref(null)
  const qemuOutput = ref([])

  // I/O 状态
  const digitalInputs = ref(Array(32).fill(false))
  const digitalOutputs = ref(Array(32).fill(false))
  const analogInputs = ref(Array(16).fill(0))
  const analogOutputs = ref(Array(8).fill(0))

  // 协议状态
  const modbusConnected = ref(false)
  const modbusConfig = ref(null)
  const opcuaConnected = ref(false)
  const opcuaConfig = ref(null)

  // 数据日志
  const dataLog = ref([])
  const maxLogSize = 1000

  // UI 状态
  const activeTab = ref('io') // 'io', 'modbus', 'opcua', 'console'
  const ioViewMode = ref('visual') // 'visual', 'table'

  // Computed
  const hasQEMU = computed(() => qemuRunning.value)
  const hasModbus = computed(() => modbusConnected.value)
  const hasOpcua = computed(() => opcuaConnected.value)

  // Actions - QEMU
  function setQEMURunning(running) {
    qemuRunning.value = running
  }

  function setQEMUConfig(config) {
    qemuConfig.value = config
  }

  function addQEMUOutput(output) {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    qemuOutput.value.push({
      time: timestamp,
      type: output.type,
      data: output.data
    })

    // 限制输出大小
    if (qemuOutput.value.length > maxLogSize) {
      qemuOutput.value.shift()
    }
  }

  function clearQEMUOutput() {
    qemuOutput.value = []
  }

  // Actions - I/O
  function setDigitalInput(pin, value) {
    if (pin >= 0 && pin < digitalInputs.value.length) {
      digitalInputs.value[pin] = Boolean(value)
    }
  }

  function setDigitalOutput(pin, value) {
    if (pin >= 0 && pin < digitalOutputs.value.length) {
      digitalOutputs.value[pin] = Boolean(value)
    }
  }

  function setAnalogInput(channel, value) {
    if (channel >= 0 && channel < analogInputs.value.length) {
      analogInputs.value[channel] = Math.max(0, Math.min(4095, value))
    }
  }

  function setAnalogOutput(channel, value) {
    if (channel >= 0 && channel < analogOutputs.value.length) {
      analogOutputs.value[channel] = Math.max(0, Math.min(4095, value))
    }
  }

  function toggleDigitalInput(pin) {
    if (pin >= 0 && pin < digitalInputs.value.length) {
      digitalInputs.value[pin] = !digitalInputs.value[pin]
    }
  }

  // Actions - 协议
  function setModbusConnected(connected) {
    modbusConnected.value = connected
  }

  function setModbusConfig(config) {
    modbusConfig.value = config
  }

  function setOpcuaConnected(connected) {
    opcuaConnected.value = connected
  }

  function setOpcuaConfig(config) {
    opcuaConfig.value = config
  }

  // Actions - 数据日志
  function addDataLog(entry) {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    dataLog.value.push({
      time: timestamp,
      ...entry
    })

    // 限制日志大小
    if (dataLog.value.length > maxLogSize) {
      dataLog.value.shift()
    }
  }

  function clearDataLog() {
    dataLog.value = []
  }

  // Actions - UI
  function setActiveTab(tab) {
    activeTab.value = tab
  }

  function setIOViewMode(mode) {
    ioViewMode.value = mode
  }

  return {
    // State
    qemuRunning,
    qemuConfig,
    qemuOutput,
    digitalInputs,
    digitalOutputs,
    analogInputs,
    analogOutputs,
    modbusConnected,
    modbusConfig,
    opcuaConnected,
    opcuaConfig,
    dataLog,
    activeTab,
    ioViewMode,

    // Computed
    hasQEMU,
    hasModbus,
    hasOpcua,

    // Actions
    setQEMURunning,
    setQEMUConfig,
    addQEMUOutput,
    clearQEMUOutput,
    setDigitalInput,
    setDigitalOutput,
    setAnalogInput,
    setAnalogOutput,
    toggleDigitalInput,
    setModbusConnected,
    setModbusConfig,
    setOpcuaConnected,
    setOpcuaConfig,
    addDataLog,
    clearDataLog,
    setActiveTab,
    setIOViewMode
  }
})

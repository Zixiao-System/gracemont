import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useDebuggerStore = defineStore('debugger', () => {
  // GDB 状态
  const gdbRunning = ref(false)
  const gdbConnected = ref(false)
  const gdbPaused = ref(true)
  const currentFile = ref(null)
  const currentLine = ref(null)

  // 断点
  const breakpoints = ref([])

  // 变量和调用栈
  const variables = ref([])
  const callStack = ref([])
  const registers = ref([])
  const watchExpressions = ref([])

  // GDB 输出
  const gdbOutput = ref([])
  const maxOutputSize = 1000

  // 串口状态
  const serialConnected = ref(false)
  const serialConfig = ref(null)
  const serialData = ref([])

  // UI 状态
  const activePanel = ref('variables') // 'variables', 'callstack', 'breakpoints', 'registers', 'watch'
  const activeBottomTab = ref('console') // 'console', 'serial'

  // Computed
  const hasGDB = computed(() => gdbRunning.value)
  const canContinue = computed(() => gdbRunning.value && gdbPaused.value)
  const canPause = computed(() => gdbRunning.value && !gdbPaused.value)

  // Actions - GDB 状态
  function setGDBState(state) {
    gdbRunning.value = state.running ?? gdbRunning.value
    gdbConnected.value = state.connected ?? gdbConnected.value
    gdbPaused.value = state.paused ?? gdbPaused.value
    currentFile.value = state.currentFile ?? currentFile.value
    currentLine.value = state.currentLine ?? currentLine.value
  }

  function addGDBOutput(output) {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    gdbOutput.value.push({
      time: timestamp,
      type: output.type,
      data: output.data
    })

    // 限制输出大小
    if (gdbOutput.value.length > maxOutputSize) {
      gdbOutput.value.shift()
    }
  }

  function clearGDBOutput() {
    gdbOutput.value = []
  }

  // Actions - 断点
  function setBreakpoints(bps) {
    breakpoints.value = bps
  }

  function addBreakpoint(bp) {
    breakpoints.value.push(bp)
  }

  function removeBreakpoint(id) {
    const idx = breakpoints.value.findIndex(bp => bp.id === id)
    if (idx !== -1) {
      breakpoints.value.splice(idx, 1)
    }
  }

  function toggleBreakpoint(id) {
    const bp = breakpoints.value.find(bp => bp.id === id)
    if (bp) {
      bp.enabled = !bp.enabled
    }
  }

  // Actions - 调试信息
  function setVariables(vars) {
    variables.value = vars
  }

  function setCallStack(stack) {
    callStack.value = stack
  }

  function setRegisters(regs) {
    registers.value = regs
  }

  // Actions - 监视表达式
  function addWatchExpression(expr) {
    watchExpressions.value.push({
      id: Date.now(),
      expression: expr,
      value: null
    })
  }

  function removeWatchExpression(id) {
    const idx = watchExpressions.value.findIndex(w => w.id === id)
    if (idx !== -1) {
      watchExpressions.value.splice(idx, 1)
    }
  }

  function updateWatchValue(id, value) {
    const watch = watchExpressions.value.find(w => w.id === id)
    if (watch) {
      watch.value = value
    }
  }

  // Actions - 串口
  function setSerialConnected(connected) {
    serialConnected.value = connected
  }

  function setSerialConfig(config) {
    serialConfig.value = config
  }

  function addSerialData(data) {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    serialData.value.push({
      time: timestamp,
      ...data
    })

    // 限制数据大小
    if (serialData.value.length > maxOutputSize) {
      serialData.value.shift()
    }
  }

  function clearSerialData() {
    serialData.value = []
  }

  // Actions - UI
  function setActivePanel(panel) {
    activePanel.value = panel
  }

  function setActiveBottomTab(tab) {
    activeBottomTab.value = tab
  }

  return {
    // State
    gdbRunning,
    gdbConnected,
    gdbPaused,
    currentFile,
    currentLine,
    breakpoints,
    variables,
    callStack,
    registers,
    watchExpressions,
    gdbOutput,
    serialConnected,
    serialConfig,
    serialData,
    activePanel,
    activeBottomTab,

    // Computed
    hasGDB,
    canContinue,
    canPause,

    // Actions
    setGDBState,
    addGDBOutput,
    clearGDBOutput,
    setBreakpoints,
    addBreakpoint,
    removeBreakpoint,
    toggleBreakpoint,
    setVariables,
    setCallStack,
    setRegisters,
    addWatchExpression,
    removeWatchExpression,
    updateWatchValue,
    setSerialConnected,
    setSerialConfig,
    addSerialData,
    clearSerialData,
    setActivePanel,
    setActiveBottomTab
  }
})

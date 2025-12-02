<template>
  <mdui-layout class="debugger-layout">
    <!-- 顶部导航栏 -->
    <mdui-top-app-bar>
      <mdui-top-app-bar-title>Gracemont Debugger</mdui-top-app-bar-title>
      <div style="flex-grow: 1"></div>

      <!-- 调试控制按钮 -->
      <div class="debug-controls">
        <mdui-tooltip content="继续 (F5)">
          <mdui-button-icon :disabled="!store.canContinue" @click="debugContinue">
            <mdui-icon-play-arrow></mdui-icon-play-arrow>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="暂停 (F6)">
          <mdui-button-icon :disabled="!store.canPause" @click="debugPause">
            <mdui-icon-pause></mdui-icon-pause>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="单步跳过 (F10)">
          <mdui-button-icon :disabled="!store.canContinue" @click="debugStepOver">
            <mdui-icon-redo></mdui-icon-redo>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="单步进入 (F11)">
          <mdui-button-icon :disabled="!store.canContinue" @click="debugStep">
            <mdui-icon-arrow-downward></mdui-icon-arrow-downward>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="单步跳出 (Shift+F11)">
          <mdui-button-icon :disabled="!store.canContinue" @click="debugStepOut">
            <mdui-icon-arrow-upward></mdui-icon-arrow-upward>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="停止">
          <mdui-button-icon :disabled="!store.gdbRunning" @click="debugStop">
            <mdui-icon-stop></mdui-icon-stop>
          </mdui-button-icon>
        </mdui-tooltip>
      </div>

      <mdui-divider vertical style="height: 24px; margin: 0 12px;"></mdui-divider>

      <!-- 状态指示器 -->
      <mdui-chip v-if="store.gdbRunning && store.gdbPaused" variant="elevated" style="--mdui-color-primary: #ff9800;">
        <mdui-icon-pause-circle slot="icon"></mdui-icon-pause-circle>
        已暂停
      </mdui-chip>
      <mdui-chip v-else-if="store.gdbRunning" variant="elevated" style="--mdui-color-primary: #4caf50;">
        <mdui-icon-play-circle slot="icon"></mdui-icon-play-circle>
        运行中
      </mdui-chip>
      <mdui-chip v-else variant="outlined">
        <mdui-icon-bug-report slot="icon"></mdui-icon-bug-report>
        未连接
      </mdui-chip>

      <mdui-button-icon @click="showConnectDialog = true">
        <mdui-icon-settings></mdui-icon-settings>
      </mdui-button-icon>
    </mdui-top-app-bar>

    <!-- 主内容区 -->
    <mdui-layout-main class="main-content">
      <div class="content-grid">
        <!-- 左侧面板 - 调试信息 -->
        <div class="left-panel">
          <mdui-tabs value="variables" @change="e => store.setActivePanel(e.target.value)">
            <mdui-tab value="variables">
              <mdui-icon-data-object slot="icon"></mdui-icon-data-object>
              变量
            </mdui-tab>
            <mdui-tab value="watch">
              <mdui-icon-visibility slot="icon"></mdui-icon-visibility>
              监视
            </mdui-tab>
            <mdui-tab value="callstack">
              <mdui-icon-layers slot="icon"></mdui-icon-layers>
              调用栈
            </mdui-tab>
            <mdui-tab value="breakpoints">
              <mdui-icon-radio-button-checked slot="icon"></mdui-icon-radio-button-checked>
              断点
            </mdui-tab>
            <mdui-tab value="registers">
              <mdui-icon-memory slot="icon"></mdui-icon-memory>
              寄存器
            </mdui-tab>
          </mdui-tabs>

          <div class="panel-content">
            <!-- 变量面板 -->
            <div v-if="store.activePanel === 'variables'" class="variables-panel">
              <div v-if="store.variables.length === 0" class="empty-state">
                暂无变量信息
              </div>
              <mdui-list v-else>
                <mdui-list-item v-for="v in store.variables" :key="v.name">
                  <span slot="headline">{{ v.name }}</span>
                  <span slot="description" class="var-value">{{ v.value }}</span>
                  <span slot="end-icon" class="var-type">{{ v.type }}</span>
                </mdui-list-item>
              </mdui-list>
            </div>

            <!-- 监视面板 -->
            <div v-if="store.activePanel === 'watch'" class="watch-panel">
              <div class="watch-input">
                <mdui-text-field
                  label="添加监视表达式"
                  v-model="newWatchExpr"
                  @keyup.enter="addWatch"
                >
                  <mdui-button-icon slot="end-icon" @click="addWatch">
                    <mdui-icon-add></mdui-icon-add>
                  </mdui-button-icon>
                </mdui-text-field>
              </div>
              <mdui-list>
                <mdui-list-item v-for="w in store.watchExpressions" :key="w.id">
                  <span slot="headline">{{ w.expression }}</span>
                  <span slot="description" class="var-value">{{ w.value ?? '未计算' }}</span>
                  <mdui-button-icon slot="end-icon" @click="removeWatch(w.id)">
                    <mdui-icon-delete></mdui-icon-delete>
                  </mdui-button-icon>
                </mdui-list-item>
              </mdui-list>
            </div>

            <!-- 调用栈面板 -->
            <div v-if="store.activePanel === 'callstack'" class="callstack-panel">
              <div v-if="store.callStack.length === 0" class="empty-state">
                暂无调用栈信息
              </div>
              <mdui-list v-else>
                <mdui-list-item
                  v-for="(frame, idx) in store.callStack"
                  :key="idx"
                  :active="idx === 0"
                >
                  <span slot="headline">{{ frame.func || '???' }}</span>
                  <span slot="description">{{ frame.file }}:{{ frame.line }}</span>
                  <span slot="end-icon" class="frame-addr">{{ frame.addr }}</span>
                </mdui-list-item>
              </mdui-list>
            </div>

            <!-- 断点面板 -->
            <div v-if="store.activePanel === 'breakpoints'" class="breakpoints-panel">
              <div class="breakpoint-actions">
                <mdui-button variant="text" @click="showAddBreakpointDialog = true">
                  <mdui-icon-add slot="icon"></mdui-icon-add>
                  添加断点
                </mdui-button>
              </div>
              <mdui-list>
                <mdui-list-item v-for="bp in store.breakpoints" :key="bp.id">
                  <mdui-checkbox
                    slot="icon"
                    :checked="bp.enabled"
                    @change="toggleBreakpoint(bp.id)"
                  ></mdui-checkbox>
                  <span slot="headline">{{ bp.file }}:{{ bp.line }}</span>
                  <span slot="description" v-if="bp.condition">条件: {{ bp.condition }}</span>
                  <mdui-button-icon slot="end-icon" @click="removeBreakpoint(bp.id)">
                    <mdui-icon-delete></mdui-icon-delete>
                  </mdui-button-icon>
                </mdui-list-item>
              </mdui-list>
            </div>

            <!-- 寄存器面板 -->
            <div v-if="store.activePanel === 'registers'" class="registers-panel">
              <div v-if="store.registers.length === 0" class="empty-state">
                暂无寄存器信息
              </div>
              <div v-else class="registers-grid">
                <div v-for="(reg, idx) in store.registers" :key="idx" class="register-item">
                  <span class="reg-name">{{ reg.name || `r${idx}` }}</span>
                  <span class="reg-value">{{ reg.value }}</span>
                </div>
              </div>
            </div>
          </div>
        </div>

        <!-- 右侧 - 底部面板 -->
        <div class="right-panel">
          <mdui-tabs value="console" @change="e => store.setActiveBottomTab(e.target.value)">
            <mdui-tab value="console">
              <mdui-icon-terminal slot="icon"></mdui-icon-terminal>
              GDB 控制台
            </mdui-tab>
            <mdui-tab value="serial">
              <mdui-icon-usb slot="icon"></mdui-icon-usb>
              串口调试
            </mdui-tab>
          </mdui-tabs>

          <div class="panel-content">
            <!-- GDB 控制台 -->
            <div v-if="store.activeBottomTab === 'console'" class="console-panel">
              <div class="console-output" ref="consoleRef">
                <div
                  v-for="(line, idx) in store.gdbOutput"
                  :key="idx"
                  class="console-line"
                  :class="line.type"
                >
                  <span class="console-time">[{{ line.time }}]</span>
                  <span class="console-text">{{ line.data }}</span>
                </div>
                <div v-if="store.gdbOutput.length === 0" class="console-empty">
                  GDB 未启动
                </div>
              </div>
              <div class="console-input">
                <mdui-text-field
                  label="GDB 命令"
                  v-model="gdbCommand"
                  @keyup.enter="sendGDBCommand"
                >
                  <mdui-button-icon slot="end-icon" @click="sendGDBCommand">
                    <mdui-icon-send></mdui-icon-send>
                  </mdui-button-icon>
                </mdui-text-field>
              </div>
            </div>

            <!-- 串口调试 -->
            <div v-if="store.activeBottomTab === 'serial'" class="serial-panel">
              <div class="serial-toolbar">
                <mdui-select label="串口" v-model="selectedPort" style="width: 200px;">
                  <mdui-menu-item v-for="port in serialPorts" :key="port.path" :value="port.path">
                    {{ port.path }}
                  </mdui-menu-item>
                </mdui-select>
                <mdui-select label="波特率" v-model="serialBaudRate" style="width: 120px;">
                  <mdui-menu-item value="9600">9600</mdui-menu-item>
                  <mdui-menu-item value="19200">19200</mdui-menu-item>
                  <mdui-menu-item value="38400">38400</mdui-menu-item>
                  <mdui-menu-item value="57600">57600</mdui-menu-item>
                  <mdui-menu-item value="115200">115200</mdui-menu-item>
                </mdui-select>
                <mdui-button v-if="!store.serialConnected" @click="connectSerial">
                  <mdui-icon-link slot="icon"></mdui-icon-link>
                  连接
                </mdui-button>
                <mdui-button v-else variant="outlined" @click="disconnectSerial">
                  <mdui-icon-link-off slot="icon"></mdui-icon-link-off>
                  断开
                </mdui-button>
                <mdui-button-icon @click="refreshPorts">
                  <mdui-icon-refresh></mdui-icon-refresh>
                </mdui-button-icon>
              </div>
              <div class="serial-output" ref="serialRef">
                <div
                  v-for="(data, idx) in store.serialData"
                  :key="idx"
                  class="serial-line"
                  :class="data.type"
                >
                  <span class="serial-time">[{{ data.time }}]</span>
                  <span class="serial-dir">{{ data.type === 'tx' ? 'TX' : 'RX' }}</span>
                  <span class="serial-data">{{ data.data }}</span>
                </div>
              </div>
              <div class="serial-input">
                <mdui-text-field
                  label="发送数据"
                  v-model="serialSendData"
                  @keyup.enter="sendSerialData"
                >
                  <mdui-button-icon slot="end-icon" @click="sendSerialData">
                    <mdui-icon-send></mdui-icon-send>
                  </mdui-button-icon>
                </mdui-text-field>
              </div>
            </div>
          </div>
        </div>
      </div>
    </mdui-layout-main>

    <!-- 连接配置对话框 -->
    <mdui-dialog :open="showConnectDialog" @close="showConnectDialog = false">
      <span slot="headline">调试连接配置</span>
      <div slot="description" class="connect-dialog-content">
        <mdui-text-field label="GDB 路径" v-model="gdbPath" placeholder="aarch64-none-elf-gdb"></mdui-text-field>
        <mdui-text-field label="可执行文件" v-model="executablePath">
          <mdui-button-icon slot="end-icon" @click="selectExecutable">
            <mdui-icon-folder-open></mdui-icon-folder-open>
          </mdui-button-icon>
        </mdui-text-field>
        <mdui-text-field label="远程目标" v-model="remoteTarget" placeholder="localhost:3333"></mdui-text-field>
      </div>
      <mdui-button slot="action" variant="text" @click="showConnectDialog = false">取消</mdui-button>
      <mdui-button slot="action" @click="startDebug">
        <mdui-icon-bug-report slot="icon"></mdui-icon-bug-report>
        开始调试
      </mdui-button>
    </mdui-dialog>

    <!-- 添加断点对话框 -->
    <mdui-dialog :open="showAddBreakpointDialog" @close="showAddBreakpointDialog = false">
      <span slot="headline">添加断点</span>
      <div slot="description" class="breakpoint-dialog-content">
        <mdui-text-field label="文件路径" v-model="newBreakpointFile"></mdui-text-field>
        <mdui-text-field label="行号" v-model="newBreakpointLine" type="number"></mdui-text-field>
        <mdui-text-field label="条件 (可选)" v-model="newBreakpointCondition"></mdui-text-field>
      </div>
      <mdui-button slot="action" variant="text" @click="showAddBreakpointDialog = false">取消</mdui-button>
      <mdui-button slot="action" @click="addBreakpoint">
        <mdui-icon-add slot="icon"></mdui-icon-add>
        添加
      </mdui-button>
    </mdui-dialog>
  </mdui-layout>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick } from 'vue'
import { useDebuggerStore } from '../stores/debugger'

// 正确导入图标
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/pause.js'
import '@mdui/icons/redo.js'
import '@mdui/icons/arrow-downward.js'
import '@mdui/icons/arrow-upward.js'
import '@mdui/icons/stop.js'
import '@mdui/icons/pause-circle.js'
import '@mdui/icons/play-circle.js'
import '@mdui/icons/bug-report.js'
import '@mdui/icons/settings.js'
import '@mdui/icons/data-object.js'
import '@mdui/icons/visibility.js'
import '@mdui/icons/layers.js'
import '@mdui/icons/radio-button-checked.js'
import '@mdui/icons/memory.js'
import '@mdui/icons/terminal.js'
import '@mdui/icons/usb.js'
import '@mdui/icons/add.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/send.js'
import '@mdui/icons/link.js'
import '@mdui/icons/link-off.js'
import '@mdui/icons/refresh.js'
import '@mdui/icons/folder-open.js'

// Store
const store = useDebuggerStore()

// Refs
const consoleRef = ref(null)
const serialRef = ref(null)

// UI 状态
const showConnectDialog = ref(false)
const showAddBreakpointDialog = ref(false)

// GDB 配置
const gdbPath = ref('aarch64-none-elf-gdb')
const executablePath = ref('')
const remoteTarget = ref('localhost:3333')
const gdbCommand = ref('')

// 断点配置
const newBreakpointFile = ref('')
const newBreakpointLine = ref('')
const newBreakpointCondition = ref('')

// 监视表达式
const newWatchExpr = ref('')

// 串口配置
const serialPorts = ref([])
const selectedPort = ref('')
const serialBaudRate = ref('115200')
const serialSendData = ref('')

// 调试控制
async function debugContinue() {
  await window.electronAPI.gdbContinue()
}

async function debugPause() {
  await window.electronAPI.gdbPause()
}

async function debugStep() {
  await window.electronAPI.gdbStep()
}

async function debugStepOver() {
  await window.electronAPI.gdbStepOver()
}

async function debugStepOut() {
  await window.electronAPI.gdbStepOut()
}

async function debugStop() {
  await window.electronAPI.gdbStop()
}

async function startDebug() {
  const result = await window.electronAPI.gdbStart({
    gdbPath: gdbPath.value,
    executable: executablePath.value,
    target: remoteTarget.value
  })

  if (result.success) {
    showConnectDialog.value = false
  }
}

async function sendGDBCommand() {
  if (!gdbCommand.value.trim()) return

  await window.electronAPI.gdbCommand(gdbCommand.value)
  gdbCommand.value = ''
}

// 断点管理
async function addBreakpoint() {
  const result = await window.electronAPI.addBreakpoint({
    file: newBreakpointFile.value,
    line: parseInt(newBreakpointLine.value),
    condition: newBreakpointCondition.value || null
  })

  if (result.success) {
    store.addBreakpoint(result.breakpoint)
    showAddBreakpointDialog.value = false
    newBreakpointFile.value = ''
    newBreakpointLine.value = ''
    newBreakpointCondition.value = ''
  }
}

async function removeBreakpoint(id) {
  const result = await window.electronAPI.removeBreakpoint(id)
  if (result.success) {
    store.removeBreakpoint(id)
  }
}

async function toggleBreakpoint(id) {
  const bp = store.breakpoints.find(b => b.id === id)
  if (bp) {
    await window.electronAPI.enableBreakpoint(id, !bp.enabled)
    store.toggleBreakpoint(id)
  }
}

// 监视表达式
function addWatch() {
  if (!newWatchExpr.value.trim()) return
  store.addWatchExpression(newWatchExpr.value)
  newWatchExpr.value = ''
}

function removeWatch(id) {
  store.removeWatchExpression(id)
}

// 串口操作
async function refreshPorts() {
  const result = await window.electronAPI.serialList()
  if (result.success) {
    serialPorts.value = result.ports
  }
}

async function connectSerial() {
  const result = await window.electronAPI.serialOpen({
    port: selectedPort.value,
    baudRate: parseInt(serialBaudRate.value)
  })

  if (result.success) {
    store.setSerialConnected(true)
  }
}

async function disconnectSerial() {
  const result = await window.electronAPI.serialClose()
  if (result.success) {
    store.setSerialConnected(false)
  }
}

async function sendSerialData() {
  if (!serialSendData.value.trim()) return

  await window.electronAPI.serialWrite({ text: serialSendData.value + '\n' })
  serialSendData.value = ''
}

function selectExecutable() {
  // 实现文件选择
}

// 监听事件
onMounted(async () => {
  // GDB 输出
  window.electronAPI.onGDBOutput((data) => {
    store.addGDBOutput(data)
    nextTick(() => {
      if (consoleRef.value) {
        consoleRef.value.scrollTop = consoleRef.value.scrollHeight
      }
    })
  })

  // GDB 状态
  window.electronAPI.onGDBStateChange((state) => {
    store.setGDBState(state)
  })

  // 断点命中
  window.electronAPI.onGDBBreakpoint((data) => {
    // 处理断点命中
  })

  // 串口数据
  window.electronAPI.onSerialData((data) => {
    store.addSerialData(data)
    nextTick(() => {
      if (serialRef.value) {
        serialRef.value.scrollTop = serialRef.value.scrollHeight
      }
    })
  })

  // 刷新串口列表
  await refreshPorts()
})

onUnmounted(() => {
  window.electronAPI.removeAllListeners()
})
</script>

<style scoped>
.debugger-layout {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.debug-controls {
  display: flex;
  gap: 4px;
}

.main-content {
  flex: 1;
  overflow: hidden;
  background: #f5f5f5;
}

.content-grid {
  display: grid;
  grid-template-columns: 300px 1fr;
  height: 100%;
  gap: 1px;
  background: #e0e0e0;
}

.left-panel,
.right-panel {
  background: white;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.panel-content {
  flex: 1;
  overflow: auto;
  padding: 8px;
}

/* 变量/监视面板 */
.var-value {
  font-family: 'Monaco', 'Menlo', monospace;
  color: #1976d2;
}

.var-type {
  font-size: 12px;
  color: #666;
}

/* 调用栈 */
.frame-addr {
  font-family: monospace;
  font-size: 12px;
  color: #666;
}

/* 断点 */
.breakpoint-actions {
  padding: 8px;
  border-bottom: 1px solid #e0e0e0;
}

/* 寄存器 */
.registers-grid {
  display: grid;
  grid-template-columns: repeat(4, 1fr);
  gap: 8px;
}

.register-item {
  display: flex;
  flex-direction: column;
  padding: 8px;
  background: #f5f5f5;
  border-radius: 4px;
}

.reg-name {
  font-size: 12px;
  font-weight: 500;
  color: #666;
}

.reg-value {
  font-family: monospace;
  font-size: 14px;
}

/* 监视输入 */
.watch-input {
  padding: 8px;
  border-bottom: 1px solid #e0e0e0;
}

/* 控制台 */
.console-panel,
.serial-panel {
  display: flex;
  flex-direction: column;
  height: 100%;
}

.console-output,
.serial-output {
  flex: 1;
  background: #1e1e1e;
  border-radius: 8px;
  padding: 12px;
  overflow: auto;
  font-family: 'Monaco', 'Menlo', monospace;
  font-size: 13px;
  margin-bottom: 8px;
}

.console-line,
.serial-line {
  display: flex;
  gap: 8px;
  line-height: 1.6;
}

.console-line.stdout {
  color: #d4d4d4;
}

.console-line.stderr {
  color: #f48771;
}

.console-time,
.serial-time {
  color: #6a9955;
}

.console-text {
  white-space: pre-wrap;
  word-break: break-all;
}

.console-empty {
  color: #666;
  text-align: center;
  padding: 40px;
}

.console-input {
  padding: 8px 0;
}

/* 串口 */
.serial-toolbar {
  display: flex;
  gap: 12px;
  align-items: center;
  padding: 8px 0;
  flex-wrap: wrap;
}

.serial-line.tx {
  color: #4fc3f7;
}

.serial-line.rx {
  color: #81c784;
}

.serial-dir {
  font-weight: bold;
  width: 24px;
}

.serial-data {
  white-space: pre-wrap;
  word-break: break-all;
}

.serial-input {
  padding: 8px 0;
}

/* 空状态 */
.empty-state {
  text-align: center;
  padding: 40px;
  color: #666;
}

/* 对话框 */
.connect-dialog-content,
.breakpoint-dialog-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-width: 400px;
}

@media (max-width: 1024px) {
  .content-grid {
    grid-template-columns: 1fr;
    grid-template-rows: 1fr 1fr;
  }

  .registers-grid {
    grid-template-columns: repeat(2, 1fr);
  }
}
</style>

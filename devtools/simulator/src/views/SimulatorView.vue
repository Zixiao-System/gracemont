<template>
  <mdui-layout class="simulator-layout">
    <!-- 顶部导航栏 -->
    <mdui-top-app-bar>
      <mdui-button-icon @click="toggleDrawer">
        <mdui-icon-menu></mdui-icon-menu>
      </mdui-button-icon>
      <mdui-top-app-bar-title>Gracemont Simulator</mdui-top-app-bar-title>
      <div style="flex-grow: 1"></div>

      <!-- QEMU 状态 -->
      <mdui-chip v-if="store.qemuRunning" variant="elevated" style="--mdui-color-primary: #4caf50;">
        <mdui-icon-memory slot="icon"></mdui-icon-memory>
        QEMU 运行中
      </mdui-chip>
      <mdui-chip v-else variant="outlined">
        <mdui-icon-memory slot="icon"></mdui-icon-memory>
        QEMU 未启动
      </mdui-chip>

      <mdui-button-icon @click="showSettings = true">
        <mdui-icon-settings></mdui-icon-settings>
      </mdui-button-icon>
    </mdui-top-app-bar>

    <!-- 侧边栏 -->
    <mdui-navigation-drawer :open="drawerOpen" @close="drawerOpen = false">
      <mdui-list>
        <mdui-list-subheader>仿真控制</mdui-list-subheader>
        <mdui-list-item
          :active="store.activeTab === 'io'"
          @click="store.setActiveTab('io')"
        >
          <mdui-icon-developer-board slot="icon"></mdui-icon-developer-board>
          I/O 面板
        </mdui-list-item>
        <mdui-list-item
          :active="store.activeTab === 'modbus'"
          @click="store.setActiveTab('modbus')"
        >
          <mdui-icon-swap-horiz slot="icon"></mdui-icon-swap-horiz>
          Modbus 客户端
        </mdui-list-item>
        <mdui-list-item
          :active="store.activeTab === 'opcua'"
          @click="store.setActiveTab('opcua')"
        >
          <mdui-icon-cloud slot="icon"></mdui-icon-cloud>
          OPC UA 客户端
        </mdui-list-item>
        <mdui-list-item
          :active="store.activeTab === 'console'"
          @click="store.setActiveTab('console')"
        >
          <mdui-icon-terminal slot="icon"></mdui-icon-terminal>
          QEMU 控制台
        </mdui-list-item>

        <mdui-divider></mdui-divider>
        <mdui-list-subheader>QEMU 控制</mdui-list-subheader>
        <mdui-list-item @click="showQEMUDialog = true">
          <mdui-icon-play-arrow slot="icon"></mdui-icon-play-arrow>
          启动配置
        </mdui-list-item>
      </mdui-list>
    </mdui-navigation-drawer>

    <!-- 主内容区 -->
    <mdui-layout-main class="main-content">
      <!-- I/O 面板 -->
      <div v-if="store.activeTab === 'io'" class="panel io-panel">
        <div class="panel-header">
          <h3>I/O 仿真面板</h3>
          <mdui-segmented-button-group value="visual" @change="e => store.setIOViewMode(e.target.value)">
            <mdui-segmented-button value="visual">
              <mdui-icon-grid-view slot="icon"></mdui-icon-grid-view>
              可视化
            </mdui-segmented-button>
            <mdui-segmented-button value="table">
              <mdui-icon-table-rows slot="icon"></mdui-icon-table-rows>
              表格
            </mdui-segmented-button>
          </mdui-segmented-button-group>
        </div>

        <div class="io-content">
          <!-- 数字输入 -->
          <mdui-card class="io-card">
            <div class="card-title">
              <mdui-icon-input></mdui-icon-input>
              <span>数字输入 (DI 0-31)</span>
            </div>
            <div class="io-grid">
              <div
                v-for="(val, idx) in store.digitalInputs"
                :key="'di-' + idx"
                class="io-item"
                :class="{ active: val }"
                @click="store.toggleDigitalInput(idx)"
              >
                <span class="io-label">DI{{ idx }}</span>
                <span class="io-value">{{ val ? '1' : '0' }}</span>
              </div>
            </div>
          </mdui-card>

          <!-- 数字输出 -->
          <mdui-card class="io-card">
            <div class="card-title">
              <mdui-icon-output></mdui-icon-output>
              <span>数字输出 (DO 0-31)</span>
            </div>
            <div class="io-grid">
              <div
                v-for="(val, idx) in store.digitalOutputs"
                :key="'do-' + idx"
                class="io-item output"
                :class="{ active: val }"
              >
                <span class="io-label">DO{{ idx }}</span>
                <span class="io-value">{{ val ? '1' : '0' }}</span>
              </div>
            </div>
          </mdui-card>

          <!-- 模拟输入 -->
          <mdui-card class="io-card">
            <div class="card-title">
              <mdui-icon-show-chart slot="icon"></mdui-icon-show-chart>
              <span>模拟输入 (AI 0-15)</span>
            </div>
            <div class="analog-grid">
              <div v-for="(val, idx) in store.analogInputs" :key="'ai-' + idx" class="analog-item">
                <span class="analog-label">AI{{ idx }}</span>
                <mdui-slider
                  :value="val"
                  min="0"
                  max="4095"
                  @input="e => updateAnalogInput(idx, e.target.value)"
                ></mdui-slider>
                <span class="analog-value">{{ val }}</span>
              </div>
            </div>
          </mdui-card>

          <!-- 模拟输出 -->
          <mdui-card class="io-card">
            <div class="card-title">
              <mdui-icon-trending-up slot="icon"></mdui-icon-trending-up>
              <span>模拟输出 (AO 0-7)</span>
            </div>
            <div class="analog-grid">
              <div v-for="(val, idx) in store.analogOutputs" :key="'ao-' + idx" class="analog-item">
                <span class="analog-label">AO{{ idx }}</span>
                <mdui-linear-progress :value="val / 4095"></mdui-linear-progress>
                <span class="analog-value">{{ val }}</span>
              </div>
            </div>
          </mdui-card>
        </div>
      </div>

      <!-- Modbus 客户端 -->
      <div v-if="store.activeTab === 'modbus'" class="panel modbus-panel">
        <div class="panel-header">
          <h3>Modbus 客户端</h3>
          <mdui-chip v-if="store.modbusConnected" variant="elevated" style="--mdui-color-primary: #4caf50;">
            已连接
          </mdui-chip>
          <mdui-chip v-else variant="outlined">未连接</mdui-chip>
        </div>

        <div class="modbus-content">
          <!-- 连接配置 -->
          <mdui-card class="config-card">
            <div class="card-title">连接配置</div>
            <mdui-select label="连接类型" v-model="modbusType">
              <mdui-menu-item value="tcp">Modbus TCP</mdui-menu-item>
              <mdui-menu-item value="rtu">Modbus RTU</mdui-menu-item>
            </mdui-select>

            <div v-if="modbusType === 'tcp'" class="form-row">
              <mdui-text-field label="主机地址" v-model="modbusHost"></mdui-text-field>
              <mdui-text-field label="端口" v-model="modbusPort" type="number"></mdui-text-field>
            </div>
            <div v-else class="form-row">
              <mdui-text-field label="串口" v-model="modbusSerialPort"></mdui-text-field>
              <mdui-text-field label="波特率" v-model="modbusBaudRate" type="number"></mdui-text-field>
            </div>

            <mdui-text-field label="从站地址" v-model="modbusUnitId" type="number"></mdui-text-field>

            <div class="button-row">
              <mdui-button v-if="!store.modbusConnected" @click="connectModbus">
                <mdui-icon-link slot="icon"></mdui-icon-link>
                连接
              </mdui-button>
              <mdui-button v-else variant="outlined" @click="disconnectModbus">
                <mdui-icon-link-off slot="icon"></mdui-icon-link-off>
                断开
              </mdui-button>
            </div>
          </mdui-card>

          <!-- 读写操作 -->
          <mdui-card class="operation-card">
            <div class="card-title">数据读写</div>
            <mdui-select label="功能码" v-model="modbusFunction">
              <mdui-menu-item value="coils">读线圈 (01)</mdui-menu-item>
              <mdui-menu-item value="discrete-inputs">读离散输入 (02)</mdui-menu-item>
              <mdui-menu-item value="holding-registers">读保持寄存器 (03)</mdui-menu-item>
              <mdui-menu-item value="input-registers">读输入寄存器 (04)</mdui-menu-item>
            </mdui-select>

            <div class="form-row">
              <mdui-text-field label="起始地址" v-model="modbusAddress" type="number"></mdui-text-field>
              <mdui-text-field label="数量" v-model="modbusLength" type="number"></mdui-text-field>
            </div>

            <div class="button-row">
              <mdui-button @click="readModbus" :disabled="!store.modbusConnected">
                <mdui-icon-download slot="icon"></mdui-icon-download>
                读取
              </mdui-button>
            </div>

            <!-- 读取结果 -->
            <div v-if="modbusResult" class="result-box">
              <div class="result-title">读取结果</div>
              <pre>{{ JSON.stringify(modbusResult, null, 2) }}</pre>
            </div>
          </mdui-card>
        </div>
      </div>

      <!-- OPC UA 客户端 -->
      <div v-if="store.activeTab === 'opcua'" class="panel opcua-panel">
        <div class="panel-header">
          <h3>OPC UA 客户端</h3>
          <mdui-chip v-if="store.opcuaConnected" variant="elevated" style="--mdui-color-primary: #4caf50;">
            已连接
          </mdui-chip>
          <mdui-chip v-else variant="outlined">未连接</mdui-chip>
        </div>

        <div class="opcua-content">
          <!-- 连接配置 -->
          <mdui-card class="config-card">
            <div class="card-title">连接配置</div>
            <mdui-text-field
              label="端点 URL"
              v-model="opcuaEndpoint"
              placeholder="opc.tcp://localhost:4840"
            ></mdui-text-field>

            <div class="button-row">
              <mdui-button v-if="!store.opcuaConnected" @click="connectOpcua">
                <mdui-icon-link slot="icon"></mdui-icon-link>
                连接
              </mdui-button>
              <mdui-button v-else variant="outlined" @click="disconnectOpcua">
                <mdui-icon-link-off slot="icon"></mdui-icon-link-off>
                断开
              </mdui-button>
            </div>
          </mdui-card>

          <!-- 节点读写 -->
          <mdui-card class="operation-card">
            <div class="card-title">节点读写</div>
            <mdui-text-field
              label="节点 ID"
              v-model="opcuaNodeId"
              placeholder="ns=2;s=MyVariable"
            ></mdui-text-field>

            <div class="button-row">
              <mdui-button @click="readOpcua" :disabled="!store.opcuaConnected">
                <mdui-icon-download slot="icon"></mdui-icon-download>
                读取
              </mdui-button>
            </div>

            <!-- 读取结果 -->
            <div v-if="opcuaResult" class="result-box">
              <div class="result-title">读取结果</div>
              <pre>{{ JSON.stringify(opcuaResult, null, 2) }}</pre>
            </div>
          </mdui-card>
        </div>
      </div>

      <!-- QEMU 控制台 -->
      <div v-if="store.activeTab === 'console'" class="panel console-panel">
        <div class="panel-header">
          <h3>QEMU 控制台</h3>
          <mdui-button variant="text" @click="store.clearQEMUOutput">
            <mdui-icon-delete slot="icon"></mdui-icon-delete>
            清空
          </mdui-button>
        </div>

        <div class="console-output" ref="consoleRef">
          <div
            v-for="(line, idx) in store.qemuOutput"
            :key="idx"
            class="console-line"
            :class="line.type"
          >
            <span class="console-time">[{{ line.time }}]</span>
            <span class="console-text">{{ line.data }}</span>
          </div>
          <div v-if="store.qemuOutput.length === 0" class="console-empty">
            QEMU 未启动或无输出
          </div>
        </div>
      </div>
    </mdui-layout-main>

    <!-- QEMU 配置对话框 -->
    <mdui-dialog :open="showQEMUDialog" @close="showQEMUDialog = false">
      <span slot="headline">QEMU 启动配置</span>
      <div slot="description" class="qemu-dialog-content">
        <mdui-text-field label="内核路径" v-model="qemuKernelPath">
          <mdui-button-icon slot="end-icon" @click="selectKernelFile">
            <mdui-icon-folder-open></mdui-icon-folder-open>
          </mdui-button-icon>
        </mdui-text-field>
        <mdui-select label="机器类型" v-model="qemuMachine">
          <mdui-menu-item value="virt">virt (ARM64)</mdui-menu-item>
          <mdui-menu-item value="raspi3b">Raspberry Pi 3B</mdui-menu-item>
        </mdui-select>
        <mdui-select label="CPU" v-model="qemuCPU">
          <mdui-menu-item value="cortex-a57">Cortex-A57</mdui-menu-item>
          <mdui-menu-item value="cortex-a72">Cortex-A72</mdui-menu-item>
        </mdui-select>
        <mdui-text-field label="内存大小" v-model="qemuMemory" placeholder="512M"></mdui-text-field>
      </div>
      <mdui-button slot="action" variant="text" @click="showQEMUDialog = false">取消</mdui-button>
      <mdui-button slot="action" @click="startQEMU">
        <mdui-icon-play-arrow slot="icon"></mdui-icon-play-arrow>
        启动
      </mdui-button>
    </mdui-dialog>

    <!-- 设置对话框 -->
    <mdui-dialog :open="showSettings" @close="showSettings = false">
      <span slot="headline">设置</span>
      <div slot="description">
        <p>Simulator 设置选项</p>
      </div>
      <mdui-button slot="action" @click="showSettings = false">关闭</mdui-button>
    </mdui-dialog>
  </mdui-layout>
</template>

<script setup>
import { ref, onMounted, onUnmounted, nextTick, watch } from 'vue'
import { useSimulatorStore } from '../stores/simulator'

// 正确导入图标
import '@mdui/icons/menu.js'
import '@mdui/icons/settings.js'
import '@mdui/icons/memory.js'
import '@mdui/icons/developer-board.js'
import '@mdui/icons/swap-horiz.js'
import '@mdui/icons/cloud.js'
import '@mdui/icons/terminal.js'
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/grid-view.js'
import '@mdui/icons/table-rows.js'
import '@mdui/icons/input.js'
import '@mdui/icons/output.js'
import '@mdui/icons/show-chart.js'
import '@mdui/icons/trending-up.js'
import '@mdui/icons/link.js'
import '@mdui/icons/link-off.js'
import '@mdui/icons/download.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/folder-open.js'

// Store
const store = useSimulatorStore()

// UI 状态
const drawerOpen = ref(false)
const showQEMUDialog = ref(false)
const showSettings = ref(false)
const consoleRef = ref(null)

// QEMU 配置
const qemuKernelPath = ref('')
const qemuMachine = ref('virt')
const qemuCPU = ref('cortex-a57')
const qemuMemory = ref('512M')

// Modbus 配置
const modbusType = ref('tcp')
const modbusHost = ref('127.0.0.1')
const modbusPort = ref('502')
const modbusSerialPort = ref('/dev/ttyUSB0')
const modbusBaudRate = ref('9600')
const modbusUnitId = ref('1')
const modbusFunction = ref('holding-registers')
const modbusAddress = ref('0')
const modbusLength = ref('10')
const modbusResult = ref(null)

// OPC UA 配置
const opcuaEndpoint = ref('opc.tcp://localhost:4840')
const opcuaNodeId = ref('')
const opcuaResult = ref(null)

// 方法
function toggleDrawer() {
  drawerOpen.value = !drawerOpen.value
}

function updateAnalogInput(idx, value) {
  store.setAnalogInput(idx, parseInt(value))
}

// QEMU 控制
async function selectKernelFile() {
  // 这里需要调用 electronAPI 打开文件选择对话框
  // 简化处理
}

async function startQEMU() {
  const config = {
    kernelPath: qemuKernelPath.value,
    machine: qemuMachine.value,
    cpu: qemuCPU.value,
    memory: qemuMemory.value
  }

  const result = await window.electronAPI.startQEMU(config)
  if (result.success) {
    store.setQEMURunning(true)
    store.setQEMUConfig(config)
    showQEMUDialog.value = false
    store.setActiveTab('console')
  }
}

async function stopQEMU() {
  const result = await window.electronAPI.stopQEMU()
  if (result.success) {
    store.setQEMURunning(false)
  }
}

// Modbus 操作
async function connectModbus() {
  const config = {
    type: modbusType.value,
    host: modbusHost.value,
    port: parseInt(modbusPort.value),
    port: modbusSerialPort.value,
    baudRate: parseInt(modbusBaudRate.value),
    unitId: parseInt(modbusUnitId.value)
  }

  const result = await window.electronAPI.modbusConnect(config)
  if (result.success) {
    store.setModbusConnected(true)
    store.setModbusConfig(config)
  }
}

async function disconnectModbus() {
  const result = await window.electronAPI.modbusDisconnect()
  if (result.success) {
    store.setModbusConnected(false)
  }
}

async function readModbus() {
  const result = await window.electronAPI.modbusRead({
    type: modbusFunction.value,
    address: parseInt(modbusAddress.value),
    length: parseInt(modbusLength.value)
  })

  modbusResult.value = result
}

// OPC UA 操作
async function connectOpcua() {
  const result = await window.electronAPI.opcuaConnect({
    endpointUrl: opcuaEndpoint.value
  })

  if (result.success) {
    store.setOpcuaConnected(true)
  }
}

async function disconnectOpcua() {
  const result = await window.electronAPI.opcuaDisconnect()
  if (result.success) {
    store.setOpcuaConnected(false)
  }
}

async function readOpcua() {
  const result = await window.electronAPI.opcuaRead(opcuaNodeId.value)
  opcuaResult.value = result
}

// 监听 QEMU 输出
onMounted(() => {
  window.electronAPI.onQEMUOutput((data) => {
    store.addQEMUOutput(data)
    nextTick(() => {
      if (consoleRef.value) {
        consoleRef.value.scrollTop = consoleRef.value.scrollHeight
      }
    })
  })

  window.electronAPI.onQEMUStateChange((state) => {
    store.setQEMURunning(state.running)
  })

  window.electronAPI.onIOChange((data) => {
    if (data.type === 'digital-output') {
      store.setDigitalOutput(data.pin, data.value)
    } else if (data.type === 'analog-output') {
      store.setAnalogOutput(data.channel, data.value)
    }
  })
})

onUnmounted(() => {
  window.electronAPI.removeAllListeners()
})
</script>

<style scoped>
.simulator-layout {
  width: 100%;
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.main-content {
  flex: 1;
  overflow: auto;
  background: #f5f5f5;
  padding: 16px;
}

.panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  margin-bottom: 16px;
}

.panel-header h3 {
  margin: 0;
  font-size: 20px;
  font-weight: 500;
}

/* I/O 面板 */
.io-content {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
  flex: 1;
  overflow: auto;
}

.io-card {
  padding: 16px;
}

.card-title {
  display: flex;
  align-items: center;
  gap: 8px;
  font-size: 16px;
  font-weight: 500;
  margin-bottom: 16px;
  color: #333;
}

.io-grid {
  display: grid;
  grid-template-columns: repeat(8, 1fr);
  gap: 8px;
}

.io-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 8px 4px;
  border-radius: 8px;
  background: #e0e0e0;
  cursor: pointer;
  transition: all 0.2s;
}

.io-item:hover {
  background: #d0d0d0;
}

.io-item.active {
  background: #4caf50;
  color: white;
}

.io-item.output {
  cursor: default;
}

.io-item.output.active {
  background: #2196f3;
}

.io-label {
  font-size: 10px;
  font-weight: 500;
}

.io-value {
  font-size: 14px;
  font-weight: bold;
}

.analog-grid {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.analog-item {
  display: grid;
  grid-template-columns: 50px 1fr 60px;
  align-items: center;
  gap: 12px;
}

.analog-label {
  font-size: 12px;
  font-weight: 500;
}

.analog-value {
  font-size: 12px;
  text-align: right;
  font-family: monospace;
}

/* Modbus / OPC UA 面板 */
.modbus-content,
.opcua-content {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.config-card,
.operation-card {
  padding: 16px;
}

.form-row {
  display: grid;
  grid-template-columns: 1fr 1fr;
  gap: 12px;
  margin-top: 12px;
}

.button-row {
  display: flex;
  gap: 12px;
  margin-top: 16px;
}

.result-box {
  margin-top: 16px;
  padding: 12px;
  background: #f5f5f5;
  border-radius: 8px;
}

.result-title {
  font-size: 12px;
  font-weight: 500;
  margin-bottom: 8px;
  color: #666;
}

.result-box pre {
  margin: 0;
  font-size: 12px;
  white-space: pre-wrap;
  word-break: break-all;
}

/* 控制台 */
.console-panel {
  display: flex;
  flex-direction: column;
}

.console-output {
  flex: 1;
  background: #1e1e1e;
  border-radius: 8px;
  padding: 12px;
  overflow: auto;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', monospace;
  font-size: 13px;
}

.console-line {
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

.console-time {
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

/* 对话框 */
.qemu-dialog-content {
  display: flex;
  flex-direction: column;
  gap: 16px;
  min-width: 400px;
}

@media (max-width: 1024px) {
  .io-content {
    grid-template-columns: 1fr;
  }

  .io-grid {
    grid-template-columns: repeat(4, 1fr);
  }

  .modbus-content,
  .opcua-content {
    grid-template-columns: 1fr;
  }
}
</style>

<template>
  <div class="flasher-view">
    <!-- 顶部栏 -->
    <mdui-top-app-bar scroll-behavior="elevate">
      <mdui-top-app-bar-title>Greyy the Lightningbearer - Flasher</mdui-top-app-bar-title>
      <div style="flex-grow: 1"></div>
      <mdui-button-icon @click="showSettings = true">
        <mdui-icon-settings></mdui-icon-settings>
      </mdui-button-icon>
    </mdui-top-app-bar>

    <!-- 主内容区 -->
    <div class="content">
      <!-- 文件选择区 -->
      <mdui-card variant="elevated" class="file-card">
        <div class="card-content">
          <div class="file-info">
            <mdui-icon-description style="font-size: 48px"></mdui-icon-description>
            <div class="file-details">
              <div class="file-name">{{ fileInfo.name || '未选择文件' }}</div>
              <div class="file-meta" v-if="fileInfo.size">
                大小: {{ formatSize(fileInfo.size) }} | 校验: {{ fileInfo.checksum }}
              </div>
            </div>
          </div>
          <mdui-button variant="tonal" @click="selectFile">
            <mdui-icon-folder-open slot="icon"></mdui-icon-folder-open>
            选择文件
          </mdui-button>
        </div>
      </mdui-card>

      <!-- 设备选择区 -->
      <mdui-card variant="elevated" class="device-card">
        <div class="card-content">
          <div class="device-selector">
            <mdui-select
              v-model="selectedPort"
              label="串口设备"
              :disabled="portOpen || flashing"
              style="flex: 1"
            >
              <mdui-menu-item v-for="port in ports" :key="port.path" :value="port.path">
                {{ port.path }} {{ port.manufacturer ? `(${port.manufacturer})` : '' }}
              </mdui-menu-item>
            </mdui-select>

            <mdui-button-icon
              @click="refreshPorts"
              :disabled="portOpen || flashing"
            >
              <mdui-icon-refresh></mdui-icon-refresh>
            </mdui-button-icon>
          </div>

          <mdui-select
            v-model="baudRate"
            label="波特率"
            :disabled="portOpen || flashing"
            style="width: 200px"
          >
            <mdui-menu-item value="9600">9600</mdui-menu-item>
            <mdui-menu-item value="115200">115200</mdui-menu-item>
            <mdui-menu-item value="921600">921600</mdui-menu-item>
            <mdui-menu-item value="1000000">1000000</mdui-menu-item>
          </mdui-select>

          <mdui-button
            v-if="!portOpen"
            variant="filled"
            @click="openPort"
            :disabled="!selectedPort || flashing"
          >
            <mdui-icon-link slot="icon"></mdui-icon-link>
            连接设备
          </mdui-button>

          <mdui-button
            v-else
            variant="outlined"
            @click="closePort"
            :disabled="flashing"
          >
            <mdui-icon-link-off slot="icon"></mdui-icon-link-off>
            断开连接
          </mdui-button>
        </div>
      </mdui-card>

      <!-- 进度区 -->
      <mdui-card variant="elevated" class="progress-card">
        <div class="card-content">
          <div class="progress-info">
            <div class="progress-text">
              <span class="progress-percent">{{ progressPercent }}%</span>
              <span class="progress-stats">
                速度: {{ formatSpeed(speed) }} | 剩余: {{ formatTime(remainingTime) }}
              </span>
            </div>
            <mdui-linear-progress :value="progressPercent"></mdui-linear-progress>
          </div>

          <div class="action-buttons">
            <mdui-button
              v-if="!flashing"
              variant="filled"
              @click="startFlash"
              :disabled="!fileInfo.size || !portOpen"
            >
              <mdui-icon-play-arrow slot="icon"></mdui-icon-play-arrow>
              开始烧录
            </mdui-button>

            <mdui-button
              v-else
              variant="outlined"
              @click="stopFlash"
            >
              <mdui-icon-stop slot="icon"></mdui-icon-stop>
              停止
            </mdui-button>
          </div>
        </div>
      </mdui-card>

      <!-- 日志区 -->
      <mdui-card variant="elevated" class="log-card">
        <div class="card-header">
          <span>日志</span>
          <mdui-button-icon @click="clearLogs">
            <mdui-icon-delete></mdui-icon-delete>
          </mdui-button-icon>
        </div>
        <div class="log-content">
          <div
            v-for="(log, index) in logs"
            :key="index"
            :class="['log-item', `log-${log.type}`]"
          >
            <span class="log-time">[{{ log.time }}]</span>
            <span class="log-message">{{ log.message }}</span>
          </div>
        </div>
      </mdui-card>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'
import { storeToRefs } from 'pinia'
import { useFlasherStore } from '../stores/flasher'
import { FirmwareParser } from '../utils/firmware-parser'
import { FlashProtocol } from '../utils/flash-protocol'

// 导入 MDUI 组件
import 'mdui/components/top-app-bar.js'
import 'mdui/components/top-app-bar-title.js'
import 'mdui/components/button.js'
import 'mdui/components/button-icon.js'
import 'mdui/components/card.js'
import 'mdui/components/select.js'
import 'mdui/components/menu-item.js'
import 'mdui/components/linear-progress.js'

// 导入图标组件
import '@mdui/icons/settings.js'
import '@mdui/icons/description.js'
import '@mdui/icons/folder-open.js'
import '@mdui/icons/refresh.js'
import '@mdui/icons/link.js'
import '@mdui/icons/link-off.js'
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/stop.js'
import '@mdui/icons/delete.js'

const store = useFlasherStore()
const {
  fileInfo,
  ports,
  selectedPort: storeSelectedPort,
  portConfig,
  portOpen,
  flashing,
  speed,
  logs,
  progressPercent,
  remainingTime
} = storeToRefs(store)

const selectedPort = ref(null)
const baudRate = ref('115200')
const showSettings = ref(false)
let flashProtocol = null

// 格式化函数
const formatSize = (bytes) => {
  if (bytes < 1024) return `${bytes} B`
  if (bytes < 1024 * 1024) return `${(bytes / 1024).toFixed(2)} KB`
  return `${(bytes / (1024 * 1024)).toFixed(2)} MB`
}

const formatSpeed = (kbps) => {
  if (kbps < 1024) return `${kbps.toFixed(1)} KB/s`
  return `${(kbps / 1024).toFixed(2)} MB/s`
}

const formatTime = (seconds) => {
  if (seconds < 60) return `${seconds}s`
  const minutes = Math.floor(seconds / 60)
  const secs = seconds % 60
  return `${minutes}m${secs}s`
}

// 选择文件
const selectFile = async () => {
  try {
    const file = await window.electronAPI.openFile()
    if (!file) return

    store.addLog(`正在加载文件: ${file.name}`, 'info')

    // 读取文件内容
    const result = await window.electronAPI.readFile(file.path)
    if (!result.success) {
      store.addLog(`文件读取失败: ${result.error}`, 'error')
      return
    }

    // 解析固件文件
    try {
      const parsed = FirmwareParser.parse(result.data, file.name)

      store.setFirmwareFile(file)
      store.setFirmwareData(parsed)
      store.setFileInfo({
        name: file.name,
        size: parsed.size,
        checksum: parsed.checksum,
        type: parsed.type
      })

      store.addLog(`文件加载成功: ${file.name}`, 'success')
      store.addLog(`类型: ${parsed.type.toUpperCase()} | 大小: ${formatSize(parsed.size)} | 段数: ${parsed.segments.length}`, 'info')
    } catch (error) {
      store.addLog(`文件解析失败: ${error.message}`, 'error')
    }
  } catch (error) {
    store.addLog(`操作失败: ${error.message}`, 'error')
  }
}

// 刷新串口列表
const refreshPorts = async () => {
  try {
    const result = await window.electronAPI.listPorts()
    if (result.success) {
      store.setPorts(result.ports)
      store.addLog(`发现 ${result.ports.length} 个串口设备`, 'info')
    } else {
      store.addLog(`获取串口列表失败: ${result.error}`, 'error')
    }
  } catch (error) {
    store.addLog(`操作失败: ${error.message}`, 'error')
  }
}

// 打开串口
const openPort = async () => {
  try {
    const result = await window.electronAPI.openPort(selectedPort.value, {
      baudRate: parseInt(baudRate.value)
    })

    if (result.success) {
      store.setPortOpen(true)
      store.setSelectedPort(selectedPort.value)
      store.setPortConfig({ baudRate: parseInt(baudRate.value) })
      store.addLog(`串口已打开: ${selectedPort.value} @ ${baudRate.value}`, 'success')

      // 初始化烧录协议
      flashProtocol = new FlashProtocol(window.electronAPI)
    } else {
      store.addLog(`串口打开失败: ${result.error}`, 'error')
    }
  } catch (error) {
    store.addLog(`操作失败: ${error.message}`, 'error')
  }
}

// 关闭串口
const closePort = async () => {
  try {
    const result = await window.electronAPI.closePort()
    if (result.success) {
      store.setPortOpen(false)
      store.addLog('串口已关闭', 'info')

      if (flashProtocol) {
        flashProtocol.destroy()
        flashProtocol = null
      }
    } else {
      store.addLog(`串口关闭失败: ${result.error}`, 'error')
    }
  } catch (error) {
    store.addLog(`操作失败: ${error.message}`, 'error')
  }
}

// 开始烧录
const startFlash = async () => {
  if (!flashProtocol) {
    store.addLog('请先连接设备', 'error')
    return
  }

  try {
    store.startFlashing()

    const firmware = store.firmwareData

    // 擦除 Flash
    store.addLog('正在擦除 Flash...', 'info')
    for (const segment of firmware.segments) {
      await flashProtocol.eraseFlash(segment.address, segment.size)
    }
    store.addLog('Flash 擦除完成', 'success')

    // 写入数据
    store.addLog('正在写入固件...', 'info')
    let totalWritten = 0

    for (const segment of firmware.segments) {
      await flashProtocol.writeBlock(segment.address, segment.data)
      totalWritten += segment.size
      store.updateProgress(totalWritten)
    }

    store.addLog('固件写入完成', 'success')

    // 校验
    store.addLog('正在校验...', 'info')
    for (const segment of firmware.segments) {
      await flashProtocol.verifyBlock(segment.address, segment.size)
    }
    store.addLog('校验通过', 'success')

    // 复位设备
    store.addLog('正在复位设备...', 'info')
    await flashProtocol.resetDevice()

    store.stopFlashing(true)
  } catch (error) {
    store.addLog(`烧录失败: ${error.message}`, 'error')
    store.stopFlashing(false)
  }
}

// 停止烧录
const stopFlash = () => {
  store.stopFlashing(false)
}

// 清空日志
const clearLogs = () => {
  store.clearLogs()
}

// 生命周期
onMounted(async () => {
  await refreshPorts()
})

onUnmounted(() => {
  if (flashProtocol) {
    flashProtocol.destroy()
  }
})
</script>

<style scoped>
.flasher-view {
  display: flex;
  flex-direction: column;
  height: 100%;
  width: 100%;
}

.content {
  flex: 1;
  padding: 16px;
  overflow-y: auto;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.card-content {
  padding: 20px;
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.card-header {
  padding: 16px 20px;
  display: flex;
  align-items: center;
  justify-content: space-between;
  border-bottom: 1px solid rgb(var(--mdui-color-surface-variant));
  font-weight: 500;
}

/* 文件卡片 */
.file-info {
  display: flex;
  align-items: center;
  gap: 16px;
}

.file-info mdui-icon-description {
  color: rgb(var(--mdui-color-primary));
}

.file-details {
  flex: 1;
}

.file-name {
  font-size: 16px;
  font-weight: 500;
  margin-bottom: 4px;
}

.file-meta {
  font-size: 14px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

/* 设备卡片 */
.device-selector {
  display: flex;
  gap: 8px;
}

/* 进度卡片 */
.progress-info {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.progress-text {
  display: flex;
  justify-content: space-between;
  align-items: center;
}

.progress-percent {
  font-size: 24px;
  font-weight: 600;
  color: rgb(var(--mdui-color-primary));
}

.progress-stats {
  font-size: 14px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.action-buttons {
  display: flex;
  justify-content: center;
}

/* 日志卡片 */
.log-card {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 200px;
}

.log-content {
  flex: 1;
  padding: 16px;
  overflow-y: auto;
  font-family: 'Menlo', 'Monaco', 'Courier New', monospace;
  font-size: 13px;
  background-color: rgba(0, 0, 0, 0.2);
}

.log-item {
  padding: 4px 0;
  line-height: 1.5;
}

.log-time {
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-right: 8px;
}

.log-message {
  color: rgb(var(--mdui-color-on-surface));
}

.log-info .log-message {
  color: rgb(var(--mdui-color-on-surface));
}

.log-success .log-message {
  color: #4caf50;
}

.log-error .log-message {
  color: #f44336;
}

.log-warning .log-message {
  color: #ff9800;
}
</style>

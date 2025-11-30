<template>
  <div class="config-view">
    <div class="config-content">
      <mdui-tabs :value="activeTab" @change="handleTabChange">
        <mdui-tab value="project">项目配置</mdui-tab>
        <mdui-tab value="tasks">任务配置</mdui-tab>
        <mdui-tab value="communication">通信配置</mdui-tab>
        <mdui-tab value="io">I/O 配置</mdui-tab>
      </mdui-tabs>

      <!-- 项目配置 -->
      <div class="tab-content" v-if="activeTab === 'project'">
        <mdui-card variant="outlined" class="config-section">
          <h3>基本信息</h3>
          <div class="form-grid">
            <mdui-text-field
              v-model="config.name"
              label="项目名称"
              required
            ></mdui-text-field>

            <mdui-text-field
              v-model="config.version"
              label="版本号"
            ></mdui-text-field>

            <mdui-text-field
              v-model="config.author"
              label="作者"
            ></mdui-text-field>

            <mdui-text-field
              v-model="config.description"
              label="描述"
              rows="2"
            ></mdui-text-field>
          </div>
        </mdui-card>

        <mdui-card variant="outlined" class="config-section">
          <h3>运行时配置</h3>
          <div class="form-grid">
            <mdui-select v-model="config.targetPlatform" label="目标平台">
              <mdui-menu-item value="ARM64-RTOS">ARM64-RTOS</mdui-menu-item>
              <mdui-menu-item value="x86-Linux">x86-Linux</mdui-menu-item>
              <mdui-menu-item value="Simulation">仿真模式</mdui-menu-item>
            </mdui-select>

            <mdui-text-field
              v-model.number="config.scanCycle"
              label="扫描周期 (ms)"
              type="number"
              min="1"
              max="1000"
            ></mdui-text-field>

            <mdui-text-field
              v-model.number="config.watchdogTime"
              label="看门狗时间 (ms)"
              type="number"
              min="10"
              max="10000"
            ></mdui-text-field>
          </div>
        </mdui-card>
      </div>

      <!-- 任务配置 -->
      <div class="tab-content" v-if="activeTab === 'tasks'">
        <div class="tasks-header">
          <mdui-button @click="addTask">
            <mdui-icon-add slot="icon"></mdui-icon-add>
            添加任务
          </mdui-button>
        </div>

        <div class="tasks-list">
          <mdui-card
            v-for="task in tasks"
            :key="task.id"
            variant="outlined"
            class="task-card"
          >
            <div class="task-header">
              <mdui-text-field
                :value="task.name"
                @input="updateTask(task.id, 'name', ($event.target as HTMLInputElement).value)"
                label="任务名称"
                style="flex: 1;"
              ></mdui-text-field>
              <mdui-button-icon @click="deleteTask(task.id)">
                <mdui-icon-delete></mdui-icon-delete>
              </mdui-button-icon>
            </div>

            <div class="form-grid">
              <mdui-text-field
                :value="task.priority"
                @input="updateTask(task.id, 'priority', parseInt(($event.target as HTMLInputElement).value))"
                label="优先级"
                type="number"
                min="1"
                max="255"
              ></mdui-text-field>

              <mdui-text-field
                :value="task.interval"
                @input="updateTask(task.id, 'interval', parseInt(($event.target as HTMLInputElement).value))"
                label="执行间隔 (ms)"
                type="number"
                min="1"
              ></mdui-text-field>
            </div>

            <h4>关联程序</h4>
            <div class="program-list">
              <mdui-chip
                v-for="programId in task.programs"
                :key="programId"
                deletable
                @delete="removeProgramFromTask(task.id, programId)"
              >
                {{ getProgramName(programId) }}
              </mdui-chip>
              <mdui-dropdown>
                <mdui-chip slot="trigger" icon="add">添加程序</mdui-chip>
                <mdui-menu>
                  <mdui-menu-item
                    v-for="program in availablePrograms(task)"
                    :key="program.id"
                    @click="addProgramToTask(task.id, program.id)"
                  >
                    {{ program.name }}
                  </mdui-menu-item>
                </mdui-menu>
              </mdui-dropdown>
            </div>
          </mdui-card>
        </div>
      </div>

      <!-- 通信配置 -->
      <div class="tab-content" v-if="activeTab === 'communication'">
        <mdui-card variant="outlined" class="config-section">
          <h3>连接设置</h3>
          <div class="form-grid">
            <mdui-select v-model="connection.type" label="连接方式">
              <mdui-menu-item value="tcp">TCP/IP</mdui-menu-item>
              <mdui-menu-item value="serial">串口</mdui-menu-item>
              <mdui-menu-item value="usb">USB</mdui-menu-item>
            </mdui-select>

            <template v-if="connection.type === 'tcp'">
              <mdui-text-field
                v-model="connection.host"
                label="IP 地址"
                placeholder="192.168.1.100"
              ></mdui-text-field>

              <mdui-text-field
                v-model.number="connection.port"
                label="端口"
                type="number"
                placeholder="502"
              ></mdui-text-field>
            </template>

            <template v-if="connection.type === 'serial'">
              <mdui-select v-model="connection.serialPort" label="串口">
                <mdui-menu-item value="COM1">COM1</mdui-menu-item>
                <mdui-menu-item value="COM2">COM2</mdui-menu-item>
                <mdui-menu-item value="COM3">COM3</mdui-menu-item>
                <mdui-menu-item value="/dev/ttyUSB0">/dev/ttyUSB0</mdui-menu-item>
              </mdui-select>

              <mdui-select v-model.number="connection.baudRate" label="波特率">
                <mdui-menu-item :value="9600">9600</mdui-menu-item>
                <mdui-menu-item :value="19200">19200</mdui-menu-item>
                <mdui-menu-item :value="38400">38400</mdui-menu-item>
                <mdui-menu-item :value="57600">57600</mdui-menu-item>
                <mdui-menu-item :value="115200">115200</mdui-menu-item>
              </mdui-select>
            </template>
          </div>

          <div class="connection-actions">
            <mdui-button variant="outlined" @click="testConnection">
              <mdui-icon-wifi-tethering slot="icon"></mdui-icon-wifi-tethering>
              测试连接
            </mdui-button>
          </div>
        </mdui-card>

        <mdui-card variant="outlined" class="config-section">
          <h3>协议配置</h3>
          <div class="protocol-list">
            <mdui-switch>Modbus TCP</mdui-switch>
            <mdui-switch>OPC UA</mdui-switch>
            <mdui-switch>PROFINET</mdui-switch>
          </div>
        </mdui-card>
      </div>

      <!-- I/O 配置 -->
      <div class="tab-content" v-if="activeTab === 'io'">
        <mdui-card variant="outlined" class="config-section">
          <h3>数字输入 (DI)</h3>
          <table class="io-table">
            <thead>
              <tr>
                <th>地址</th>
                <th>名称</th>
                <th>映射变量</th>
                <th>状态</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="i in 8" :key="`di-${i}`">
                <td>%IX0.{{ i - 1 }}</td>
                <td><mdui-text-field variant="outlined" placeholder="DI{{ i }}"></mdui-text-field></td>
                <td><mdui-text-field variant="outlined" placeholder="选择变量"></mdui-text-field></td>
                <td><span class="io-status off">OFF</span></td>
              </tr>
            </tbody>
          </table>
        </mdui-card>

        <mdui-card variant="outlined" class="config-section">
          <h3>数字输出 (DO)</h3>
          <table class="io-table">
            <thead>
              <tr>
                <th>地址</th>
                <th>名称</th>
                <th>映射变量</th>
                <th>状态</th>
              </tr>
            </thead>
            <tbody>
              <tr v-for="i in 8" :key="`do-${i}`">
                <td>%QX0.{{ i - 1 }}</td>
                <td><mdui-text-field variant="outlined" placeholder="DO{{ i }}"></mdui-text-field></td>
                <td><mdui-text-field variant="outlined" placeholder="选择变量"></mdui-text-field></td>
                <td><span class="io-status off">OFF</span></td>
              </tr>
            </tbody>
          </table>
        </mdui-card>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, reactive, computed } from 'vue'
import { useProjectStore } from '@/stores/project'
import type { TaskConfig, PLCConnection } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

import '@mdui/icons/add.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/wifi-tethering.js'

const projectStore = useProjectStore()

const activeTab = ref('project')

// 项目配置
const config = reactive({
  name: projectStore.currentProject?.config.name || '',
  version: projectStore.currentProject?.config.version || '1.0.0',
  author: projectStore.currentProject?.config.author || '',
  description: projectStore.currentProject?.config.description || '',
  targetPlatform: projectStore.currentProject?.config.targetPlatform || 'ARM64-RTOS',
  scanCycle: projectStore.currentProject?.config.scanCycle || 10,
  watchdogTime: projectStore.currentProject?.config.watchdogTime || 100
})

// 任务列表
const tasks = computed(() =>
  projectStore.currentProject?.resources[0]?.tasks || []
)

// 连接配置
const connection = reactive<PLCConnection>({
  type: 'tcp',
  host: '192.168.1.100',
  port: 502,
  serialPort: 'COM1',
  baudRate: 115200
})

function handleTabChange(e: CustomEvent) {
  activeTab.value = e.detail.value
}

function addTask() {
  const resource = projectStore.currentProject?.resources[0]
  if (resource) {
    const task: TaskConfig = {
      id: uuidv4(),
      name: `Task${resource.tasks.length + 1}`,
      priority: 10,
      interval: 100,
      programs: []
    }
    resource.tasks.push(task)
  }
}

function updateTask(taskId: string, field: keyof TaskConfig, value: any) {
  const task = tasks.value.find(t => t.id === taskId)
  if (task) {
    (task as any)[field] = value
  }
}

function deleteTask(taskId: string) {
  const resource = projectStore.currentProject?.resources[0]
  if (resource) {
    resource.tasks = resource.tasks.filter(t => t.id !== taskId)
  }
}

function getProgramName(programId: string): string {
  const pou = projectStore.getPOU(programId)
  return pou?.name || '未知'
}

function availablePrograms(task: TaskConfig) {
  return projectStore.programs.filter(p => !task.programs.includes(p.id))
}

function addProgramToTask(taskId: string, programId: string) {
  const task = tasks.value.find(t => t.id === taskId)
  if (task && !task.programs.includes(programId)) {
    task.programs.push(programId)
  }
}

function removeProgramFromTask(taskId: string, programId: string) {
  const task = tasks.value.find(t => t.id === taskId)
  if (task) {
    task.programs = task.programs.filter(id => id !== programId)
  }
}

function testConnection() {
  // TODO: 实现连接测试
  console.log('Testing connection...', connection)
}
</script>

<style scoped>
.config-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.config-content {
  flex: 1;
  overflow: auto;
}

.tab-content {
  padding: 16px;
}

.config-section {
  padding: 16px;
  margin-bottom: 16px;
}

.config-section h3 {
  margin: 0 0 16px 0;
  font-size: 16px;
  font-weight: 500;
}

.form-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 16px;
}

.tasks-header {
  margin-bottom: 16px;
}

.tasks-list {
  display: flex;
  flex-direction: column;
  gap: 16px;
}

.task-card {
  padding: 16px;
}

.task-header {
  display: flex;
  align-items: flex-start;
  gap: 8px;
  margin-bottom: 16px;
}

.task-card h4 {
  margin: 16px 0 8px 0;
  font-size: 14px;
  font-weight: 500;
}

.program-list {
  display: flex;
  flex-wrap: wrap;
  gap: 8px;
}

.connection-actions {
  margin-top: 16px;
}

.protocol-list {
  display: flex;
  flex-direction: column;
  gap: 12px;
}

.io-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.io-table th {
  text-align: left;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.io-table td {
  padding: 4px 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.io-table mdui-text-field {
  --mdui-comp-text-field-height: 32px;
}

.io-status {
  display: inline-block;
  padding: 2px 8px;
  border-radius: 4px;
  font-size: 12px;
}

.io-status.on {
  background-color: #4caf50;
  color: white;
}

.io-status.off {
  background-color: #9e9e9e;
  color: white;
}
</style>

<template>
  <div class="fbd-editor-view">
    <!-- 工具栏 -->
    <div class="fbd-toolbar">
      <mdui-segmented-button-group>
        <mdui-segmented-button value="select" :selected="currentTool === 'select'" @click="currentTool = 'select'">
          <mdui-icon-near-me slot="icon"></mdui-icon-near-me>
          选择
        </mdui-segmented-button>
        <mdui-segmented-button value="connect" :selected="currentTool === 'connect'" @click="currentTool = 'connect'">
          <mdui-icon-timeline slot="icon"></mdui-icon-timeline>
          连接
        </mdui-segmented-button>
      </mdui-segmented-button-group>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <!-- 元素工具 -->
      <mdui-dropdown>
        <mdui-button slot="trigger" variant="outlined">
          <mdui-icon-input slot="icon"></mdui-icon-input>
          输入
        </mdui-button>
        <mdui-menu>
          <mdui-menu-item @click="addElement('INPUT')">输入变量</mdui-menu-item>
        </mdui-menu>
      </mdui-dropdown>

      <mdui-dropdown>
        <mdui-button slot="trigger" variant="outlined">
          <mdui-icon-output slot="icon"></mdui-icon-output>
          输出
        </mdui-button>
        <mdui-menu>
          <mdui-menu-item @click="addElement('OUTPUT')">输出变量</mdui-menu-item>
        </mdui-menu>
      </mdui-dropdown>

      <mdui-dropdown>
        <mdui-button slot="trigger" variant="outlined">
          <mdui-icon-widgets slot="icon"></mdui-icon-widgets>
          功能块
        </mdui-button>
        <mdui-menu>
          <mdui-menu-item @click="addFunctionBlock('AND')">AND</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('OR')">OR</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('NOT')">NOT</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('XOR')">XOR</mdui-menu-item>
          <mdui-divider></mdui-divider>
          <mdui-menu-item @click="addFunctionBlock('ADD')">ADD</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('SUB')">SUB</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('MUL')">MUL</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('DIV')">DIV</mdui-menu-item>
          <mdui-divider></mdui-divider>
          <mdui-menu-item @click="addFunctionBlock('TON')">TON (定时器)</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('TOF')">TOF (关闭延时)</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('CTU')">CTU (计数器)</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('CTD')">CTD (递减计数器)</mdui-menu-item>
          <mdui-divider></mdui-divider>
          <mdui-menu-item @click="addFunctionBlock('SR')">SR (置位优先)</mdui-menu-item>
          <mdui-menu-item @click="addFunctionBlock('RS')">RS (复位优先)</mdui-menu-item>
        </mdui-menu>
      </mdui-dropdown>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <mdui-tooltip content="删除选中">
        <mdui-button-icon @click="deleteSelected" :disabled="!selectedElement">
          <mdui-icon-delete></mdui-icon-delete>
        </mdui-button-icon>
      </mdui-tooltip>

      <mdui-tooltip content="放大">
        <mdui-button-icon @click="zoomIn">
          <mdui-icon-zoom-in></mdui-icon-zoom-in>
        </mdui-button-icon>
      </mdui-tooltip>

      <mdui-tooltip content="缩小">
        <mdui-button-icon @click="zoomOut">
          <mdui-icon-zoom-out></mdui-icon-zoom-out>
        </mdui-button-icon>
      </mdui-tooltip>
    </div>

    <!-- FBD 画布 -->
    <div
      class="fbd-canvas"
      ref="canvasRef"
      @mousedown="handleCanvasMouseDown"
      @mousemove="handleCanvasMouseMove"
      @mouseup="handleCanvasMouseUp"
      :style="{ transform: `scale(${zoom})` }"
    >
      <!-- SVG 连接线层 -->
      <svg class="connections-layer">
        <path
          v-for="conn in diagram.connections"
          :key="conn.id"
          :d="getConnectionPath(conn)"
          class="fbd-connection"
          :class="{ selected: selectedConnection?.id === conn.id }"
          @click="selectConnection(conn)"
        />
        <!-- 正在绘制的连接线 -->
        <path
          v-if="drawingConnection"
          :d="drawingConnectionPath"
          class="fbd-connection drawing"
        />
      </svg>

      <!-- 元素层 -->
      <div
        v-for="element in diagram.elements"
        :key="element.id"
        class="fbd-block"
        :class="{ selected: selectedElement?.id === element.id }"
        :style="{ left: element.x + 'px', top: element.y + 'px', width: element.width + 'px' }"
        @mousedown.stop="handleElementMouseDown($event, element)"
        @dblclick="editElement(element)"
      >
        <div class="fbd-block-header">{{ element.name }}</div>
        <div class="fbd-block-body">
          <!-- 输入端口 -->
          <div class="fbd-ports-left">
            <div
              v-for="pin in element.inputs"
              :key="pin.id"
              class="fbd-port input"
              @mousedown.stop="startConnection($event, element, pin, 'input')"
              @mouseup="endConnection(element, pin, 'input')"
            >
              <span class="port-name">{{ pin.name }}</span>
            </div>
          </div>

          <!-- 输出端口 -->
          <div class="fbd-ports-right">
            <div
              v-for="pin in element.outputs"
              :key="pin.id"
              class="fbd-port output"
              @mousedown.stop="startConnection($event, element, pin, 'output')"
              @mouseup="endConnection(element, pin, 'output')"
            >
              <span class="port-name">{{ pin.name }}</span>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- 属性面板 -->
    <div class="property-panel" v-if="selectedElement">
      <h4>属性</h4>
      <mdui-text-field
        :value="selectedElement.name"
        @input="updateElementName($event)"
        label="名称"
      ></mdui-text-field>
      <div class="property-row">
        <span>X:</span>
        <mdui-text-field type="number" :value="selectedElement.x" @input="updateElementX($event)"></mdui-text-field>
        <span>Y:</span>
        <mdui-text-field type="number" :value="selectedElement.y" @input="updateElementY($event)"></mdui-text-field>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, reactive } from 'vue'
import { useRoute } from 'vue-router'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { FBDDiagram, FBDElement, FBDConnection, FBDPin } from '@/types/iec61131'
import { FBDElementType, DataType } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

import '@mdui/icons/near-me.js'
import '@mdui/icons/timeline.js'
import '@mdui/icons/input.js'
import '@mdui/icons/output.js'
import '@mdui/icons/widgets.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/zoom-in.js'
import '@mdui/icons/zoom-out.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const pouId = computed(() => route.params.id as string)
const pou = computed(() => projectStore.getPOU(pouId.value))

const canvasRef = ref<HTMLElement | null>(null)
const currentTool = ref('select')
const zoom = ref(1)
const selectedElement = ref<FBDElement | null>(null)
const selectedConnection = ref<FBDConnection | null>(null)

// 拖拽状态
const isDragging = ref(false)
const dragOffset = reactive({ x: 0, y: 0 })

// 连接绘制状态
const drawingConnection = ref(false)
const connectionStart = reactive({
  element: null as FBDElement | null,
  pin: null as FBDPin | null,
  type: '' as 'input' | 'output' | ''
})
const connectionEnd = reactive({ x: 0, y: 0 })

const diagram = ref<FBDDiagram>({
  elements: [],
  connections: []
})

onMounted(() => {
  if (pou.value && typeof pou.value.body === 'object') {
    diagram.value = pou.value.body as FBDDiagram
  }
})

const drawingConnectionPath = computed(() => {
  if (!connectionStart.element || !connectionStart.pin) return ''
  const startX = connectionStart.element.x + (connectionStart.type === 'output' ? connectionStart.element.width : 0)
  const startY = connectionStart.element.y + 50
  return `M ${startX} ${startY} L ${connectionEnd.x} ${connectionEnd.y}`
})

function getConnectionPath(conn: FBDConnection): string {
  const source = diagram.value.elements.find(e => e.id === conn.sourceElement)
  const target = diagram.value.elements.find(e => e.id === conn.targetElement)
  if (!source || !target) return ''

  const x1 = source.x + source.width
  const y1 = source.y + 50
  const x2 = target.x
  const y2 = target.y + 50

  // 贝塞尔曲线
  const cx = (x1 + x2) / 2
  return `M ${x1} ${y1} C ${cx} ${y1}, ${cx} ${y2}, ${x2} ${y2}`
}

function addElement(type: 'INPUT' | 'OUTPUT') {
  const element: FBDElement = {
    id: uuidv4(),
    type: type === 'INPUT' ? FBDElementType.INPUT : FBDElementType.OUTPUT,
    name: type === 'INPUT' ? 'Input' : 'Output',
    x: 100,
    y: 100,
    width: 80,
    height: 60,
    inputs: type === 'OUTPUT' ? [{ id: uuidv4(), name: 'IN', dataType: DataType.BOOL }] : [],
    outputs: type === 'INPUT' ? [{ id: uuidv4(), name: 'OUT', dataType: DataType.BOOL }] : []
  }
  diagram.value.elements.push(element)
  selectedElement.value = element
  saveDiagram()
}

function addFunctionBlock(name: string) {
  const configs: Record<string, { inputs: string[], outputs: string[] }> = {
    'AND': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'OR': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'NOT': { inputs: ['IN'], outputs: ['OUT'] },
    'XOR': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'ADD': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'SUB': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'MUL': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'DIV': { inputs: ['IN1', 'IN2'], outputs: ['OUT'] },
    'TON': { inputs: ['IN', 'PT'], outputs: ['Q', 'ET'] },
    'TOF': { inputs: ['IN', 'PT'], outputs: ['Q', 'ET'] },
    'CTU': { inputs: ['CU', 'R', 'PV'], outputs: ['Q', 'CV'] },
    'CTD': { inputs: ['CD', 'LD', 'PV'], outputs: ['Q', 'CV'] },
    'SR': { inputs: ['S1', 'R'], outputs: ['Q1'] },
    'RS': { inputs: ['S', 'R1'], outputs: ['Q1'] }
  }

  const config = configs[name] || { inputs: ['IN'], outputs: ['OUT'] }

  const element: FBDElement = {
    id: uuidv4(),
    type: FBDElementType.FUNCTION_BLOCK,
    name: name,
    x: 200,
    y: 100,
    width: 100,
    height: 60 + Math.max(config.inputs.length, config.outputs.length) * 20,
    inputs: config.inputs.map(n => ({ id: uuidv4(), name: n, dataType: DataType.BOOL })),
    outputs: config.outputs.map(n => ({ id: uuidv4(), name: n, dataType: DataType.BOOL }))
  }

  diagram.value.elements.push(element)
  selectedElement.value = element
  saveDiagram()
}

function handleCanvasMouseDown(e: MouseEvent) {
  if (currentTool.value === 'select') {
    selectedElement.value = null
    selectedConnection.value = null
  }
}

function handleCanvasMouseMove(e: MouseEvent) {
  if (isDragging.value && selectedElement.value) {
    const rect = canvasRef.value!.getBoundingClientRect()
    selectedElement.value.x = (e.clientX - rect.left) / zoom.value - dragOffset.x
    selectedElement.value.y = (e.clientY - rect.top) / zoom.value - dragOffset.y
  }

  if (drawingConnection.value) {
    const rect = canvasRef.value!.getBoundingClientRect()
    connectionEnd.x = (e.clientX - rect.left) / zoom.value
    connectionEnd.y = (e.clientY - rect.top) / zoom.value
  }
}

function handleCanvasMouseUp() {
  isDragging.value = false
  if (drawingConnection.value) {
    drawingConnection.value = false
    connectionStart.element = null
    connectionStart.pin = null
    connectionStart.type = ''
  }
}

function handleElementMouseDown(e: MouseEvent, element: FBDElement) {
  selectedElement.value = element
  selectedConnection.value = null

  if (currentTool.value === 'select') {
    isDragging.value = true
    const rect = canvasRef.value!.getBoundingClientRect()
    dragOffset.x = (e.clientX - rect.left) / zoom.value - element.x
    dragOffset.y = (e.clientY - rect.top) / zoom.value - element.y
  }
}

function startConnection(e: MouseEvent, element: FBDElement, pin: FBDPin, type: 'input' | 'output') {
  if (currentTool.value !== 'connect' && currentTool.value !== 'select') return
  if (type === 'input') return // 只能从输出端开始

  drawingConnection.value = true
  connectionStart.element = element
  connectionStart.pin = pin
  connectionStart.type = type

  const rect = canvasRef.value!.getBoundingClientRect()
  connectionEnd.x = (e.clientX - rect.left) / zoom.value
  connectionEnd.y = (e.clientY - rect.top) / zoom.value
}

function endConnection(element: FBDElement, pin: FBDPin, type: 'input' | 'output') {
  if (!drawingConnection.value || !connectionStart.element || !connectionStart.pin) return
  if (type !== 'input') return // 只能连接到输入端
  if (connectionStart.element.id === element.id) return // 不能连接到自己

  const connection: FBDConnection = {
    id: uuidv4(),
    sourceElement: connectionStart.element.id,
    sourcePin: connectionStart.pin.id,
    targetElement: element.id,
    targetPin: pin.id
  }

  diagram.value.connections.push(connection)
  drawingConnection.value = false
  connectionStart.element = null
  connectionStart.pin = null
  connectionStart.type = ''
  saveDiagram()
}

function selectConnection(conn: FBDConnection) {
  selectedConnection.value = conn
  selectedElement.value = null
}

function deleteSelected() {
  if (selectedElement.value) {
    // 删除相关连接
    diagram.value.connections = diagram.value.connections.filter(
      c => c.sourceElement !== selectedElement.value!.id && c.targetElement !== selectedElement.value!.id
    )
    // 删除元素
    diagram.value.elements = diagram.value.elements.filter(e => e.id !== selectedElement.value!.id)
    selectedElement.value = null
    saveDiagram()
  }

  if (selectedConnection.value) {
    diagram.value.connections = diagram.value.connections.filter(c => c.id !== selectedConnection.value!.id)
    selectedConnection.value = null
    saveDiagram()
  }
}

function editElement(element: FBDElement) {
  // TODO: 打开元素编辑对话框
}

function updateElementName(e: Event) {
  if (selectedElement.value) {
    selectedElement.value.name = (e.target as HTMLInputElement).value
    saveDiagram()
  }
}

function updateElementX(e: Event) {
  if (selectedElement.value) {
    selectedElement.value.x = parseInt((e.target as HTMLInputElement).value) || 0
    saveDiagram()
  }
}

function updateElementY(e: Event) {
  if (selectedElement.value) {
    selectedElement.value.y = parseInt((e.target as HTMLInputElement).value) || 0
    saveDiagram()
  }
}

function zoomIn() {
  zoom.value = Math.min(2, zoom.value + 0.1)
}

function zoomOut() {
  zoom.value = Math.max(0.5, zoom.value - 0.1)
}

function saveDiagram() {
  if (pou.value) {
    projectStore.updatePOU(pouId.value, { body: diagram.value })
    editorStore.markModified(`${pouId.value}-pou`, true)
  }
}
</script>

<style scoped>
.fbd-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.fbd-toolbar {
  display: flex;
  align-items: center;
  padding: 8px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  background-color: rgb(var(--mdui-color-surface-container));
  gap: 8px;
  flex-wrap: wrap;
}

.fbd-canvas {
  flex: 1;
  position: relative;
  overflow: auto;
  background-color: rgb(var(--mdui-color-surface));
  background-image:
    linear-gradient(rgba(var(--mdui-color-outline-variant), 0.3) 1px, transparent 1px),
    linear-gradient(90deg, rgba(var(--mdui-color-outline-variant), 0.3) 1px, transparent 1px);
  background-size: 20px 20px;
  transform-origin: top left;
}

.connections-layer {
  position: absolute;
  top: 0;
  left: 0;
  width: 100%;
  height: 100%;
  pointer-events: none;
}

.connections-layer path {
  pointer-events: stroke;
}

.fbd-connection {
  stroke: rgb(var(--mdui-color-outline));
  stroke-width: 2;
  fill: none;
  cursor: pointer;
}

.fbd-connection.selected {
  stroke: rgb(var(--mdui-color-primary));
  stroke-width: 3;
}

.fbd-connection.drawing {
  stroke: rgb(var(--mdui-color-primary));
  stroke-dasharray: 5, 5;
}

.fbd-block {
  position: absolute;
  background-color: rgb(var(--mdui-color-surface-container));
  border: 2px solid rgb(var(--mdui-color-outline));
  border-radius: 4px;
  cursor: move;
  user-select: none;
}

.fbd-block.selected {
  border-color: rgb(var(--mdui-color-primary));
  box-shadow: 0 0 0 2px rgba(var(--mdui-color-primary), 0.3);
}

.fbd-block-header {
  background-color: rgb(var(--mdui-color-primary));
  color: rgb(var(--mdui-color-on-primary));
  padding: 4px 8px;
  font-size: 12px;
  font-weight: 500;
  border-radius: 2px 2px 0 0;
  text-align: center;
}

.fbd-block-body {
  display: flex;
  justify-content: space-between;
  padding: 8px;
  min-height: 40px;
}

.fbd-ports-left,
.fbd-ports-right {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.fbd-port {
  display: flex;
  align-items: center;
  gap: 4px;
  cursor: crosshair;
}

.fbd-port::before {
  content: '';
  width: 10px;
  height: 10px;
  border-radius: 50%;
  background-color: rgb(var(--mdui-color-outline));
  border: 2px solid rgb(var(--mdui-color-surface));
}

.fbd-port:hover::before {
  background-color: rgb(var(--mdui-color-primary));
}

.fbd-port.input {
  flex-direction: row;
}

.fbd-port.output {
  flex-direction: row-reverse;
}

.port-name {
  font-size: 10px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.property-panel {
  position: absolute;
  right: 16px;
  top: 60px;
  width: 200px;
  padding: 16px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-radius: 8px;
  box-shadow: var(--mdui-elevation-level2);
}

.property-panel h4 {
  margin: 0 0 16px 0;
  font-size: 14px;
}

.property-row {
  display: flex;
  align-items: center;
  gap: 8px;
  margin-top: 8px;
}

.property-row span {
  font-size: 12px;
}

.property-row mdui-text-field {
  flex: 1;
}
</style>

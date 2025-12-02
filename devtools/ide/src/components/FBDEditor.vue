<template>
  <div class="fbd-editor" ref="containerRef">
    <div class="editor-toolbar">
      <!-- 输入/输出 -->
      <mdui-button-icon @click="addBlock('INPUT')">
        <mdui-icon-login></mdui-icon-login>
      </mdui-button-icon>
      <mdui-tooltip content="输入">
        <span class="toolbar-label">IN</span>
      </mdui-tooltip>

      <mdui-button-icon @click="addBlock('OUTPUT')">
        <mdui-icon-logout></mdui-icon-logout>
      </mdui-button-icon>
      <mdui-tooltip content="输出">
        <span class="toolbar-label">OUT</span>
      </mdui-tooltip>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <!-- 逻辑门 -->
      <mdui-button-icon @click="addBlock('AND')">
        <mdui-icon-call-merge></mdui-icon-call-merge>
      </mdui-button-icon>
      <span class="toolbar-label">AND</span>

      <mdui-button-icon @click="addBlock('OR')">
        <mdui-icon-call-split></mdui-icon-call-split>
      </mdui-button-icon>
      <span class="toolbar-label">OR</span>

      <mdui-button-icon @click="addBlock('NOT')">
        <mdui-icon-block></mdui-icon-block>
      </mdui-button-icon>
      <span class="toolbar-label">NOT</span>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <!-- 比较器 -->
      <mdui-button-icon @click="addBlock('GT')">
        <mdui-icon-chevron-right></mdui-icon-chevron-right>
      </mdui-button-icon>
      <span class="toolbar-label">&gt;</span>

      <mdui-button-icon @click="addBlock('LT')">
        <mdui-icon-chevron-left></mdui-icon-chevron-left>
      </mdui-button-icon>
      <span class="toolbar-label">&lt;</span>

      <mdui-button-icon @click="addBlock('EQ')">
        <mdui-icon-drag-handle></mdui-icon-drag-handle>
      </mdui-button-icon>
      <span class="toolbar-label">=</span>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <!-- 数学运算 -->
      <mdui-button-icon @click="addBlock('ADD')">
        <mdui-icon-add></mdui-icon-add>
      </mdui-button-icon>
      <span class="toolbar-label">+</span>

      <mdui-button-icon @click="addBlock('SUB')">
        <mdui-icon-remove></mdui-icon-remove>
      </mdui-button-icon>
      <span class="toolbar-label">-</span>

      <mdui-button-icon @click="addBlock('MUL')">
        <mdui-icon-close></mdui-icon-close>
      </mdui-button-icon>
      <span class="toolbar-label">×</span>

      <mdui-button-icon @click="addBlock('DIV')">
        <mdui-icon-safety-divider></mdui-icon-safety-divider>
      </mdui-button-icon>
      <span class="toolbar-label">÷</span>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <!-- 定时器/计数器 -->
      <mdui-button-icon @click="addBlock('TON')">
        <mdui-icon-timer></mdui-icon-timer>
      </mdui-button-icon>
      <span class="toolbar-label">TON</span>

      <mdui-button-icon @click="addBlock('CTU')">
        <mdui-icon-exposure-plus-1></mdui-icon-exposure-plus-1>
      </mdui-button-icon>
      <span class="toolbar-label">CTU</span>

      <div style="flex: 1;"></div>

      <mdui-button-icon @click="deleteSelected" :disabled="!selectedNode">
        <mdui-icon-delete></mdui-icon-delete>
      </mdui-button-icon>

      <mdui-button-icon @click="zoomIn">
        <mdui-icon-zoom-in></mdui-icon-zoom-in>
      </mdui-button-icon>
      <mdui-button-icon @click="zoomOut">
        <mdui-icon-zoom-out></mdui-icon-zoom-out>
      </mdui-button-icon>
    </div>

    <div class="editor-canvas" ref="canvasRef"></div>

    <!-- 属性面板 -->
    <div class="properties-panel" v-if="selectedNode">
      <h4>{{ getBlockName(selectedNode.type) }}</h4>
      <mdui-text-field
        v-if="selectedNode.type === 'INPUT' || selectedNode.type === 'OUTPUT'"
        label="变量名"
        :value="selectedNode.variable || ''"
        @input="updateNodeProperty('variable', $event.target.value)"
      ></mdui-text-field>
      <mdui-text-field
        v-if="selectedNode.type === 'TON'"
        label="延时 (ms)"
        :value="selectedNode.preset || '1000'"
        @input="updateNodeProperty('preset', $event.target.value)"
        type="number"
      ></mdui-text-field>
      <mdui-text-field
        v-if="selectedNode.type === 'CTU'"
        label="预设值"
        :value="selectedNode.preset || '10'"
        @input="updateNodeProperty('preset', $event.target.value)"
        type="number"
      ></mdui-text-field>
      <mdui-text-field
        label="注释"
        :value="selectedNode.comment || ''"
        @input="updateNodeProperty('comment', $event.target.value)"
      ></mdui-text-field>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch } from 'vue'
import Konva from 'konva'

// 图标导入
import '@mdui/icons/login.js'
import '@mdui/icons/logout.js'
import '@mdui/icons/call-merge.js'
import '@mdui/icons/call-split.js'
import '@mdui/icons/block.js'
import '@mdui/icons/chevron-right.js'
import '@mdui/icons/chevron-left.js'
import '@mdui/icons/drag-handle.js'
import '@mdui/icons/add.js'
import '@mdui/icons/remove.js'
import '@mdui/icons/close.js'
import '@mdui/icons/safety-divider.js'
import '@mdui/icons/timer.js'
import '@mdui/icons/exposure-plus-1.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/zoom-in.js'
import '@mdui/icons/zoom-out.js'

const props = defineProps({
  modelValue: {
    type: Object,
    default: () => ({ blocks: [], connections: [] })
  }
})

const emit = defineEmits(['update:modelValue', 'change'])

// Refs
const containerRef = ref(null)
const canvasRef = ref(null)

// Konva
let stage = null
let layer = null
let connectionLayer = null
let transformer = null

// 状态
const selectedNode = ref(null)
const scale = ref(1)
const isConnecting = ref(false)
const connectionStart = ref(null)

// 块配置
const BLOCK_CONFIG = {
  INPUT: { inputs: 0, outputs: 1, width: 80, height: 40, color: '#4caf50' },
  OUTPUT: { inputs: 1, outputs: 0, width: 80, height: 40, color: '#f44336' },
  AND: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#2196f3' },
  OR: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#2196f3' },
  NOT: { inputs: 1, outputs: 1, width: 50, height: 40, color: '#2196f3' },
  GT: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#9c27b0' },
  LT: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#9c27b0' },
  EQ: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#9c27b0' },
  ADD: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#ff9800' },
  SUB: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#ff9800' },
  MUL: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#ff9800' },
  DIV: { inputs: 2, outputs: 1, width: 60, height: 50, color: '#ff9800' },
  TON: { inputs: 2, outputs: 2, width: 80, height: 60, color: '#795548' },
  CTU: { inputs: 2, outputs: 2, width: 80, height: 60, color: '#795548' }
}

const BLOCK_NAMES = {
  INPUT: '输入',
  OUTPUT: '输出',
  AND: '与门',
  OR: '或门',
  NOT: '非门',
  GT: '大于',
  LT: '小于',
  EQ: '等于',
  ADD: '加法',
  SUB: '减法',
  MUL: '乘法',
  DIV: '除法',
  TON: '延时接通',
  CTU: '递增计数器'
}

function getBlockName(type) {
  return BLOCK_NAMES[type] || type
}

onMounted(() => {
  initCanvas()
  renderDiagram()
})

onUnmounted(() => {
  if (stage) {
    stage.destroy()
  }
})

function initCanvas() {
  const container = canvasRef.value
  const width = container.clientWidth
  const height = container.clientHeight

  stage = new Konva.Stage({
    container: container,
    width: width,
    height: height,
    draggable: true
  })

  connectionLayer = new Konva.Layer()
  layer = new Konva.Layer()
  stage.add(connectionLayer)
  stage.add(layer)

  transformer = new Konva.Transformer({
    enabledAnchors: [],
    rotateEnabled: false
  })
  layer.add(transformer)

  // 绘制网格
  drawGrid()

  // 点击空白取消选择
  stage.on('click', (e) => {
    if (e.target === stage || e.target.getParent() === connectionLayer) {
      selectedNode.value = null
      transformer.nodes([])
      layer.batchDraw()
    }
  })

  // 缩放
  stage.on('wheel', (e) => {
    e.evt.preventDefault()
    const oldScale = stage.scaleX()
    const pointer = stage.getPointerPosition()
    const mousePointTo = {
      x: (pointer.x - stage.x()) / oldScale,
      y: (pointer.y - stage.y()) / oldScale
    }
    const direction = e.evt.deltaY > 0 ? -1 : 1
    const newScale = direction > 0 ? oldScale * 1.1 : oldScale / 1.1
    stage.scale({ x: newScale, y: newScale })
    scale.value = newScale
    const newPos = {
      x: pointer.x - mousePointTo.x * newScale,
      y: pointer.y - mousePointTo.y * newScale
    }
    stage.position(newPos)
    layer.batchDraw()
  })

  // 窗口大小变化
  const resizeObserver = new ResizeObserver(() => {
    stage.width(container.clientWidth)
    stage.height(container.clientHeight)
    drawGrid()
    layer.batchDraw()
  })
  resizeObserver.observe(container)
}

function drawGrid() {
  // 简化：不绘制网格以提高性能
}

function renderDiagram() {
  // 清除旧元素���保留 transformer）
  layer.find('.block').forEach(node => node.destroy())
  connectionLayer.find('.connection').forEach(node => node.destroy())

  const { blocks = [], connections = [] } = props.modelValue

  // 绘制连接线
  connections.forEach(conn => {
    renderConnection(conn)
  })

  // 绘制块
  blocks.forEach((block, index) => {
    renderBlock(block, index)
  })

  connectionLayer.batchDraw()
  layer.batchDraw()
}

function renderBlock(block, index) {
  const config = BLOCK_CONFIG[block.type] || BLOCK_CONFIG.AND
  const { width, height, color, inputs, outputs } = config

  const group = new Konva.Group({
    x: block.x || 100 + index * 120,
    y: block.y || 100,
    draggable: true,
    name: 'block',
    id: `block-${index}`
  })

  // 背景
  group.add(new Konva.Rect({
    width: width,
    height: height,
    fill: 'white',
    stroke: color,
    strokeWidth: 2,
    cornerRadius: 4,
    shadowColor: 'rgba(0,0,0,0.1)',
    shadowBlur: 4,
    shadowOffset: { x: 2, y: 2 }
  }))

  // 类型标签
  group.add(new Konva.Text({
    x: 0,
    y: height / 2 - 8,
    width: width,
    text: block.type,
    fontSize: 14,
    fontStyle: 'bold',
    fill: color,
    align: 'center'
  }))

  // 变量名（如果有）
  if (block.variable) {
    group.add(new Konva.Text({
      x: 0,
      y: -18,
      width: width,
      text: block.variable,
      fontSize: 11,
      fill: '#666',
      align: 'center'
    }))
  }

  // 输入端口
  for (let i = 0; i < inputs; i++) {
    const portY = height / (inputs + 1) * (i + 1)
    const port = new Konva.Circle({
      x: 0,
      y: portY,
      radius: 5,
      fill: 'white',
      stroke: color,
      strokeWidth: 2,
      name: 'input-port'
    })
    port.on('click', (e) => {
      e.cancelBubble = true
      handlePortClick(index, 'input', i)
    })
    group.add(port)
  }

  // 输出端口
  for (let i = 0; i < outputs; i++) {
    const portY = height / (outputs + 1) * (i + 1)
    const port = new Konva.Circle({
      x: width,
      y: portY,
      radius: 5,
      fill: color,
      stroke: color,
      strokeWidth: 2,
      name: 'output-port'
    })
    port.on('click', (e) => {
      e.cancelBubble = true
      handlePortClick(index, 'output', i)
    })
    group.add(port)
  }

  // 选中事件
  group.on('click', (e) => {
    e.cancelBubble = true
    selectedNode.value = { ...block, index }
    transformer.nodes([group])
    layer.batchDraw()
  })

  // 拖动事件
  group.on('dragend', () => {
    const data = { ...props.modelValue }
    data.blocks[index].x = group.x()
    data.blocks[index].y = group.y()
    emit('update:modelValue', data)
    renderConnections()
  })

  group.on('dragmove', () => {
    renderConnections()
  })

  layer.add(group)
}

function renderConnection(conn) {
  const { blocks = [] } = props.modelValue
  const fromBlock = blocks[conn.from.block]
  const toBlock = blocks[conn.to.block]

  if (!fromBlock || !toBlock) return

  const fromConfig = BLOCK_CONFIG[fromBlock.type]
  const toConfig = BLOCK_CONFIG[toBlock.type]

  const fromX = (fromBlock.x || 100) + fromConfig.width
  const fromY = (fromBlock.y || 100) + fromConfig.height / (fromConfig.outputs + 1) * (conn.from.port + 1)
  const toX = toBlock.x || 100
  const toY = (toBlock.y || 100) + toConfig.height / (toConfig.inputs + 1) * (conn.to.port + 1)

  // 贝塞尔曲线
  const midX = (fromX + toX) / 2
  const line = new Konva.Line({
    points: [fromX, fromY, midX, fromY, midX, toY, toX, toY],
    stroke: '#666',
    strokeWidth: 2,
    tension: 0.5,
    name: 'connection'
  })

  connectionLayer.add(line)
}

function renderConnections() {
  connectionLayer.find('.connection').forEach(node => node.destroy())
  const { connections = [] } = props.modelValue
  connections.forEach(conn => renderConnection(conn))
  connectionLayer.batchDraw()
}

function handlePortClick(blockIndex, portType, portIndex) {
  if (!isConnecting.value) {
    if (portType === 'output') {
      isConnecting.value = true
      connectionStart.value = { block: blockIndex, port: portIndex }
    }
  } else {
    if (portType === 'input' && connectionStart.value) {
      const data = { ...props.modelValue }
      data.connections = data.connections || []
      data.connections.push({
        from: connectionStart.value,
        to: { block: blockIndex, port: portIndex }
      })
      emit('update:modelValue', data)
      renderDiagram()
    }
    isConnecting.value = false
    connectionStart.value = null
  }
}

function addBlock(type) {
  const data = { ...props.modelValue }
  data.blocks = data.blocks || []

  const newBlock = {
    type,
    x: 100 + data.blocks.length * 100,
    y: 100 + (data.blocks.length % 3) * 80,
    variable: type === 'INPUT' ? `I${data.blocks.filter(b => b.type === 'INPUT').length}` :
              type === 'OUTPUT' ? `Q${data.blocks.filter(b => b.type === 'OUTPUT').length}` : ''
  }

  data.blocks.push(newBlock)
  emit('update:modelValue', data)
  renderDiagram()
}

function deleteSelected() {
  if (!selectedNode.value) return

  const data = { ...props.modelValue }
  const index = selectedNode.value.index

  // 删除相关连接
  data.connections = (data.connections || []).filter(conn =>
    conn.from.block !== index && conn.to.block !== index
  )

  // 更新连接索引
  data.connections = data.connections.map(conn => ({
    from: {
      block: conn.from.block > index ? conn.from.block - 1 : conn.from.block,
      port: conn.from.port
    },
    to: {
      block: conn.to.block > index ? conn.to.block - 1 : conn.to.block,
      port: conn.to.port
    }
  }))

  // 删除块
  data.blocks.splice(index, 1)

  selectedNode.value = null
  transformer.nodes([])
  emit('update:modelValue', data)
  renderDiagram()
}

function updateNodeProperty(key, value) {
  if (!selectedNode.value) return

  const data = { ...props.modelValue }
  data.blocks[selectedNode.value.index][key] = value
  emit('update:modelValue', data)
  renderDiagram()
}

function zoomIn() {
  const newScale = scale.value * 1.2
  stage.scale({ x: newScale, y: newScale })
  scale.value = newScale
  layer.batchDraw()
  connectionLayer.batchDraw()
}

function zoomOut() {
  const newScale = scale.value / 1.2
  stage.scale({ x: newScale, y: newScale })
  scale.value = newScale
  layer.batchDraw()
  connectionLayer.batchDraw()
}

watch(() => props.modelValue, () => {
  if (stage) {
    renderDiagram()
  }
}, { deep: true })
</script>

<style scoped>
.fbd-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fafafa;
  position: relative;
}

.editor-toolbar {
  display: flex;
  align-items: center;
  padding: 8px;
  background: white;
  border-bottom: 1px solid #e0e0e0;
  gap: 4px;
  flex-wrap: wrap;
}

.toolbar-label {
  font-size: 11px;
  color: #666;
  margin-left: -4px;
  margin-right: 8px;
}

.editor-canvas {
  flex: 1;
  overflow: hidden;
  background: #f5f5f5;
}

.properties-panel {
  position: absolute;
  right: 16px;
  top: 60px;
  width: 200px;
  padding: 16px;
  background: white;
  border-radius: 8px;
  box-shadow: 0 2px 8px rgba(0, 0, 0, 0.15);
}

.properties-panel h4 {
  margin: 0 0 12px;
  font-size: 14px;
  font-weight: 500;
}
</style>

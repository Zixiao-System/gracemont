<template>
  <div class="ld-editor" ref="containerRef">
    <div class="editor-toolbar">
      <mdui-button-icon @click="addContact('NO')">
        <mdui-icon-radio-button-unchecked></mdui-icon-radio-button-unchecked>
      </mdui-button-icon>
      <mdui-tooltip content="常开触点">
        <span class="toolbar-label">NO</span>
      </mdui-tooltip>

      <mdui-button-icon @click="addContact('NC')">
        <mdui-icon-radio-button-checked></mdui-icon-radio-button-checked>
      </mdui-button-icon>
      <mdui-tooltip content="常闭触点">
        <span class="toolbar-label">NC</span>
      </mdui-tooltip>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <mdui-button-icon @click="addCoil('COIL')">
        <mdui-icon-circle></mdui-icon-circle>
      </mdui-button-icon>
      <mdui-tooltip content="线圈">
        <span class="toolbar-label">( )</span>
      </mdui-tooltip>

      <mdui-button-icon @click="addCoil('SET')">
        <mdui-icon-check-circle></mdui-icon-check-circle>
      </mdui-button-icon>
      <mdui-tooltip content="置位线圈">
        <span class="toolbar-label">(S)</span>
      </mdui-tooltip>

      <mdui-button-icon @click="addCoil('RESET')">
        <mdui-icon-cancel></mdui-icon-cancel>
      </mdui-button-icon>
      <mdui-tooltip content="复位线圈">
        <span class="toolbar-label">(R)</span>
      </mdui-tooltip>

      <mdui-divider vertical style="height: 24px; margin: 0 8px;"></mdui-divider>

      <mdui-button-icon @click="addRung">
        <mdui-icon-add></mdui-icon-add>
      </mdui-button-icon>
      <mdui-tooltip content="添加梯级">
        <span class="toolbar-label">Rung</span>
      </mdui-tooltip>

      <mdui-button-icon @click="deleteSelected" :disabled="!selectedElement">
        <mdui-icon-delete></mdui-icon-delete>
      </mdui-button-icon>

      <div style="flex: 1;"></div>

      <mdui-button-icon @click="zoomIn">
        <mdui-icon-zoom-in></mdui-icon-zoom-in>
      </mdui-button-icon>
      <mdui-button-icon @click="zoomOut">
        <mdui-icon-zoom-out></mdui-icon-zoom-out>
      </mdui-button-icon>
      <mdui-button-icon @click="resetZoom">
        <mdui-icon-fit-screen></mdui-icon-fit-screen>
      </mdui-button-icon>
    </div>

    <div class="editor-canvas" ref="canvasRef"></div>

    <!-- 属性面板 -->
    <div class="properties-panel" v-if="selectedElement">
      <h4>属性</h4>
      <mdui-text-field
        label="变量名"
        :value="selectedElement.attrs?.variable || ''"
        @input="updateProperty('variable', $event.target.value)"
      ></mdui-text-field>
      <mdui-text-field
        label="注释"
        :value="selectedElement.attrs?.comment || ''"
        @input="updateProperty('comment', $event.target.value)"
      ></mdui-text-field>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted, onUnmounted, watch } from 'vue'
import Konva from 'konva'

// 图标导入
import '@mdui/icons/radio-button-unchecked.js'
import '@mdui/icons/radio-button-checked.js'
import '@mdui/icons/circle.js'
import '@mdui/icons/check-circle.js'
import '@mdui/icons/cancel.js'
import '@mdui/icons/add.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/zoom-in.js'
import '@mdui/icons/zoom-out.js'
import '@mdui/icons/fit-screen.js'

const props = defineProps({
  modelValue: {
    type: Object,
    default: () => ({ rungs: [] })
  }
})

const emit = defineEmits(['update:modelValue', 'change'])

// Refs
const containerRef = ref(null)
const canvasRef = ref(null)

// Konva
let stage = null
let layer = null
let transformer = null

// 状态
const selectedElement = ref(null)
const scale = ref(1)

// 常量
const RUNG_HEIGHT = 80
const RUNG_PADDING = 20
const ELEMENT_WIDTH = 60
const ELEMENT_HEIGHT = 40
const RAIL_WIDTH = 20

// 颜色
const COLORS = {
  rail: '#333',
  wire: '#333',
  contact: '#1976d2',
  coil: '#4caf50',
  selected: '#ff9800',
  text: '#333'
}

onMounted(() => {
  initCanvas()
  renderLadder()
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

  layer = new Konva.Layer()
  stage.add(layer)

  // 创建 transformer
  transformer = new Konva.Transformer({
    enabledAnchors: [],
    rotateEnabled: false
  })
  layer.add(transformer)

  // 点击空白区域取消选择
  stage.on('click', (e) => {
    if (e.target === stage) {
      selectedElement.value = null
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
    layer.batchDraw()
  })
  resizeObserver.observe(container)
}

function renderLadder() {
  layer.destroyChildren()
  layer.add(transformer)

  const rungs = props.modelValue.rungs || []

  // 绘制左右电源轨
  const height = Math.max(rungs.length * RUNG_HEIGHT + RUNG_PADDING * 2, 400)

  // 左轨
  layer.add(new Konva.Rect({
    x: RUNG_PADDING,
    y: RUNG_PADDING,
    width: RAIL_WIDTH,
    height: height,
    fill: COLORS.rail
  }))

  // 右轨
  layer.add(new Konva.Rect({
    x: stage.width() - RUNG_PADDING - RAIL_WIDTH,
    y: RUNG_PADDING,
    width: RAIL_WIDTH,
    height: height,
    fill: COLORS.rail
  }))

  // 绘制每个梯级
  rungs.forEach((rung, rungIndex) => {
    renderRung(rung, rungIndex)
  })

  layer.batchDraw()
}

function renderRung(rung, rungIndex) {
  const y = RUNG_PADDING + rungIndex * RUNG_HEIGHT + RUNG_HEIGHT / 2
  const leftX = RUNG_PADDING + RAIL_WIDTH
  const rightX = stage.width() - RUNG_PADDING - RAIL_WIDTH

  // 绘制横线
  layer.add(new Konva.Line({
    points: [leftX, y, rightX, y],
    stroke: COLORS.wire,
    strokeWidth: 2
  }))

  // 绘制梯级编号
  layer.add(new Konva.Text({
    x: 5,
    y: y - 10,
    text: String(rungIndex + 1),
    fontSize: 14,
    fill: '#666'
  }))

  // 绘制元素
  const elements = rung.elements || []
  const spacing = (rightX - leftX - ELEMENT_WIDTH) / Math.max(elements.length, 1)

  elements.forEach((element, elementIndex) => {
    const x = leftX + spacing * elementIndex + spacing / 2
    renderElement(element, x, y, rungIndex, elementIndex)
  })
}

function renderElement(element, x, y, rungIndex, elementIndex) {
  const group = new Konva.Group({
    x: x,
    y: y - ELEMENT_HEIGHT / 2,
    draggable: true,
    id: `${rungIndex}-${elementIndex}`
  })

  // 背景
  const bg = new Konva.Rect({
    width: ELEMENT_WIDTH,
    height: ELEMENT_HEIGHT,
    fill: 'white',
    stroke: element.type.includes('COIL') ? COLORS.coil : COLORS.contact,
    strokeWidth: 2,
    cornerRadius: 4
  })
  group.add(bg)

  // 符号
  if (element.type === 'NO') {
    // 常开触点 --| |--
    group.add(new Konva.Line({
      points: [0, ELEMENT_HEIGHT / 2, 15, ELEMENT_HEIGHT / 2],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [15, 10, 15, ELEMENT_HEIGHT - 10],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [45, 10, 45, ELEMENT_HEIGHT - 10],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [45, ELEMENT_HEIGHT / 2, ELEMENT_WIDTH, ELEMENT_HEIGHT / 2],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
  } else if (element.type === 'NC') {
    // 常闭触点 --|/|--
    group.add(new Konva.Line({
      points: [0, ELEMENT_HEIGHT / 2, 15, ELEMENT_HEIGHT / 2],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [15, 10, 15, ELEMENT_HEIGHT - 10],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [15, ELEMENT_HEIGHT - 10, 45, 10],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [45, 10, 45, ELEMENT_HEIGHT - 10],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [45, ELEMENT_HEIGHT / 2, ELEMENT_WIDTH, ELEMENT_HEIGHT / 2],
      stroke: COLORS.contact,
      strokeWidth: 2
    }))
  } else if (element.type === 'COIL' || element.type === 'SET' || element.type === 'RESET') {
    // 线圈 --( )--
    group.add(new Konva.Line({
      points: [0, ELEMENT_HEIGHT / 2, 10, ELEMENT_HEIGHT / 2],
      stroke: COLORS.coil,
      strokeWidth: 2
    }))
    group.add(new Konva.Circle({
      x: ELEMENT_WIDTH / 2,
      y: ELEMENT_HEIGHT / 2,
      radius: 12,
      stroke: COLORS.coil,
      strokeWidth: 2
    }))
    group.add(new Konva.Line({
      points: [ELEMENT_WIDTH - 10, ELEMENT_HEIGHT / 2, ELEMENT_WIDTH, ELEMENT_HEIGHT / 2],
      stroke: COLORS.coil,
      strokeWidth: 2
    }))

    // S/R 标记
    if (element.type === 'SET') {
      group.add(new Konva.Text({
        x: ELEMENT_WIDTH / 2 - 4,
        y: ELEMENT_HEIGHT / 2 - 6,
        text: 'S',
        fontSize: 12,
        fill: COLORS.coil
      }))
    } else if (element.type === 'RESET') {
      group.add(new Konva.Text({
        x: ELEMENT_WIDTH / 2 - 4,
        y: ELEMENT_HEIGHT / 2 - 6,
        text: 'R',
        fontSize: 12,
        fill: COLORS.coil
      }))
    }
  }

  // 变量名
  group.add(new Konva.Text({
    x: 0,
    y: -15,
    width: ELEMENT_WIDTH,
    text: element.variable || '???',
    fontSize: 11,
    fill: COLORS.text,
    align: 'center'
  }))

  // 选中事件
  group.on('click', (e) => {
    e.cancelBubble = true
    selectedElement.value = {
      rungIndex,
      elementIndex,
      attrs: element
    }
    transformer.nodes([group])
    layer.batchDraw()
  })

  // 拖动事件
  group.on('dragend', () => {
    // 更新位置（简化处理）
    emit('change')
  })

  layer.add(group)
}

// 操作方法
function addRung() {
  const data = { ...props.modelValue }
  data.rungs = data.rungs || []
  data.rungs.push({ elements: [] })
  emit('update:modelValue', data)
  renderLadder()
}

function addContact(type) {
  const rungIndex = selectedElement.value?.rungIndex ?? (props.modelValue.rungs?.length - 1) ?? 0
  const data = { ...props.modelValue }

  if (!data.rungs || data.rungs.length === 0) {
    data.rungs = [{ elements: [] }]
  }

  data.rungs[rungIndex].elements.push({
    type,
    variable: `I${data.rungs[rungIndex].elements.length}`
  })

  emit('update:modelValue', data)
  renderLadder()
}

function addCoil(type) {
  const rungIndex = selectedElement.value?.rungIndex ?? (props.modelValue.rungs?.length - 1) ?? 0
  const data = { ...props.modelValue }

  if (!data.rungs || data.rungs.length === 0) {
    data.rungs = [{ elements: [] }]
  }

  data.rungs[rungIndex].elements.push({
    type,
    variable: `Q${data.rungs[rungIndex].elements.length}`
  })

  emit('update:modelValue', data)
  renderLadder()
}

function deleteSelected() {
  if (!selectedElement.value) return

  const { rungIndex, elementIndex } = selectedElement.value
  const data = { ...props.modelValue }

  if (elementIndex !== undefined) {
    data.rungs[rungIndex].elements.splice(elementIndex, 1)
  } else {
    data.rungs.splice(rungIndex, 1)
  }

  selectedElement.value = null
  transformer.nodes([])
  emit('update:modelValue', data)
  renderLadder()
}

function updateProperty(key, value) {
  if (!selectedElement.value) return

  const { rungIndex, elementIndex } = selectedElement.value
  const data = { ...props.modelValue }
  data.rungs[rungIndex].elements[elementIndex][key] = value

  emit('update:modelValue', data)
  renderLadder()
}

function zoomIn() {
  const newScale = scale.value * 1.2
  stage.scale({ x: newScale, y: newScale })
  scale.value = newScale
  layer.batchDraw()
}

function zoomOut() {
  const newScale = scale.value / 1.2
  stage.scale({ x: newScale, y: newScale })
  scale.value = newScale
  layer.batchDraw()
}

function resetZoom() {
  stage.scale({ x: 1, y: 1 })
  stage.position({ x: 0, y: 0 })
  scale.value = 1
  layer.batchDraw()
}

// 监听数据变化
watch(() => props.modelValue, () => {
  if (stage) {
    renderLadder()
  }
}, { deep: true })
</script>

<style scoped>
.ld-editor {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #fafafa;
}

.editor-toolbar {
  display: flex;
  align-items: center;
  padding: 8px;
  background: white;
  border-bottom: 1px solid #e0e0e0;
  gap: 4px;
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
  background: white;
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

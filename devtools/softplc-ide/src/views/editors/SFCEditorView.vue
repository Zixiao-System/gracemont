<template>
  <div class="sfc-editor-view">
    <!-- 工具栏 -->
    <div class="sfc-toolbar">
      <mdui-segmented-button-group>
        <mdui-segmented-button value="select" :selected="currentTool === 'select'" @click="currentTool = 'select'">
          <mdui-icon-near-me slot="icon"></mdui-icon-near-me>
          选择
        </mdui-segmented-button>
        <mdui-segmented-button value="step" :selected="currentTool === 'step'" @click="currentTool = 'step'">
          <mdui-icon-square slot="icon"></mdui-icon-square>
          步骤
        </mdui-segmented-button>
        <mdui-segmented-button value="transition" :selected="currentTool === 'transition'" @click="currentTool = 'transition'">
          <mdui-icon-horizontal-rule slot="icon"></mdui-icon-horizontal-rule>
          转换
        </mdui-segmented-button>
        <mdui-segmented-button value="connect" :selected="currentTool === 'connect'" @click="currentTool = 'connect'">
          <mdui-icon-timeline slot="icon"></mdui-icon-timeline>
          连接
        </mdui-segmented-button>
      </mdui-segmented-button-group>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <mdui-tooltip content="初始步骤">
        <mdui-button variant="outlined" @click="addInitialStep">
          <mdui-icon-play-arrow slot="icon"></mdui-icon-play-arrow>
          初始步骤
        </mdui-button>
      </mdui-tooltip>

      <mdui-tooltip content="删除选中">
        <mdui-button-icon @click="deleteSelected" :disabled="!selectedStep && !selectedTransition">
          <mdui-icon-delete></mdui-icon-delete>
        </mdui-button-icon>
      </mdui-tooltip>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

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

    <div class="sfc-main">
      <!-- SFC 画布 -->
      <div
        class="sfc-canvas"
        ref="canvasRef"
        @click="handleCanvasClick"
        @mousemove="handleCanvasMouseMove"
        @mouseup="handleCanvasMouseUp"
        :style="{ transform: `scale(${zoom})` }"
      >
        <!-- SVG 连接线层 -->
        <svg class="connections-layer">
          <!-- 步骤到转换的连接 -->
          <line
            v-for="conn in stepToTransitionConnections"
            :key="conn.id"
            :x1="conn.x1"
            :y1="conn.y1"
            :x2="conn.x2"
            :y2="conn.y2"
            class="sfc-connection"
          />
        </svg>

        <!-- 步骤 -->
        <div
          v-for="step in diagram.steps"
          :key="step.id"
          class="sfc-step"
          :class="{
            initial: step.type === 'INITIAL',
            selected: selectedStep?.id === step.id,
            active: step.type === 'INITIAL'
          }"
          :style="{ left: step.x + 'px', top: step.y + 'px' }"
          @mousedown.stop="handleStepMouseDown($event, step)"
          @dblclick="editStep(step)"
        >
          {{ step.name }}
          <!-- 动作指示器 -->
          <div class="step-actions" v-if="step.actions.length > 0">
            <mdui-badge>{{ step.actions.length }}</mdui-badge>
          </div>
        </div>

        <!-- 转换 -->
        <div
          v-for="transition in diagram.transitions"
          :key="transition.id"
          class="sfc-transition"
          :class="{ selected: selectedTransition?.id === transition.id }"
          :style="{ left: transition.x + 'px', top: transition.y + 'px' }"
          @mousedown.stop="handleTransitionMouseDown($event, transition)"
          @dblclick="editTransition(transition)"
        >
          <span class="transition-condition">{{ transition.condition || '条件' }}</span>
        </div>
      </div>

      <!-- 属性面板 -->
      <div class="property-panel">
        <!-- 步骤属性 -->
        <template v-if="selectedStep">
          <h4>步骤属性</h4>
          <mdui-text-field
            :value="selectedStep.name"
            @input="updateStepName($event)"
            label="步骤名称"
          ></mdui-text-field>

          <mdui-select :value="selectedStep.type" @change="updateStepType($event)" label="类型" style="margin-top: 12px;">
            <mdui-menu-item value="NORMAL">普通步骤</mdui-menu-item>
            <mdui-menu-item value="INITIAL">初始步骤</mdui-menu-item>
          </mdui-select>

          <h5 style="margin-top: 16px;">动作列表</h5>
          <div class="actions-list">
            <div v-for="action in selectedStep.actions" :key="action.id" class="action-item">
              <span>{{ action.qualifier }}: {{ action.name }}</span>
              <mdui-button-icon @click="removeAction(action.id)">
                <mdui-icon-close></mdui-icon-close>
              </mdui-button-icon>
            </div>
            <mdui-button variant="text" @click="addAction">
              <mdui-icon-add slot="icon"></mdui-icon-add>
              添加动作
            </mdui-button>
          </div>
        </template>

        <!-- 转换属性 -->
        <template v-else-if="selectedTransition">
          <h4>转换属性</h4>
          <mdui-text-field
            :value="selectedTransition.name"
            @input="updateTransitionName($event)"
            label="转换名称"
          ></mdui-text-field>

          <mdui-text-field
            :value="selectedTransition.condition"
            @input="updateTransitionCondition($event)"
            label="转换条件"
            helper="输入 ST 布尔表达式"
            style="margin-top: 12px;"
          ></mdui-text-field>
        </template>

        <!-- 无选中 -->
        <template v-else>
          <div class="no-selection">
            <mdui-icon-touch-app style="font-size: 48px; opacity: 0.3;"></mdui-icon-touch-app>
            <p>选择步骤或转换以编辑属性</p>
          </div>
        </template>
      </div>
    </div>

    <!-- 添加动作对话框 -->
    <mdui-dialog :open="showActionDialog" @closed="showActionDialog = false" headline="添加动作">
      <div class="dialog-content">
        <mdui-text-field v-model="newAction.name" label="动作名称" required></mdui-text-field>

        <mdui-select v-model="newAction.qualifier" label="限定符" style="margin-top: 12px;">
          <mdui-menu-item value="N">N - 非存储</mdui-menu-item>
          <mdui-menu-item value="S">S - 置位</mdui-menu-item>
          <mdui-menu-item value="R">R - 复位</mdui-menu-item>
          <mdui-menu-item value="P">P - 脉冲</mdui-menu-item>
          <mdui-menu-item value="L">L - 时间限制</mdui-menu-item>
          <mdui-menu-item value="D">D - 延时</mdui-menu-item>
        </mdui-select>

        <mdui-text-field
          v-model="newAction.body"
          label="动作代码 (ST)"
          rows="4"
          style="margin-top: 12px;"
        ></mdui-text-field>
      </div>

      <mdui-button slot="action" variant="text" @click="showActionDialog = false">取消</mdui-button>
      <mdui-button slot="action" @click="saveAction">添加</mdui-button>
    </mdui-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, reactive, onMounted } from 'vue'
import { useRoute } from 'vue-router'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { SFCDiagram, SFCStep, SFCTransition, SFCAction } from '@/types/iec61131'
import { SFCStepType, SFCActionQualifier } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

import '@mdui/icons/near-me.js'
import '@mdui/icons/square.js'
import '@mdui/icons/horizontal-rule.js'
import '@mdui/icons/timeline.js'
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/zoom-in.js'
import '@mdui/icons/zoom-out.js'
import '@mdui/icons/close.js'
import '@mdui/icons/add.js'
import '@mdui/icons/touch-app.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const pouId = computed(() => route.params.id as string)
const pou = computed(() => projectStore.getPOU(pouId.value))

const canvasRef = ref<HTMLElement | null>(null)
const currentTool = ref('select')
const zoom = ref(1)
const selectedStep = ref<SFCStep | null>(null)
const selectedTransition = ref<SFCTransition | null>(null)

const isDragging = ref(false)
const dragOffset = reactive({ x: 0, y: 0 })

const showActionDialog = ref(false)
const newAction = reactive({
  name: '',
  qualifier: 'N',
  body: ''
})

const diagram = ref<SFCDiagram>({
  steps: [],
  transitions: []
})

onMounted(() => {
  if (pou.value && typeof pou.value.body === 'object') {
    diagram.value = pou.value.body as SFCDiagram
  }
})

// 计算连接线
const stepToTransitionConnections = computed(() => {
  const connections: Array<{ id: string, x1: number, y1: number, x2: number, y2: number }> = []

  diagram.value.transitions.forEach(trans => {
    trans.sourceSteps.forEach(stepId => {
      const step = diagram.value.steps.find(s => s.id === stepId)
      if (step) {
        connections.push({
          id: `${step.id}-${trans.id}`,
          x1: step.x + 60,
          y1: step.y + 60,
          x2: trans.x + 40,
          y2: trans.y
        })
      }
    })

    trans.targetSteps.forEach(stepId => {
      const step = diagram.value.steps.find(s => s.id === stepId)
      if (step) {
        connections.push({
          id: `${trans.id}-${step.id}`,
          x1: trans.x + 40,
          y1: trans.y + 8,
          x2: step.x + 60,
          y2: step.y
        })
      }
    })
  })

  return connections
})

function handleCanvasClick(e: MouseEvent) {
  if (currentTool.value === 'step') {
    addStep(e)
  } else if (currentTool.value === 'transition') {
    addTransition(e)
  } else {
    selectedStep.value = null
    selectedTransition.value = null
  }
}

function handleCanvasMouseMove(e: MouseEvent) {
  if (isDragging.value) {
    const rect = canvasRef.value!.getBoundingClientRect()
    const x = (e.clientX - rect.left) / zoom.value - dragOffset.x
    const y = (e.clientY - rect.top) / zoom.value - dragOffset.y

    if (selectedStep.value) {
      selectedStep.value.x = x
      selectedStep.value.y = y
    } else if (selectedTransition.value) {
      selectedTransition.value.x = x
      selectedTransition.value.y = y
    }
  }
}

function handleCanvasMouseUp() {
  if (isDragging.value) {
    isDragging.value = false
    saveDiagram()
  }
}

function handleStepMouseDown(e: MouseEvent, step: SFCStep) {
  selectedStep.value = step
  selectedTransition.value = null

  if (currentTool.value === 'select') {
    isDragging.value = true
    const rect = canvasRef.value!.getBoundingClientRect()
    dragOffset.x = (e.clientX - rect.left) / zoom.value - step.x
    dragOffset.y = (e.clientY - rect.top) / zoom.value - step.y
  } else if (currentTool.value === 'connect') {
    // 连接模式
  }
}

function handleTransitionMouseDown(e: MouseEvent, transition: SFCTransition) {
  selectedTransition.value = transition
  selectedStep.value = null

  if (currentTool.value === 'select') {
    isDragging.value = true
    const rect = canvasRef.value!.getBoundingClientRect()
    dragOffset.x = (e.clientX - rect.left) / zoom.value - transition.x
    dragOffset.y = (e.clientY - rect.top) / zoom.value - transition.y
  }
}

function addStep(e: MouseEvent) {
  const rect = canvasRef.value!.getBoundingClientRect()
  const step: SFCStep = {
    id: uuidv4(),
    name: `Step${diagram.value.steps.length + 1}`,
    type: SFCStepType.NORMAL,
    x: (e.clientX - rect.left) / zoom.value - 60,
    y: (e.clientY - rect.top) / zoom.value - 30,
    actions: []
  }

  diagram.value.steps.push(step)
  selectedStep.value = step
  selectedTransition.value = null
  saveDiagram()
}

function addInitialStep() {
  const step: SFCStep = {
    id: uuidv4(),
    name: 'Init',
    type: SFCStepType.INITIAL,
    x: 100,
    y: 50,
    actions: []
  }

  diagram.value.steps.push(step)
  selectedStep.value = step
  saveDiagram()
}

function addTransition(e: MouseEvent) {
  const rect = canvasRef.value!.getBoundingClientRect()
  const transition: SFCTransition = {
    id: uuidv4(),
    name: `T${diagram.value.transitions.length + 1}`,
    condition: 'TRUE',
    sourceSteps: [],
    targetSteps: [],
    x: (e.clientX - rect.left) / zoom.value - 40,
    y: (e.clientY - rect.top) / zoom.value - 4
  }

  diagram.value.transitions.push(transition)
  selectedTransition.value = transition
  selectedStep.value = null
  saveDiagram()
}

function deleteSelected() {
  if (selectedStep.value) {
    // 删除相关转换的引用
    diagram.value.transitions.forEach(t => {
      t.sourceSteps = t.sourceSteps.filter(id => id !== selectedStep.value!.id)
      t.targetSteps = t.targetSteps.filter(id => id !== selectedStep.value!.id)
    })
    // 删除步骤
    diagram.value.steps = diagram.value.steps.filter(s => s.id !== selectedStep.value!.id)
    selectedStep.value = null
    saveDiagram()
  }

  if (selectedTransition.value) {
    diagram.value.transitions = diagram.value.transitions.filter(t => t.id !== selectedTransition.value!.id)
    selectedTransition.value = null
    saveDiagram()
  }
}

function editStep(step: SFCStep) {
  selectedStep.value = step
}

function editTransition(transition: SFCTransition) {
  selectedTransition.value = transition
}

function updateStepName(e: Event) {
  if (selectedStep.value) {
    selectedStep.value.name = (e.target as HTMLInputElement).value
    saveDiagram()
  }
}

function updateStepType(e: Event) {
  if (selectedStep.value) {
    selectedStep.value.type = (e.target as HTMLSelectElement).value as SFCStepType
    saveDiagram()
  }
}

function updateTransitionName(e: Event) {
  if (selectedTransition.value) {
    selectedTransition.value.name = (e.target as HTMLInputElement).value
    saveDiagram()
  }
}

function updateTransitionCondition(e: Event) {
  if (selectedTransition.value) {
    selectedTransition.value.condition = (e.target as HTMLInputElement).value
    saveDiagram()
  }
}

function addAction() {
  newAction.name = ''
  newAction.qualifier = 'N'
  newAction.body = ''
  showActionDialog.value = true
}

function saveAction() {
  if (selectedStep.value && newAction.name) {
    const action: SFCAction = {
      id: uuidv4(),
      name: newAction.name,
      qualifier: newAction.qualifier as SFCActionQualifier,
      body: newAction.body
    }
    selectedStep.value.actions.push(action)
    saveDiagram()
  }
  showActionDialog.value = false
}

function removeAction(actionId: string) {
  if (selectedStep.value) {
    selectedStep.value.actions = selectedStep.value.actions.filter(a => a.id !== actionId)
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
.sfc-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.sfc-toolbar {
  display: flex;
  align-items: center;
  padding: 8px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  background-color: rgb(var(--mdui-color-surface-container));
  gap: 8px;
}

.sfc-main {
  flex: 1;
  display: flex;
  overflow: hidden;
}

.sfc-canvas {
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

.sfc-connection {
  stroke: rgb(var(--mdui-color-outline));
  stroke-width: 2;
}

.sfc-step {
  position: absolute;
  width: 120px;
  height: 60px;
  background-color: rgb(var(--mdui-color-surface-container));
  border: 2px solid rgb(var(--mdui-color-outline));
  display: flex;
  align-items: center;
  justify-content: center;
  font-weight: 500;
  cursor: move;
  user-select: none;
}

.sfc-step.initial {
  border-width: 4px;
}

.sfc-step.selected {
  border-color: rgb(var(--mdui-color-primary));
  box-shadow: 0 0 0 2px rgba(var(--mdui-color-primary), 0.3);
}

.sfc-step.active {
  background-color: rgb(var(--mdui-color-primary-container));
}

.step-actions {
  position: absolute;
  right: -8px;
  top: -8px;
}

.sfc-transition {
  position: absolute;
  width: 80px;
  height: 8px;
  background-color: rgb(var(--mdui-color-outline));
  cursor: move;
  user-select: none;
}

.sfc-transition.selected {
  background-color: rgb(var(--mdui-color-primary));
}

.transition-condition {
  position: absolute;
  left: 100%;
  top: 50%;
  transform: translateY(-50%);
  margin-left: 8px;
  font-size: 12px;
  white-space: nowrap;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.property-panel {
  width: 280px;
  border-left: 1px solid rgb(var(--mdui-color-outline-variant));
  padding: 16px;
  overflow: auto;
}

.property-panel h4 {
  margin: 0 0 16px 0;
  font-size: 14px;
  font-weight: 500;
}

.property-panel h5 {
  margin: 0 0 8px 0;
  font-size: 13px;
  font-weight: 500;
}

.actions-list {
  display: flex;
  flex-direction: column;
  gap: 8px;
}

.action-item {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-radius: 4px;
  font-size: 12px;
}

.no-selection {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 200px;
  text-align: center;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.dialog-content {
  min-width: 350px;
  padding: 16px 0;
}
</style>

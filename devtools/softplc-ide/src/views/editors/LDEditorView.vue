<template>
  <div class="ld-editor-view">
    <!-- 工具栏 -->
    <div class="ld-toolbar">
      <mdui-segmented-button-group>
        <mdui-segmented-button value="select" :selected="currentTool === 'select'" @click="currentTool = 'select'">
          <mdui-icon-near-me slot="icon"></mdui-icon-near-me>
          选择
        </mdui-segmented-button>
        <mdui-segmented-button value="contact-no" :selected="currentTool === 'contact-no'" @click="currentTool = 'contact-no'">
          ─┤ ├─
        </mdui-segmented-button>
        <mdui-segmented-button value="contact-nc" :selected="currentTool === 'contact-nc'" @click="currentTool = 'contact-nc'">
          ─┤/├─
        </mdui-segmented-button>
        <mdui-segmented-button value="coil" :selected="currentTool === 'coil'" @click="currentTool = 'coil'">
          ─( )─
        </mdui-segmented-button>
        <mdui-segmented-button value="coil-set" :selected="currentTool === 'coil-set'" @click="currentTool = 'coil-set'">
          ─(S)─
        </mdui-segmented-button>
        <mdui-segmented-button value="coil-reset" :selected="currentTool === 'coil-reset'" @click="currentTool = 'coil-reset'">
          ─(R)─
        </mdui-segmented-button>
      </mdui-segmented-button-group>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <mdui-tooltip content="添加行">
        <mdui-button-icon @click="addRung">
          <mdui-icon-add></mdui-icon-add>
        </mdui-button-icon>
      </mdui-tooltip>
      <mdui-tooltip content="删除行">
        <mdui-button-icon @click="deleteSelectedRung" :disabled="!selectedRung">
          <mdui-icon-delete></mdui-icon-delete>
        </mdui-button-icon>
      </mdui-tooltip>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <mdui-tooltip content="功能块">
        <mdui-button-icon @click="showFunctionBlockDialog = true">
          <mdui-icon-widgets></mdui-icon-widgets>
        </mdui-button-icon>
      </mdui-tooltip>
    </div>

    <!-- 梯形图编辑区 -->
    <div class="ld-canvas" ref="canvasRef">
      <div
        v-for="(rung, rungIndex) in diagram.rungs"
        :key="rung.id"
        class="ladder-rung"
        :class="{ selected: selectedRung?.id === rung.id }"
        @click="selectRung(rung)"
      >
        <!-- 行号 -->
        <div class="rung-number">{{ rungIndex + 1 }}</div>

        <!-- 左母线 -->
        <div class="ladder-power-rail left"></div>

        <!-- 元素区域 -->
        <div class="rung-elements">
          <template v-for="(element, elemIndex) in rung.elements" :key="element.id">
            <!-- 连接线 -->
            <div class="ladder-wire" v-if="elemIndex > 0"></div>

            <!-- 元素 -->
            <div
              class="ladder-element"
              :class="[getElementClass(element), { selected: selectedElement?.id === element.id }]"
              @click.stop="selectElement(element)"
              @dblclick="editElement(element)"
            >
              <span class="element-symbol">{{ getElementSymbol(element) }}</span>
              <span class="element-label">{{ element.variable || '???' }}</span>
            </div>
          </template>

          <!-- 占位符 - 用于添加新元素 -->
          <div class="ladder-wire"></div>
          <div
            class="element-placeholder"
            @click.stop="addElementToRung(rung)"
            v-if="currentTool !== 'select'"
          >
            <mdui-icon-add></mdui-icon-add>
          </div>
        </div>

        <!-- 右母线 -->
        <div class="ladder-power-rail right"></div>

        <!-- 注释 -->
        <div class="rung-comment" v-if="rung.comment">
          {{ rung.comment }}
        </div>
      </div>

      <!-- 添加新行按钮 -->
      <div class="add-rung-button" @click="addRung">
        <mdui-icon-add></mdui-icon-add>
        添加新行
      </div>
    </div>

    <!-- 元素编辑对话框 -->
    <mdui-dialog :open="showElementDialog" @closed="showElementDialog = false" headline="编辑元素">
      <div class="dialog-content">
        <mdui-text-field
          v-model="editingElement.variable"
          label="变量名"
          required
        ></mdui-text-field>

        <mdui-select v-model="editingElement.type" label="类型" style="margin-top: 16px;">
          <mdui-menu-item value="CONTACT_NO">常开触点 ─┤ ├─</mdui-menu-item>
          <mdui-menu-item value="CONTACT_NC">常闭触点 ─┤/├─</mdui-menu-item>
          <mdui-menu-item value="CONTACT_P">上升沿触点 ─┤P├─</mdui-menu-item>
          <mdui-menu-item value="CONTACT_N">下降沿触点 ─┤N├─</mdui-menu-item>
          <mdui-menu-item value="COIL">线圈 ─( )─</mdui-menu-item>
          <mdui-menu-item value="COIL_SET">置位线圈 ─(S)─</mdui-menu-item>
          <mdui-menu-item value="COIL_RESET">复位线圈 ─(R)─</mdui-menu-item>
        </mdui-select>
      </div>

      <mdui-button slot="action" variant="text" @click="showElementDialog = false">取消</mdui-button>
      <mdui-button slot="action" @click="saveElement">确定</mdui-button>
    </mdui-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, reactive } from 'vue'
import { useRoute } from 'vue-router'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { LDDiagram, LDRung, LDElement } from '@/types/iec61131'
import { LDElementType } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

import '@mdui/icons/near-me.js'
import '@mdui/icons/add.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/widgets.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const pouId = computed(() => route.params.id as string)
const pou = computed(() => projectStore.getPOU(pouId.value))

const currentTool = ref<string>('select')
const selectedRung = ref<LDRung | null>(null)
const selectedElement = ref<LDElement | null>(null)
const showElementDialog = ref(false)
const showFunctionBlockDialog = ref(false)

const editingElement = reactive({
  variable: '',
  type: 'CONTACT_NO'
})

// 初始化或获取梯形图数据
const diagram = ref<LDDiagram>({
  rungs: []
})

onMounted(() => {
  if (pou.value && typeof pou.value.body === 'object') {
    diagram.value = pou.value.body as LDDiagram
  }
  if (diagram.value.rungs.length === 0) {
    addRung()
  }
})

function getElementClass(element: LDElement): string {
  if (element.type.includes('CONTACT')) return 'contact'
  if (element.type.includes('COIL')) return 'coil'
  return ''
}

function getElementSymbol(element: LDElement): string {
  const symbols: Record<string, string> = {
    [LDElementType.CONTACT_NO]: '┤ ├',
    [LDElementType.CONTACT_NC]: '┤/├',
    [LDElementType.CONTACT_P]: '┤P├',
    [LDElementType.CONTACT_N]: '┤N├',
    [LDElementType.COIL]: '( )',
    [LDElementType.COIL_SET]: '(S)',
    [LDElementType.COIL_RESET]: '(R)',
    [LDElementType.COIL_P]: '(P)',
    [LDElementType.COIL_N]: '(N)'
  }
  return symbols[element.type] || '???'
}

function selectRung(rung: LDRung) {
  selectedRung.value = rung
  selectedElement.value = null
}

function selectElement(element: LDElement) {
  selectedElement.value = element
}

function addRung() {
  const newRung: LDRung = {
    id: uuidv4(),
    elements: [],
    comment: ''
  }
  diagram.value.rungs.push(newRung)
  selectedRung.value = newRung
  saveDiagram()
}

function deleteSelectedRung() {
  if (!selectedRung.value) return
  const index = diagram.value.rungs.findIndex(r => r.id === selectedRung.value!.id)
  if (index >= 0) {
    diagram.value.rungs.splice(index, 1)
    selectedRung.value = null
    saveDiagram()
  }
}

function addElementToRung(rung: LDRung) {
  if (currentTool.value === 'select') return

  const typeMap: Record<string, LDElementType> = {
    'contact-no': LDElementType.CONTACT_NO,
    'contact-nc': LDElementType.CONTACT_NC,
    'coil': LDElementType.COIL,
    'coil-set': LDElementType.COIL_SET,
    'coil-reset': LDElementType.COIL_RESET
  }

  const newElement: LDElement = {
    id: uuidv4(),
    type: typeMap[currentTool.value] || LDElementType.CONTACT_NO,
    variable: '',
    x: rung.elements.length,
    y: 0
  }

  rung.elements.push(newElement)
  selectedElement.value = newElement
  editingElement.variable = ''
  editingElement.type = newElement.type
  showElementDialog.value = true
  saveDiagram()
}

function editElement(element: LDElement) {
  selectedElement.value = element
  editingElement.variable = element.variable || ''
  editingElement.type = element.type
  showElementDialog.value = true
}

function saveElement() {
  if (selectedElement.value) {
    selectedElement.value.variable = editingElement.variable
    selectedElement.value.type = editingElement.type as LDElementType
    saveDiagram()
  }
  showElementDialog.value = false
}

function saveDiagram() {
  if (pou.value) {
    projectStore.updatePOU(pouId.value, { body: diagram.value })
    editorStore.markModified(`${pouId.value}-pou`, true)
  }
}
</script>

<style scoped>
.ld-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.ld-toolbar {
  display: flex;
  align-items: center;
  padding: 8px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  background-color: rgb(var(--mdui-color-surface-container));
}

.ld-canvas {
  flex: 1;
  overflow: auto;
  padding: 16px;
}

.ladder-rung {
  display: flex;
  align-items: center;
  min-height: 60px;
  padding: 8px 0;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  cursor: pointer;
}

.ladder-rung:hover {
  background-color: rgba(var(--mdui-color-primary), 0.04);
}

.ladder-rung.selected {
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.rung-number {
  width: 40px;
  text-align: center;
  font-size: 12px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.ladder-power-rail {
  width: 4px;
  height: 60px;
  background-color: rgb(var(--mdui-color-primary));
}

.rung-elements {
  display: flex;
  align-items: center;
  flex: 1;
  padding: 0 8px;
  min-height: 48px;
}

.ladder-wire {
  flex: 1;
  height: 2px;
  background-color: rgb(var(--mdui-color-outline));
  min-width: 20px;
  max-width: 60px;
}

.ladder-element {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  min-width: 80px;
  height: 48px;
  border: 2px solid rgb(var(--mdui-color-outline));
  background-color: rgb(var(--mdui-color-surface));
  cursor: pointer;
  padding: 4px 8px;
}

.ladder-element:hover {
  border-color: rgb(var(--mdui-color-primary));
}

.ladder-element.selected {
  border-color: rgb(var(--mdui-color-primary));
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.ladder-element.contact {
  border-color: rgb(var(--mdui-color-primary));
}

.ladder-element.coil {
  border-radius: 50%;
  border-color: rgb(var(--mdui-color-secondary));
  width: 48px;
  min-width: 48px;
}

.element-symbol {
  font-family: monospace;
  font-size: 14px;
  font-weight: bold;
}

.element-label {
  font-size: 11px;
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-top: 2px;
}

.element-placeholder {
  display: flex;
  align-items: center;
  justify-content: center;
  width: 48px;
  height: 48px;
  border: 2px dashed rgb(var(--mdui-color-outline-variant));
  border-radius: 4px;
  cursor: pointer;
  opacity: 0.5;
}

.element-placeholder:hover {
  opacity: 1;
  border-color: rgb(var(--mdui-color-primary));
}

.rung-comment {
  font-size: 12px;
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-left: 16px;
  font-style: italic;
}

.add-rung-button {
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 8px;
  padding: 16px;
  margin-top: 8px;
  border: 2px dashed rgb(var(--mdui-color-outline-variant));
  border-radius: 8px;
  cursor: pointer;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.add-rung-button:hover {
  border-color: rgb(var(--mdui-color-primary));
  color: rgb(var(--mdui-color-primary));
}

.dialog-content {
  min-width: 300px;
  padding: 16px 0;
}
</style>

<template>
  <mdui-dialog ref="dialogRef" headline="新建项目" :close-on-overlay-click="false">
    <div class="dialog-content">
      <mdui-text-field
        v-model="projectName"
        label="项目名称"
        required
        helper="请输入项目名称"
      ></mdui-text-field>

      <mdui-text-field
        v-model="projectDescription"
        label="项目描述"
        rows="3"
        style="margin-top: 16px;"
      ></mdui-text-field>

      <mdui-select
        v-model="targetPlatform"
        label="目标平台"
        style="margin-top: 16px;"
      >
        <mdui-menu-item value="ARM64-RTOS">ARM64-RTOS</mdui-menu-item>
        <mdui-menu-item value="x86-Linux">x86-Linux</mdui-menu-item>
        <mdui-menu-item value="Simulation">仿真模式</mdui-menu-item>
      </mdui-select>

      <div class="template-section" style="margin-top: 16px;">
        <label class="section-label">项目模板</label>
        <div class="template-grid">
          <div
            class="template-card"
            :class="{ selected: selectedTemplate === 'empty' }"
            @click="selectedTemplate = 'empty'"
          >
            <mdui-icon-folder-open style="font-size: 32px;"></mdui-icon-folder-open>
            <span>空项目</span>
          </div>
          <div
            class="template-card"
            :class="{ selected: selectedTemplate === 'basic' }"
            @click="selectedTemplate = 'basic'"
          >
            <mdui-icon-play-arrow style="font-size: 32px;"></mdui-icon-play-arrow>
            <span>基础程序</span>
          </div>
          <div
            class="template-card"
            :class="{ selected: selectedTemplate === 'modbus' }"
            @click="selectedTemplate = 'modbus'"
          >
            <mdui-icon-settings-ethernet style="font-size: 32px;"></mdui-icon-settings-ethernet>
            <span>Modbus 通信</span>
          </div>
          <div
            class="template-card"
            :class="{ selected: selectedTemplate === 'motion' }"
            @click="selectedTemplate = 'motion'"
          >
            <mdui-icon-precision-manufacturing style="font-size: 32px;"></mdui-icon-precision-manufacturing>
            <span>运动控制</span>
          </div>
        </div>
      </div>
    </div>

    <mdui-button slot="action" variant="text" @click="close">取消</mdui-button>
    <mdui-button slot="action" @click="createProject" :disabled="!isValid">创建</mdui-button>
  </mdui-dialog>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRouter } from 'vue-router'
import { useProjectStore } from '@/stores/project'

import '@mdui/icons/folder-open.js'
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/settings-ethernet.js'
import '@mdui/icons/precision-manufacturing.js'

const router = useRouter()
const projectStore = useProjectStore()

const dialogRef = ref()
const projectName = ref('')
const projectDescription = ref('')
const targetPlatform = ref('ARM64-RTOS')
const selectedTemplate = ref('basic')

const isValid = computed(() => projectName.value.trim().length > 0)

function open() {
  // 重置表单
  projectName.value = ''
  projectDescription.value = ''
  targetPlatform.value = 'ARM64-RTOS'
  selectedTemplate.value = 'basic'

  dialogRef.value?.open?.()
  dialogRef.value?.setAttribute?.('open', '')
}

function close() {
  dialogRef.value?.close?.()
  dialogRef.value?.removeAttribute?.('open')
}

function createProject() {
  if (!isValid.value) return

  const project = projectStore.createProject(
    projectName.value.trim(),
    projectDescription.value.trim()
  )

  close()

  // 导航到主编辑器
  router.push({ name: 'st-editor', params: { id: project.pous[0].id } })
}

defineExpose({ open, close })
</script>

<style scoped>
.dialog-content {
  padding: 16px 0;
  min-width: 400px;
}

.section-label {
  display: block;
  font-size: 14px;
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-bottom: 8px;
}

.template-grid {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 12px;
}

.template-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 16px;
  border: 2px solid rgb(var(--mdui-color-outline-variant));
  border-radius: 8px;
  cursor: pointer;
  transition: all 0.2s;
  gap: 8px;
}

.template-card:hover {
  border-color: rgb(var(--mdui-color-primary));
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.template-card.selected {
  border-color: rgb(var(--mdui-color-primary));
  background-color: rgba(var(--mdui-color-primary), 0.12);
}

.template-card span {
  font-size: 12px;
  text-align: center;
}
</style>

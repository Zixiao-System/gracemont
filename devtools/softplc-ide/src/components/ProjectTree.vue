<template>
  <div class="project-tree">
    <template v-if="projectStore.currentProject">
      <!-- 程序 -->
      <mdui-collapse>
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="apps">
            程序 ({{ projectStore.programs.length }})
          </mdui-list-item>
          <mdui-list>
            <mdui-list-item
              v-for="pou in projectStore.programs"
              :key="pou.id"
              @click="handlePOUClick(pou)"
              @contextmenu.prevent="handleContextMenu($event, pou)"
            >
              <mdui-icon-description slot="icon"></mdui-icon-description>
              {{ pou.name }}
              <mdui-badge slot="end-icon">{{ getLanguageLabel(pou.language) }}</mdui-badge>
            </mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <!-- 功能块 -->
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="widgets">
            功能块 ({{ projectStore.functionBlocks.length }})
          </mdui-list-item>
          <mdui-list>
            <mdui-list-item
              v-for="pou in projectStore.functionBlocks"
              :key="pou.id"
              @click="handlePOUClick(pou)"
              @contextmenu.prevent="handleContextMenu($event, pou)"
            >
              <mdui-icon-view-module slot="icon"></mdui-icon-view-module>
              {{ pou.name }}
              <mdui-badge slot="end-icon">{{ getLanguageLabel(pou.language) }}</mdui-badge>
            </mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <!-- 函数 -->
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="functions">
            函数 ({{ projectStore.functions.length }})
          </mdui-list-item>
          <mdui-list>
            <mdui-list-item
              v-for="pou in projectStore.functions"
              :key="pou.id"
              @click="handlePOUClick(pou)"
              @contextmenu.prevent="handleContextMenu($event, pou)"
            >
              <mdui-icon-function slot="icon"></mdui-icon-function>
              {{ pou.name }}
              <mdui-badge slot="end-icon">{{ getLanguageLabel(pou.language) }}</mdui-badge>
            </mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <!-- 全局变量 -->
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="storage">
            全局变量 ({{ projectStore.globalVariables.length }})
          </mdui-list-item>
          <mdui-list>
            <mdui-list-item @click="openGlobalVariables">
              <mdui-icon-table-view slot="icon"></mdui-icon-table-view>
              变量表
            </mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <!-- 数据类型 -->
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="data-object">
            数据类型 ({{ projectStore.dataTypes.length }})
          </mdui-list-item>
          <mdui-list>
            <mdui-list-item
              v-for="dt in projectStore.dataTypes"
              :key="dt.id"
            >
              <mdui-icon-class slot="icon"></mdui-icon-class>
              {{ dt.name }}
            </mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <!-- 任务配置 -->
        <mdui-collapse-item>
          <mdui-list-item slot="header" icon="schedule">
            任务配置
          </mdui-list-item>
          <mdui-list>
            <template v-if="projectStore.currentProject">
              <mdui-list-item
                v-for="resource in projectStore.currentProject.resources"
                :key="resource.id"
              >
                <mdui-icon-dns slot="icon"></mdui-icon-dns>
                {{ resource.name }}
              </mdui-list-item>
            </template>
          </mdui-list>
        </mdui-collapse-item>
      </mdui-collapse>
    </template>

    <template v-else>
      <div class="no-project">
        <mdui-icon-folder-off style="font-size: 48px; opacity: 0.5;"></mdui-icon-folder-off>
        <p>无打开的项目</p>
        <mdui-button variant="outlined" @click="$emit('new-project')">
          新建项目
        </mdui-button>
      </div>
    </template>

    <!-- 右键菜单 -->
    <mdui-menu
      ref="contextMenu"
      :open="contextMenuOpen"
      @closed="contextMenuOpen = false"
      :style="{ position: 'fixed', left: contextMenuX + 'px', top: contextMenuY + 'px' }"
    >
      <mdui-menu-item @click="handleOpenPOU">
        <mdui-icon-edit slot="icon"></mdui-icon-edit>
        打开
      </mdui-menu-item>
      <mdui-menu-item @click="handleOpenVariables">
        <mdui-icon-table-view slot="icon"></mdui-icon-table-view>
        编辑变量
      </mdui-menu-item>
      <mdui-divider></mdui-divider>
      <mdui-menu-item @click="handleRenamePOU">
        <mdui-icon-drive-file-rename-outline slot="icon"></mdui-icon-drive-file-rename-outline>
        重命名
      </mdui-menu-item>
      <mdui-menu-item @click="handleDeletePOU">
        <mdui-icon-delete slot="icon"></mdui-icon-delete>
        删除
      </mdui-menu-item>
    </mdui-menu>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { POU } from '@/types/iec61131'
import { LanguageType } from '@/types/iec61131'

// 图标导入
import '@mdui/icons/apps.js'
import '@mdui/icons/widgets.js'
import '@mdui/icons/functions.js'
import '@mdui/icons/storage.js'
import '@mdui/icons/data-object.js'
import '@mdui/icons/schedule.js'
import '@mdui/icons/description.js'
import '@mdui/icons/view-module.js'
import '@mdui/icons/table-view.js'
import '@mdui/icons/class.js'
import '@mdui/icons/dns.js'
import '@mdui/icons/folder-off.js'
import '@mdui/icons/edit.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/drive-file-rename-outline.js'

const router = useRouter()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

// 右键菜单状态
const contextMenuOpen = ref(false)
const contextMenuX = ref(0)
const contextMenuY = ref(0)
const selectedPOU = ref<POU | null>(null)

function getLanguageLabel(language: LanguageType): string {
  const labels: Record<LanguageType, string> = {
    [LanguageType.ST]: 'ST',
    [LanguageType.LD]: 'LD',
    [LanguageType.FBD]: 'FBD',
    [LanguageType.IL]: 'IL',
    [LanguageType.SFC]: 'SFC'
  }
  return labels[language]
}

function handlePOUClick(pou: POU) {
  editorStore.openTab(pou, 'pou')
  navigateToPOU(pou)
}

function navigateToPOU(pou: POU) {
  const routeName = `${pou.language.toLowerCase()}-editor`
  router.push({ name: routeName, params: { id: pou.id } })
}

function handleContextMenu(event: MouseEvent, pou: POU) {
  selectedPOU.value = pou
  contextMenuX.value = event.clientX
  contextMenuY.value = event.clientY
  contextMenuOpen.value = true
}

function handleOpenPOU() {
  if (selectedPOU.value) {
    handlePOUClick(selectedPOU.value)
  }
  contextMenuOpen.value = false
}

function handleOpenVariables() {
  if (selectedPOU.value) {
    editorStore.openTab(selectedPOU.value, 'variables')
    router.push({ name: 'variables', params: { id: selectedPOU.value.id } })
  }
  contextMenuOpen.value = false
}

function handleRenamePOU() {
  // TODO: 实现重命名对话框
  contextMenuOpen.value = false
}

function handleDeletePOU() {
  if (selectedPOU.value) {
    projectStore.deletePOU(selectedPOU.value.id)
    editorStore.closeTab(`${selectedPOU.value.id}-pou`)
  }
  contextMenuOpen.value = false
}

function openGlobalVariables() {
  router.push({ name: 'variables', params: { id: 'global' } })
}

defineEmits(['new-project'])
</script>

<style scoped>
.project-tree {
  height: 100%;
  overflow: auto;
}

.no-project {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 200px;
  text-align: center;
  color: rgb(var(--mdui-color-outline));
}

.no-project p {
  margin: 16px 0;
}

mdui-badge {
  font-size: 10px;
}
</style>

<template>
  <div class="editor-tabs">
    <div class="tabs-container" v-if="editorStore.tabs.length > 0">
      <div
        v-for="tab in editorStore.tabs"
        :key="tab.id"
        class="tab"
        :class="{ active: tab.id === editorStore.activeTabId }"
        @click="editorStore.setActiveTab(tab.id)"
        @contextmenu.prevent="handleContextMenu($event, tab)"
      >
        <span class="tab-name">
          {{ tab.name }}
          <span v-if="tab.isModified" class="modified-indicator">*</span>
        </span>
        <mdui-button-icon
          class="close-btn"
          @click.stop="editorStore.closeTab(tab.id)"
        >
          <mdui-icon-close></mdui-icon-close>
        </mdui-button-icon>
      </div>
    </div>

    <!-- 右键菜单 -->
    <mdui-menu
      :open="contextMenuOpen"
      @closed="contextMenuOpen = false"
      :style="{ position: 'fixed', left: contextMenuX + 'px', top: contextMenuY + 'px' }"
    >
      <mdui-menu-item @click="handleClose">
        关闭
      </mdui-menu-item>
      <mdui-menu-item @click="handleCloseOthers">
        关闭其他
      </mdui-menu-item>
      <mdui-menu-item @click="handleCloseAll">
        关闭全部
      </mdui-menu-item>
    </mdui-menu>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useEditorStore, type EditorTab } from '@/stores/editor'

import '@mdui/icons/close.js'

const editorStore = useEditorStore()

const contextMenuOpen = ref(false)
const contextMenuX = ref(0)
const contextMenuY = ref(0)
const selectedTab = ref<EditorTab | null>(null)

function handleContextMenu(event: MouseEvent, tab: EditorTab) {
  selectedTab.value = tab
  contextMenuX.value = event.clientX
  contextMenuY.value = event.clientY
  contextMenuOpen.value = true
}

function handleClose() {
  if (selectedTab.value) {
    editorStore.closeTab(selectedTab.value.id)
  }
  contextMenuOpen.value = false
}

function handleCloseOthers() {
  if (selectedTab.value) {
    editorStore.closeOtherTabs(selectedTab.value.id)
  }
  contextMenuOpen.value = false
}

function handleCloseAll() {
  editorStore.closeAllTabs()
  contextMenuOpen.value = false
}
</script>

<style scoped>
.editor-tabs {
  height: 36px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  display: flex;
  align-items: stretch;
}

.tabs-container {
  display: flex;
  overflow-x: auto;
  flex: 1;
}

.tab {
  display: flex;
  align-items: center;
  padding: 0 8px 0 16px;
  min-width: 120px;
  max-width: 200px;
  border-right: 1px solid rgb(var(--mdui-color-outline-variant));
  cursor: pointer;
  font-size: 13px;
  color: rgb(var(--mdui-color-on-surface-variant));
  transition: background-color 0.2s;
}

.tab:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.tab.active {
  background-color: rgb(var(--mdui-color-surface));
  color: rgb(var(--mdui-color-on-surface));
  border-bottom: 2px solid rgb(var(--mdui-color-primary));
}

.tab-name {
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}

.modified-indicator {
  color: rgb(var(--mdui-color-primary));
  margin-left: 4px;
}

.close-btn {
  --mdui-comp-icon-button-shape-corner: 4px;
  width: 20px;
  height: 20px;
  font-size: 14px;
  opacity: 0;
  transition: opacity 0.2s;
}

.tab:hover .close-btn,
.tab.active .close-btn {
  opacity: 1;
}
</style>

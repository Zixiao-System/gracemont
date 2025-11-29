<template>
  <mdui-layout class="ide-layout">
    <!-- 顶部应用栏 -->
    <mdui-top-app-bar class="ide-header">
      <mdui-button-icon @click="toggleSidebar">
        <mdui-icon-menu></mdui-icon-menu>
      </mdui-button-icon>
      <mdui-top-app-bar-title>SoftPLC IDE</mdui-top-app-bar-title>

      <!-- 工具栏按钮 -->
      <div class="toolbar-actions">
        <mdui-tooltip content="新建项目 (Ctrl+N)">
          <mdui-button-icon @click="handleNewProject">
            <mdui-icon-create-new-folder></mdui-icon-create-new-folder>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="打开项目 (Ctrl+O)">
          <mdui-button-icon @click="handleOpenProject">
            <mdui-icon-folder-open></mdui-icon-folder-open>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="保存 (Ctrl+S)">
          <mdui-button-icon @click="handleSave">
            <mdui-icon-save></mdui-icon-save>
          </mdui-button-icon>
        </mdui-tooltip>

        <mdui-divider vertical class="toolbar-divider"></mdui-divider>

        <mdui-tooltip content="编译 (F7)">
          <mdui-button-icon @click="handleCompile">
            <mdui-icon-build></mdui-icon-build>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="下载 (F8)">
          <mdui-button-icon @click="handleDownload">
            <mdui-icon-download></mdui-icon-download>
          </mdui-button-icon>
        </mdui-tooltip>

        <mdui-divider vertical class="toolbar-divider"></mdui-divider>

        <mdui-tooltip content="运行 (F5)">
          <mdui-button-icon @click="handleRun" :disabled="!isConnected">
            <mdui-icon-play-arrow></mdui-icon-play-arrow>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="停止 (Shift+F5)">
          <mdui-button-icon @click="handleStop" :disabled="!isRunning">
            <mdui-icon-stop></mdui-icon-stop>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-tooltip content="在线监控 (F6)">
          <mdui-button-icon @click="handleMonitor" :class="{ active: isMonitoring }">
            <mdui-icon-monitor></mdui-icon-monitor>
          </mdui-button-icon>
        </mdui-tooltip>
      </div>

      <div class="spacer"></div>

      <!-- 连接状态 -->
      <div class="connection-status">
        <span class="status-indicator" :class="connectionStatus"></span>
        <span class="status-text">{{ connectionStatusText }}</span>
      </div>

      <mdui-tooltip content="切换主题">
        <mdui-button-icon @click="toggleTheme">
          <mdui-icon-dark-mode v-if="isDarkMode"></mdui-icon-dark-mode>
          <mdui-icon-light-mode v-else></mdui-icon-light-mode>
        </mdui-button-icon>
      </mdui-tooltip>
    </mdui-top-app-bar>

    <!-- 侧边栏导航 -->
    <mdui-navigation-drawer :open="sidebarOpen" class="ide-sidebar">
      <mdui-list>
        <mdui-list-subheader>项目浏览器</mdui-list-subheader>
        <ProjectTree />
      </mdui-list>
    </mdui-navigation-drawer>

    <!-- 主内容区 -->
    <mdui-layout-main class="ide-main">
      <div class="main-content">
        <!-- 编辑区域 -->
        <div class="editor-area">
          <EditorTabs />
          <router-view />
        </div>

        <!-- 底部面板 -->
        <div class="bottom-panel" v-if="bottomPanelOpen">
          <mdui-tabs :value="activeBottomTab" @change="handleBottomTabChange">
            <mdui-tab value="output">
              <mdui-icon-terminal slot="icon"></mdui-icon-terminal>
              输出
            </mdui-tab>
            <mdui-tab value="errors">
              <mdui-icon-error slot="icon"></mdui-icon-error>
              错误列表
            </mdui-tab>
            <mdui-tab value="watch">
              <mdui-icon-visibility slot="icon"></mdui-icon-visibility>
              监视
            </mdui-tab>
            <mdui-tab value="crossref">
              <mdui-icon-link slot="icon"></mdui-icon-link>
              交叉引用
            </mdui-tab>
          </mdui-tabs>

          <div class="bottom-panel-content">
            <OutputPanel v-if="activeBottomTab === 'output'" />
            <ErrorListPanel v-if="activeBottomTab === 'errors'" />
            <WatchPanel v-if="activeBottomTab === 'watch'" />
            <CrossRefPanel v-if="activeBottomTab === 'crossref'" />
          </div>
        </div>
      </div>
    </mdui-layout-main>

    <!-- 状态栏 -->
    <div class="ide-statusbar">
      <div class="statusbar-left">
        <span v-if="currentProject">{{ currentProject.name }}</span>
        <span v-else>无项目</span>
        <mdui-divider vertical></mdui-divider>
        <span v-if="currentFile">{{ currentFile }}</span>
      </div>
      <div class="statusbar-right">
        <span>行 {{ cursorLine }}, 列 {{ cursorColumn }}</span>
        <mdui-divider vertical></mdui-divider>
        <span>{{ plcStatus }}</span>
        <mdui-divider vertical></mdui-divider>
        <span>{{ scanTime }}</span>
      </div>
    </div>
  </mdui-layout>

  <!-- 新建项目对话框 -->
  <NewProjectDialog ref="newProjectDialog" />
</template>

<script setup lang="ts">
import { ref, computed, onMounted } from 'vue'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import { usePlcStore } from '@/stores/plc'

// 图标导入
import '@mdui/icons/menu.js'
import '@mdui/icons/create-new-folder.js'
import '@mdui/icons/folder-open.js'
import '@mdui/icons/save.js'
import '@mdui/icons/build.js'
import '@mdui/icons/download.js'
import '@mdui/icons/play-arrow.js'
import '@mdui/icons/stop.js'
import '@mdui/icons/monitor.js'
import '@mdui/icons/dark-mode.js'
import '@mdui/icons/light-mode.js'
import '@mdui/icons/terminal.js'
import '@mdui/icons/error.js'
import '@mdui/icons/visibility.js'
import '@mdui/icons/link.js'

// 组件导入
import ProjectTree from '@/components/ProjectTree.vue'
import EditorTabs from '@/components/EditorTabs.vue'
import OutputPanel from '@/components/panels/OutputPanel.vue'
import ErrorListPanel from '@/components/panels/ErrorListPanel.vue'
import WatchPanel from '@/components/panels/WatchPanel.vue'
import CrossRefPanel from '@/components/panels/CrossRefPanel.vue'
import NewProjectDialog from '@/components/dialogs/NewProjectDialog.vue'

const projectStore = useProjectStore()
const editorStore = useEditorStore()
const plcStore = usePlcStore()

// UI 状态
const sidebarOpen = ref(true)
const bottomPanelOpen = ref(true)
const activeBottomTab = ref('output')
const isDarkMode = ref(false)
const newProjectDialog = ref()

// 计算属性
const currentProject = computed(() => projectStore.currentProject)
const currentFile = computed(() => editorStore.activeFile?.name || '')
const cursorLine = computed(() => editorStore.cursorPosition.line)
const cursorColumn = computed(() => editorStore.cursorPosition.column)
const isConnected = computed(() => plcStore.isConnected)
const isRunning = computed(() => plcStore.isRunning)
const isMonitoring = computed(() => plcStore.isMonitoring)
const connectionStatus = computed(() => {
  if (plcStore.isConnected) {
    return plcStore.isRunning ? 'running' : 'online'
  }
  return 'offline'
})
const connectionStatusText = computed(() => {
  if (!plcStore.isConnected) return '离线'
  return plcStore.isRunning ? '运行中' : '已连接'
})
const plcStatus = computed(() => plcStore.status)
const scanTime = computed(() => `扫描周期: ${plcStore.scanTime}ms`)

// 方法
function toggleSidebar() {
  sidebarOpen.value = !sidebarOpen.value
}

function toggleTheme() {
  isDarkMode.value = !isDarkMode.value
  document.documentElement.classList.toggle('mdui-theme-dark', isDarkMode.value)
}

function handleBottomTabChange(e: CustomEvent) {
  activeBottomTab.value = e.detail.value
}

function handleNewProject() {
  newProjectDialog.value?.open()
}

function handleOpenProject() {
  projectStore.openProject()
}

function handleSave() {
  editorStore.saveCurrentFile()
}

function handleCompile() {
  projectStore.compile()
}

function handleDownload() {
  plcStore.download()
}

function handleRun() {
  plcStore.run()
}

function handleStop() {
  plcStore.stop()
}

function handleMonitor() {
  plcStore.toggleMonitor()
}

// Electron 菜单事件
onMounted(() => {
  if (window.electronAPI) {
    window.electronAPI.onMenuAction((action: string) => {
      switch (action) {
        case 'menu:new-project': handleNewProject(); break
        case 'menu:open-project': handleOpenProject(); break
        case 'menu:save': handleSave(); break
        case 'menu:compile': handleCompile(); break
        case 'menu:download': handleDownload(); break
        case 'menu:run': handleRun(); break
        case 'menu:stop': handleStop(); break
        case 'menu:monitor': handleMonitor(); break
      }
    })
  }
})
</script>

<style scoped>
.ide-layout {
  height: 100vh;
  display: flex;
  flex-direction: column;
}

.ide-header {
  --mdui-comp-top-app-bar-background: rgb(var(--mdui-color-surface-container));
}

.toolbar-actions {
  display: flex;
  align-items: center;
  gap: 4px;
  margin-left: 16px;
}

.toolbar-divider {
  height: 24px;
  margin: 0 8px;
}

.spacer {
  flex: 1;
}

.connection-status {
  display: flex;
  align-items: center;
  margin-right: 16px;
  font-size: 14px;
}

.status-text {
  margin-left: 8px;
}

.ide-sidebar {
  --mdui-comp-navigation-drawer-width: var(--ide-sidebar-width);
}

.ide-main {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.main-content {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.editor-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.bottom-panel {
  height: var(--ide-bottom-panel-height);
  border-top: 1px solid rgb(var(--mdui-color-outline-variant));
  display: flex;
  flex-direction: column;
}

.bottom-panel-content {
  flex: 1;
  overflow: auto;
}

.ide-statusbar {
  height: 24px;
  background-color: rgb(var(--mdui-color-primary));
  color: rgb(var(--mdui-color-on-primary));
  display: flex;
  justify-content: space-between;
  align-items: center;
  padding: 0 16px;
  font-size: 12px;
}

.statusbar-left,
.statusbar-right {
  display: flex;
  align-items: center;
  gap: 8px;
}

.statusbar-left mdui-divider,
.statusbar-right mdui-divider {
  height: 12px;
  opacity: 0.5;
}

mdui-button-icon.active {
  color: rgb(var(--mdui-color-primary));
}
</style>

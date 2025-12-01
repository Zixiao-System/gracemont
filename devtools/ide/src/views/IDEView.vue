<template>
  <div class="ide-view">
    <!-- 顶部菜单栏 -->
    <mdui-top-app-bar scroll-behavior="elevate">
      <mdui-button-icon @click="toggleSidebar">
        <mdui-icon-menu></mdui-icon-menu>
      </mdui-button-icon>
      <mdui-top-app-bar-title>{{ projectName }} - Greyy IDE</mdui-top-app-bar-title>

      <div style="flex-grow: 1"></div>

      <!-- 菜单按钮 -->
      <mdui-button variant="text" @click="showNewProjectDialog = true">
        <mdui-icon-create-new-folder slot="icon"></mdui-icon-create-new-folder>
        新建项目
      </mdui-button>

      <mdui-button variant="text" @click="openProject">
        <mdui-icon-folder-open slot="icon"></mdui-icon-folder-open>
        打开项目
      </mdui-button>

      <mdui-button
        variant="text"
        @click="buildProject"
        :disabled="!hasProject || building"
      >
        <mdui-icon-build slot="icon"></mdui-icon-build>
        构建
      </mdui-button>

      <mdui-button-icon>
        <mdui-icon-settings></mdui-icon-settings>
      </mdui-button-icon>
    </mdui-top-app-bar>

    <!-- 主工作区 -->
    <div class="workspace">
      <!-- 左侧边栏 - 文件树 -->
      <div class="sidebar" :class="{ collapsed: sidebarCollapsed }">
        <div class="sidebar-header">
          <span>项目文件</span>
          <mdui-button-icon
            @click="refreshProjectFiles"
            :disabled="!hasProject"
            style="--mdui-comp-button-icon-size: 36px"
          >
            <mdui-icon-refresh></mdui-icon-refresh>
          </mdui-button-icon>
        </div>

        <div class="file-tree">
          <div v-if="!hasProject" class="empty-state">
            <mdui-icon-folder-off style="font-size: 48px; opacity: 0.3"></mdui-icon-folder-off>
            <p>未打开项目</p>
          </div>

          <FileTreeItem
            v-else
            v-for="item in projectFiles"
            :key="item.path"
            :item="item"
            @select="handleFileSelect"
          />
        </div>
      </div>

      <!-- 中间 - 编辑器区域 -->
      <div class="editor-area">
        <!-- 标签页 -->
        <div class="editor-tabs">
          <div
            v-for="file in openFiles"
            :key="file.path"
            :class="['editor-tab', { active: activeFile?.path === file.path }]"
            @click="setActiveFile(file)"
          >
            <mdui-icon-description style="font-size: 16px"></mdui-icon-description>
            <span class="tab-label">{{ file.name }}</span>
            <span v-if="file.modified" class="modified-indicator">●</span>
            <mdui-button-icon
              @click.stop="closeFile(file.path)"
              style="--mdui-comp-button-icon-size: 24px"
            >
              <mdui-icon-close style="font-size: 16px"></mdui-icon-close>
            </mdui-button-icon>
          </div>

          <div v-if="openFiles.length === 0" class="empty-tabs">
            <p>未打开文件</p>
          </div>
        </div>

        <!-- Monaco 编辑器 -->
        <div ref="editorContainer" class="editor-container"></div>
      </div>
    </div>

    <!-- 底部输出面板 -->
    <div v-if="outputPanelVisible" class="output-panel" :style="{ height: outputPanelHeight + 'px' }">
      <div class="output-header">
        <span>输出</span>
        <div class="output-actions">
          <mdui-button-icon @click="clearBuildOutput">
            <mdui-icon-delete></mdui-icon-delete>
          </mdui-button-icon>
          <mdui-button-icon @click="toggleOutputPanel">
            <mdui-icon-close></mdui-icon-close>
          </mdui-button-icon>
        </div>
      </div>
      <div class="output-content">
        <div
          v-for="(log, index) in buildOutput"
          :key="index"
          :class="['output-line', `output-${log.type}`]"
        >
          <span class="output-time">[{{ log.time }}]</span>
          <span class="output-message">{{ log.message }}</span>
        </div>
      </div>
    </div>

    <!-- 新建项目对话框 -->
    <mdui-dialog v-model:open="showNewProjectDialog" headline="新建项目">
      <mdui-text-field
        label="项目名称"
        v-model="newProjectName"
        style="margin-bottom: 16px"
      ></mdui-text-field>

      <mdui-select
        label="目标平台"
        v-model="newProjectTarget"
        style="margin-bottom: 16px"
      >
        <mdui-menu-item value="arm64">ARM64</mdui-menu-item>
        <mdui-menu-item value="x86_64">x86_64</mdui-menu-item>
      </mdui-select>

      <mdui-button slot="action" variant="text" @click="showNewProjectDialog = false">
        取消
      </mdui-button>
      <mdui-button slot="action" variant="filled" @click="createNewProject">
        创建
      </mdui-button>
    </mdui-dialog>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted, watch, nextTick } from 'vue'
import { storeToRefs } from 'pinia'
import { useIDEStore } from '../stores/ide'
import { setupMonacoST, createEditor } from '../utils/monaco-setup'
import FileTreeItem from '../components/FileTreeItem.vue'

// 导入 MDUI 组件
import 'mdui/components/top-app-bar.js'
import 'mdui/components/top-app-bar-title.js'
import 'mdui/components/button.js'
import 'mdui/components/button-icon.js'
import 'mdui/components/dialog.js'
import 'mdui/components/text-field.js'
import 'mdui/components/select.js'
import 'mdui/components/menu-item.js'

// 导入图标
import '@mdui/icons/menu.js'
import '@mdui/icons/create-new-folder.js'
import '@mdui/icons/folder-open.js'
import '@mdui/icons/folder-off.js'
import '@mdui/icons/build.js'
import '@mdui/icons/settings.js'
import '@mdui/icons/refresh.js'
import '@mdui/icons/description.js'
import '@mdui/icons/close.js'
import '@mdui/icons/delete.js'

const store = useIDEStore()
const {
  hasProject,
  projectName,
  projectFiles,
  openFiles,
  activeFile,
  building,
  buildOutput,
  sidebarCollapsed,
  outputPanelVisible,
  outputPanelHeight
} = storeToRefs(store)

const editorContainer = ref(null)
let editorInstance = null

const showNewProjectDialog = ref(false)
const newProjectName = ref('MyProject')
const newProjectTarget = ref('arm64')

// 初始化 Monaco Editor
onMounted(async () => {
  // 设置 ST 语言支持
  setupMonacoST()

  // 等待 DOM 渲染
  await nextTick()

  if (editorContainer.value) {
    editorInstance = createEditor(editorContainer.value, {
      language: 'st',
      value: '// 欢迎使用 Greyy IDE\n// 创建或打开一个项目开始编程\n'
    })

    // 监听内容变化
    editorInstance.onDidChangeModelContent(() => {
      if (activeFile.value) {
        const content = editorInstance.getValue()
        store.updateFileContent(activeFile.value.path, content)
      }
    })
  }
})

// 监听活动文件变化
watch(activeFile, async (newFile) => {
  if (newFile && editorInstance) {
    // 如果文件还没有加载内容
    if (!newFile.content) {
      const fullPath = `${store.projectPath}/${newFile.path}`
      const result = await window.electronAPI.readFile(fullPath)
      if (result.success) {
        newFile.content = result.content
      }
    }

    // 更新编辑器内容
    editorInstance.setValue(newFile.content || '')

    // 设置语言模式
    const ext = newFile.name.split('.').pop()
    const languageMap = {
      'st': 'st',
      'ld': 'xml', // 临时使用 XML
      'fbd': 'xml',
      'js': 'javascript',
      'json': 'json'
    }
    const language = languageMap[ext] || 'plaintext'
    const model = editorInstance.getModel()
    if (model) {
      monaco.editor.setModelLanguage(model, language)
    }
  }
})

// 切换侧边栏
const toggleSidebar = () => {
  store.toggleSidebar()
}

// 切换输出面板
const toggleOutputPanel = () => {
  store.toggleOutputPanel()
}

// 创建新项目
const createNewProject = async () => {
  const { dialog } = require('@electron/remote')
  const result = await dialog.showOpenDialog({
    properties: ['openDirectory', 'createDirectory']
  })

  if (!result.canceled) {
    const projectPath = result.filePaths[0] + '/' + newProjectName.value

    const createResult = await window.electronAPI.createProject(projectPath, {
      name: newProjectName.value,
      target: newProjectTarget.value
    })

    if (createResult.success) {
      store.setProject(createResult.config)
      store.setProjectPath(createResult.path)
      showNewProjectDialog.value = false
      await refreshProjectFiles()
      store.addBuildOutput(`项目创建成功: ${newProjectName.value}`, 'success')
    } else {
      store.addBuildOutput(`项目创建失败: ${createResult.error}`, 'error')
    }
  }
}

// 打开项目
const openProject = async () => {
  const result = await window.electronAPI.openProject()
  if (result && result.success) {
    store.setProject(result.config)
    store.setProjectPath(result.path)
    await refreshProjectFiles()
    store.addBuildOutput(`项目已打开: ${result.config.name}`, 'success')
  } else if (result && !result.success) {
    store.addBuildOutput(`打开项目失败: ${result.error}`, 'error')
  }
}

// 刷新项目文件
const refreshProjectFiles = async () => {
  if (!store.projectPath) return

  const result = await window.electronAPI.getProjectFiles(store.projectPath)
  if (result.success) {
    store.setProjectFiles(result.tree)
  }
}

// 处理文件选择
const handleFileSelect = async (item) => {
  if (item.type === 'file') {
    const file = {
      name: item.name,
      path: item.path,
      content: null,
      modified: false
    }
    store.openFile(file)
  }
}

// 关闭文件
const closeFile = (path) => {
  store.closeFile(path)
}

// 设置活动文件
const setActiveFile = (file) => {
  store.setActiveFile(file)
}

// 清空构建输出
const clearBuildOutput = () => {
  store.clearBuildOutput()
}

// 构建项目
const buildProject = async () => {
  if (!hasProject.value) return

  store.startBuilding()

  try {
    // 调用编译器（示例）
    const result = await window.electronAPI.spawn('make', ['all'], {
      cwd: store.projectPath
    })

    if (result.success) {
      // 监听输出
      window.electronAPI.onProcessOutput((data) => {
        store.addBuildOutput(data.data, 'info')
      })

      window.electronAPI.onProcessError((data) => {
        store.addBuildOutput(data.data, 'error')
      })

      window.electronAPI.onProcessExit((data) => {
        store.stopBuilding(data.code === 0)
        window.electronAPI.removeProcessListeners()
      })
    } else {
      store.addBuildOutput(`启动构建失败: ${result.error}`, 'error')
      store.stopBuilding(false)
    }
  } catch (error) {
    store.addBuildOutput(`构建错误: ${error.message}`, 'error')
    store.stopBuilding(false)
  }
}

// 清理
onUnmounted(() => {
  if (editorInstance) {
    editorInstance.dispose()
  }
  window.electronAPI.removeProcessListeners()
})
</script>

<style scoped>
.ide-view {
  display: flex;
  flex-direction: column;
  height: 100%;
  width: 100%;
}

.workspace {
  flex: 1;
  display: flex;
  overflow: hidden;
}

/* 侧边栏 */
.sidebar {
  width: 280px;
  background-color: rgb(var(--mdui-color-surface));
  border-right: 1px solid rgb(var(--mdui-color-surface-variant));
  display: flex;
  flex-direction: column;
  transition: width 0.3s;
}

.sidebar.collapsed {
  width: 0;
  overflow: hidden;
}

.sidebar-header {
  padding: 12px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-surface-variant));
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-weight: 500;
}

.file-tree {
  flex: 1;
  overflow-y: auto;
  padding: 8px;
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  height: 100%;
  color: rgb(var(--mdui-color-on-surface-variant));
  gap: 12px;
}

/* 编辑器区域 */
.editor-area {
  flex: 1;
  display: flex;
  flex-direction: column;
  overflow: hidden;
}

.editor-tabs {
  display: flex;
  background-color: rgb(var(--mdui-color-surface));
  border-bottom: 1px solid rgb(var(--mdui-color-surface-variant));
  overflow-x: auto;
  min-height: 40px;
}

.editor-tab {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  cursor: pointer;
  border-right: 1px solid rgb(var(--mdui-color-surface-variant));
  background-color: rgb(var(--mdui-color-surface-container));
  transition: background-color 0.2s;
}

.editor-tab:hover {
  background-color: rgb(var(--mdui-color-surface-container-high));
}

.editor-tab.active {
  background-color: rgb(var(--mdui-color-surface-container-highest));
}

.tab-label {
  font-size: 13px;
}

.modified-indicator {
  color: rgb(var(--mdui-color-primary));
  font-size: 18px;
}

.empty-tabs {
  display: flex;
  align-items: center;
  justify-content: center;
  flex: 1;
  color: rgb(var(--mdui-color-on-surface-variant));
  font-size: 14px;
}

.editor-container {
  flex: 1;
  overflow: hidden;
}

/* 输出面板 */
.output-panel {
  border-top: 1px solid rgb(var(--mdui-color-surface-variant));
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.output-header {
  padding: 8px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-surface-variant));
  display: flex;
  align-items: center;
  justify-content: space-between;
  font-weight: 500;
  font-size: 14px;
}

.output-actions {
  display: flex;
  gap: 4px;
}

.output-content {
  flex: 1;
  overflow-y: auto;
  padding: 12px;
  font-family: 'Menlo', 'Monaco', 'Courier New', monospace;
  font-size: 13px;
  background-color: rgba(0, 0, 0, 0.2);
}

.output-line {
  padding: 2px 0;
  line-height: 1.5;
}

.output-time {
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-right: 8px;
}

.output-message {
  color: rgb(var(--mdui-color-on-surface));
}

.output-info .output-message {
  color: rgb(var(--mdui-color-on-surface));
}

.output-success .output-message {
  color: #4caf50;
}

.output-error .output-message {
  color: #f44336;
}

.output-warning .output-message {
  color: #ff9800;
}
</style>

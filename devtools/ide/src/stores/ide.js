import { defineStore } from 'pinia'
import { ref, computed } from 'vue'

export const useIDEStore = defineStore('ide', () => {
  // 项目信息
  const project = ref(null)
  const projectPath = ref(null)
  const projectFiles = ref([])

  // 打开的文件
  const openFiles = ref([])
  const activeFile = ref(null)

  // 编辑器状态
  const editorContent = ref('')
  const editorLanguage = ref('st')
  const editorModified = ref(false)

  // 构建输出
  const buildOutput = ref([])
  const building = ref(false)
  const buildSuccess = ref(null)

  // UI 状态
  const sidebarCollapsed = ref(false)
  const outputPanelVisible = ref(true)
  const outputPanelHeight = ref(200)

  // Computed
  const hasProject = computed(() => project.value !== null)
  const hasOpenFiles = computed(() => openFiles.value.length > 0)
  const projectName = computed(() => project.value?.name || 'Untitled')

  // Actions - 项目管理
  function setProject(proj) {
    project.value = proj
  }

  function setProjectPath(path) {
    projectPath.value = path
  }

  function setProjectFiles(files) {
    projectFiles.value = files
  }

  function closeProject() {
    project.value = null
    projectPath.value = null
    projectFiles.value = []
    openFiles.value = []
    activeFile.value = null
  }

  // Actions - 文件管理
  function openFile(file) {
    const existing = openFiles.value.find(f => f.path === file.path)
    if (existing) {
      activeFile.value = existing
    } else {
      openFiles.value.push(file)
      activeFile.value = file
    }
  }

  function closeFile(filePath) {
    const index = openFiles.value.findIndex(f => f.path === filePath)
    if (index !== -1) {
      openFiles.value.splice(index, 1)
      if (activeFile.value?.path === filePath) {
        activeFile.value = openFiles.value[index] || openFiles.value[index - 1] || null
      }
    }
  }

  function setActiveFile(file) {
    activeFile.value = file
  }

  function updateFileContent(filePath, content) {
    const file = openFiles.value.find(f => f.path === filePath)
    if (file) {
      file.content = content
      file.modified = true
    }
  }

  function markFileSaved(filePath) {
    const file = openFiles.value.find(f => f.path === filePath)
    if (file) {
      file.modified = false
    }
  }

  // Actions - 编辑器
  function setEditorContent(content) {
    editorContent.value = content
  }

  function setEditorLanguage(lang) {
    editorLanguage.value = lang
  }

  function setEditorModified(modified) {
    editorModified.value = modified
  }

  // Actions - 构���
  function addBuildOutput(message, type = 'info') {
    const timestamp = new Date().toLocaleTimeString('zh-CN', { hour12: false })
    buildOutput.value.push({
      time: timestamp,
      message,
      type // 'info', 'success', 'error', 'warning'
    })
  }

  function clearBuildOutput() {
    buildOutput.value = []
  }

  function startBuilding() {
    building.value = true
    buildSuccess.value = null
    addBuildOutput('开始构建...', 'info')
  }

  function stopBuilding(success = false) {
    building.value = false
    buildSuccess.value = success
    if (success) {
      addBuildOutput('构建成功！', 'success')
    } else {
      addBuildOutput('构建失败', 'error')
    }
  }

  // Actions - UI
  function toggleSidebar() {
    sidebarCollapsed.value = !sidebarCollapsed.value
  }

  function toggleOutputPanel() {
    outputPanelVisible.value = !outputPanelVisible.value
  }

  function setOutputPanelHeight(height) {
    outputPanelHeight.value = height
  }

  return {
    // State
    project,
    projectPath,
    projectFiles,
    openFiles,
    activeFile,
    editorContent,
    editorLanguage,
    editorModified,
    buildOutput,
    building,
    buildSuccess,
    sidebarCollapsed,
    outputPanelVisible,
    outputPanelHeight,

    // Computed
    hasProject,
    hasOpenFiles,
    projectName,

    // Actions
    setProject,
    setProjectPath,
    setProjectFiles,
    closeProject,
    openFile,
    closeFile,
    setActiveFile,
    updateFileContent,
    markFileSaved,
    setEditorContent,
    setEditorLanguage,
    setEditorModified,
    addBuildOutput,
    clearBuildOutput,
    startBuilding,
    stopBuilding,
    toggleSidebar,
    toggleOutputPanel,
    setOutputPanelHeight
  }
})

import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { POU } from '@/types/iec61131'

export interface EditorTab {
  id: string
  name: string
  pouId: string
  type: 'pou' | 'variables' | 'config'
  isModified: boolean
}

export interface CursorPosition {
  line: number
  column: number
}

export const useEditorStore = defineStore('editor', () => {
  // 状态
  const tabs = ref<EditorTab[]>([])
  const activeTabId = ref<string | null>(null)
  const cursorPosition = ref<CursorPosition>({ line: 1, column: 1 })

  // 计算属性
  const activeTab = computed(() => tabs.value.find(t => t.id === activeTabId.value))
  const activeFile = computed(() => activeTab.value ? { name: activeTab.value.name } : null)
  const hasModifiedTabs = computed(() => tabs.value.some(t => t.isModified))

  // 打开标签页
  function openTab(pou: POU, type: 'pou' | 'variables' | 'config' = 'pou'): EditorTab {
    // 检查是否已打开
    const existing = tabs.value.find(t => t.pouId === pou.id && t.type === type)
    if (existing) {
      activeTabId.value = existing.id
      return existing
    }

    // 创建新标签
    const tab: EditorTab = {
      id: `${pou.id}-${type}`,
      name: type === 'pou' ? pou.name : `${pou.name} (${type === 'variables' ? '变量' : '配置'})`,
      pouId: pou.id,
      type,
      isModified: false
    }

    tabs.value.push(tab)
    activeTabId.value = tab.id
    return tab
  }

  // 关闭标签页
  function closeTab(tabId: string): boolean {
    const index = tabs.value.findIndex(t => t.id === tabId)
    if (index >= 0) {
      tabs.value.splice(index, 1)

      // 如果关闭的是当前标签，切换到其他标签
      if (activeTabId.value === tabId) {
        if (tabs.value.length > 0) {
          activeTabId.value = tabs.value[Math.min(index, tabs.value.length - 1)].id
        } else {
          activeTabId.value = null
        }
      }
      return true
    }
    return false
  }

  // 关闭所有标签
  function closeAllTabs(): void {
    tabs.value = []
    activeTabId.value = null
  }

  // 关闭其他标签
  function closeOtherTabs(tabId: string): void {
    tabs.value = tabs.value.filter(t => t.id === tabId)
    activeTabId.value = tabId
  }

  // 设置活动标签
  function setActiveTab(tabId: string): void {
    if (tabs.value.some(t => t.id === tabId)) {
      activeTabId.value = tabId
    }
  }

  // 标记为已修改
  function markModified(tabId: string, modified: boolean = true): void {
    const tab = tabs.value.find(t => t.id === tabId)
    if (tab) {
      tab.isModified = modified
    }
  }

  // 更新光标位置
  function updateCursorPosition(line: number, column: number): void {
    cursorPosition.value = { line, column }
  }

  // 保存当前文件
  async function saveCurrentFile(): Promise<boolean> {
    if (!activeTab.value) return false
    // TODO: 实现保存逻辑
    markModified(activeTab.value.id, false)
    return true
  }

  // 保存所有文件
  async function saveAllFiles(): Promise<boolean> {
    for (const tab of tabs.value) {
      if (tab.isModified) {
        // TODO: 实现保存逻辑
        tab.isModified = false
      }
    }
    return true
  }

  return {
    // 状态
    tabs,
    activeTabId,
    cursorPosition,
    // 计算属性
    activeTab,
    activeFile,
    hasModifiedTabs,
    // 方法
    openTab,
    closeTab,
    closeAllTabs,
    closeOtherTabs,
    setActiveTab,
    markModified,
    updateCursorPosition,
    saveCurrentFile,
    saveAllFiles
  }
})

<template>
  <div class="welcome-view">
    <div class="welcome-content">
      <div class="logo">
        <mdui-icon-memory style="font-size: 64px; color: rgb(var(--mdui-color-primary));"></mdui-icon-memory>
      </div>
      <h1>SoftPLC IDE</h1>
      <p class="subtitle">IEC 61131-3 标准软PLC开发环境</p>

      <div class="actions">
        <mdui-card variant="outlined" class="action-card" @click="createNewProject">
          <mdui-icon-create-new-folder style="font-size: 32px;"></mdui-icon-create-new-folder>
          <span class="action-title">新建项目</span>
          <span class="action-desc">创建新的 PLC 项目</span>
        </mdui-card>

        <mdui-card variant="outlined" class="action-card" @click="openProject">
          <mdui-icon-folder-open style="font-size: 32px;"></mdui-icon-folder-open>
          <span class="action-title">打开项目</span>
          <span class="action-desc">打开已有项目</span>
        </mdui-card>

        <mdui-card variant="outlined" class="action-card">
          <mdui-icon-school style="font-size: 32px;"></mdui-icon-school>
          <span class="action-title">学习资源</span>
          <span class="action-desc">IEC 61131-3 教程</span>
        </mdui-card>
      </div>

      <div class="recent-projects" v-if="recentProjects.length > 0">
        <h3>最近项目</h3>
        <mdui-list>
          <mdui-list-item
            v-for="project in recentProjects"
            :key="project.path"
            :headline="project.name"
            :description="project.path"
          >
            <mdui-icon-history slot="icon"></mdui-icon-history>
          </mdui-list-item>
        </mdui-list>
      </div>

      <div class="features">
        <h3>支持的编程语言</h3>
        <div class="feature-grid">
          <div class="feature-item">
            <mdui-icon-code style="color: rgb(var(--mdui-color-primary));"></mdui-icon-code>
            <span>ST</span>
            <small>结构化文本</small>
          </div>
          <div class="feature-item">
            <mdui-icon-view-week style="color: rgb(var(--mdui-color-secondary));"></mdui-icon-view-week>
            <span>LD</span>
            <small>梯形图</small>
          </div>
          <div class="feature-item">
            <mdui-icon-account-tree style="color: rgb(var(--mdui-color-tertiary));"></mdui-icon-account-tree>
            <span>FBD</span>
            <small>功能块图</small>
          </div>
          <div class="feature-item">
            <mdui-icon-format-list-numbered style="color: rgb(var(--mdui-color-error));"></mdui-icon-format-list-numbered>
            <span>IL</span>
            <small>指令表</small>
          </div>
          <div class="feature-item">
            <mdui-icon-route style="color: #9c27b0;"></mdui-icon-route>
            <span>SFC</span>
            <small>顺序功能图</small>
          </div>
        </div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { useProjectStore } from '@/stores/project'

import '@mdui/icons/memory.js'
import '@mdui/icons/create-new-folder.js'
import '@mdui/icons/folder-open.js'
import '@mdui/icons/school.js'
import '@mdui/icons/history.js'
import '@mdui/icons/code.js'
import '@mdui/icons/view-week.js'
import '@mdui/icons/account-tree.js'
import '@mdui/icons/format-list-numbered.js'
import '@mdui/icons/route.js'

const projectStore = useProjectStore()

interface RecentProject {
  name: string
  path: string
}

const recentProjects = ref<RecentProject[]>([])

function createNewProject() {
  // 通过 App.vue 中的 dialog 创建
  const event = new CustomEvent('create-new-project')
  window.dispatchEvent(event)
}

function openProject() {
  projectStore.openProject()
}
</script>

<style scoped>
.welcome-view {
  height: 100%;
  overflow: auto;
  display: flex;
  justify-content: center;
  padding: 40px 20px;
  background-color: rgb(var(--mdui-color-surface));
}

.welcome-content {
  max-width: 800px;
  text-align: center;
}

.logo {
  margin-bottom: 16px;
}

h1 {
  margin: 0;
  font-size: 32px;
  font-weight: 500;
  color: rgb(var(--mdui-color-on-surface));
}

.subtitle {
  margin: 8px 0 32px;
  font-size: 16px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.actions {
  display: flex;
  justify-content: center;
  gap: 16px;
  margin-bottom: 40px;
}

.action-card {
  display: flex;
  flex-direction: column;
  align-items: center;
  padding: 24px;
  width: 160px;
  cursor: pointer;
  transition: all 0.2s;
}

.action-card:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.action-title {
  margin-top: 12px;
  font-size: 14px;
  font-weight: 500;
}

.action-desc {
  margin-top: 4px;
  font-size: 12px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.recent-projects {
  margin-bottom: 40px;
  text-align: left;
}

.recent-projects h3 {
  font-size: 14px;
  font-weight: 500;
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-bottom: 8px;
}

.features h3 {
  font-size: 14px;
  font-weight: 500;
  color: rgb(var(--mdui-color-on-surface-variant));
  margin-bottom: 16px;
}

.feature-grid {
  display: flex;
  justify-content: center;
  gap: 24px;
}

.feature-item {
  display: flex;
  flex-direction: column;
  align-items: center;
  gap: 4px;
}

.feature-item span {
  font-size: 16px;
  font-weight: 500;
}

.feature-item small {
  font-size: 12px;
  color: rgb(var(--mdui-color-on-surface-variant));
}
</style>

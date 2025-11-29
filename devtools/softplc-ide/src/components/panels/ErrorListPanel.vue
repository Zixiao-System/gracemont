<template>
  <div class="error-list-panel">
    <div class="panel-toolbar">
      <span class="error-count">
        <mdui-icon-error style="color: #f44336;"></mdui-icon-error>
        {{ errors.length }} 错误
      </span>
      <span class="warning-count">
        <mdui-icon-warning style="color: #ff9800;"></mdui-icon-warning>
        {{ warnings.length }} 警告
      </span>
    </div>
    <div class="error-content">
      <table class="error-table">
        <thead>
          <tr>
            <th style="width: 40px;"></th>
            <th style="width: 80px;">代码</th>
            <th>描述</th>
            <th style="width: 150px;">文件</th>
            <th style="width: 60px;">行</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="error in errors" :key="error.code + error.line" @dblclick="gotoError(error)">
            <td><mdui-icon-error style="color: #f44336;"></mdui-icon-error></td>
            <td>{{ error.code }}</td>
            <td>{{ error.message }}</td>
            <td>{{ error.file }}</td>
            <td>{{ error.line }}</td>
          </tr>
          <tr v-for="warning in warnings" :key="warning.code + warning.line" @dblclick="gotoError(warning)">
            <td><mdui-icon-warning style="color: #ff9800;"></mdui-icon-warning></td>
            <td>{{ warning.code }}</td>
            <td>{{ warning.message }}</td>
            <td>{{ warning.file }}</td>
            <td>{{ warning.line }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import { useProjectStore } from '@/stores/project'
import type { CompileError, CompileWarning } from '@/types/iec61131'

import '@mdui/icons/error.js'
import '@mdui/icons/warning.js'

const projectStore = useProjectStore()

const errors = computed(() => projectStore.errors)
const warnings = computed(() => projectStore.warnings)

function gotoError(error: CompileError | CompileWarning) {
  // TODO: 跳转到错误位置
  console.log('Go to:', error.file, error.line)
}
</script>

<style scoped>
.error-list-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-toolbar {
  display: flex;
  align-items: center;
  gap: 16px;
  padding: 8px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.error-count,
.warning-count {
  display: flex;
  align-items: center;
  gap: 4px;
  font-size: 13px;
}

.error-content {
  flex: 1;
  overflow: auto;
}

.error-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}

.error-table th {
  text-align: left;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  font-weight: 500;
}

.error-table td {
  padding: 6px 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.error-table tr:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
  cursor: pointer;
}
</style>

<template>
  <div class="crossref-panel">
    <div class="panel-toolbar">
      <mdui-text-field
        v-model="searchText"
        placeholder="搜索变量..."
        variant="outlined"
        style="width: 200px; --mdui-comp-text-field-height: 32px;"
      >
        <mdui-icon-search slot="icon"></mdui-icon-search>
      </mdui-text-field>
    </div>
    <div class="crossref-content">
      <table class="crossref-table">
        <thead>
          <tr>
            <th style="width: 150px;">变量名</th>
            <th style="width: 100px;">类型</th>
            <th>使用位置</th>
            <th style="width: 80px;">读/写</th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="ref in filteredRefs" :key="ref.variable" @dblclick="gotoReference(ref)">
            <td>{{ ref.variable }}</td>
            <td>{{ ref.dataType }}</td>
            <td>{{ ref.location }}</td>
            <td>{{ ref.accessType }}</td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'

import '@mdui/icons/search.js'

interface CrossReference {
  variable: string
  dataType: string
  location: string
  accessType: 'R' | 'W' | 'R/W'
}

const searchText = ref('')
const crossRefs = ref<CrossReference[]>([])

const filteredRefs = computed(() => {
  if (!searchText.value) return crossRefs.value
  const search = searchText.value.toLowerCase()
  return crossRefs.value.filter(r => r.variable.toLowerCase().includes(search))
})

function gotoReference(ref: CrossReference) {
  // TODO: 跳转到引用位置
  console.log('Go to:', ref.location)
}
</script>

<style scoped>
.crossref-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-toolbar {
  display: flex;
  padding: 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.crossref-content {
  flex: 1;
  overflow: auto;
}

.crossref-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}

.crossref-table th {
  text-align: left;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  font-weight: 500;
}

.crossref-table td {
  padding: 6px 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.crossref-table tr:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
  cursor: pointer;
}
</style>

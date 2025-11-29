<template>
  <div class="watch-panel">
    <div class="panel-toolbar">
      <mdui-button-icon @click="addWatch">
        <mdui-icon-add></mdui-icon-add>
      </mdui-button-icon>
      <mdui-button-icon @click="removeAllWatch">
        <mdui-icon-delete-sweep></mdui-icon-delete-sweep>
      </mdui-button-icon>
      <mdui-button-icon @click="refreshWatch" :disabled="!plcStore.isMonitoring">
        <mdui-icon-refresh></mdui-icon-refresh>
      </mdui-button-icon>
    </div>
    <div class="watch-content">
      <table class="watch-table">
        <thead>
          <tr>
            <th style="width: 200px;">名称</th>
            <th style="width: 100px;">类型</th>
            <th style="width: 150px;">当前值</th>
            <th>地址</th>
            <th style="width: 40px;"></th>
          </tr>
        </thead>
        <tbody>
          <tr v-for="item in watchList" :key="item.id">
            <td>{{ item.name }}</td>
            <td>{{ item.dataType }}</td>
            <td :class="{ 'value-changed': item.changed }">
              {{ item.value }}
            </td>
            <td>{{ item.address }}</td>
            <td>
              <mdui-button-icon @click="removeWatch(item.id)">
                <mdui-icon-close></mdui-icon-close>
              </mdui-button-icon>
            </td>
          </tr>
        </tbody>
      </table>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref } from 'vue'
import { usePlcStore } from '@/stores/plc'

import '@mdui/icons/add.js'
import '@mdui/icons/delete-sweep.js'
import '@mdui/icons/refresh.js'
import '@mdui/icons/close.js'

interface WatchItem {
  id: string
  name: string
  dataType: string
  value: string
  address: string
  changed: boolean
}

const plcStore = usePlcStore()
const watchList = ref<WatchItem[]>([])

function addWatch() {
  // TODO: 打开添加监视对话框
}

function removeWatch(id: string) {
  watchList.value = watchList.value.filter(w => w.id !== id)
}

function removeAllWatch() {
  watchList.value = []
}

function refreshWatch() {
  // TODO: 刷新监视值
}
</script>

<style scoped>
.watch-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-toolbar {
  display: flex;
  padding: 4px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.watch-content {
  flex: 1;
  overflow: auto;
}

.watch-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 12px;
}

.watch-table th {
  text-align: left;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  font-weight: 500;
}

.watch-table td {
  padding: 6px 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.value-changed {
  color: rgb(var(--mdui-color-primary));
  font-weight: 500;
}
</style>

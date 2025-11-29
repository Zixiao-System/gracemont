<template>
  <div class="output-panel">
    <div class="panel-toolbar">
      <mdui-button-icon @click="clearOutput">
        <mdui-icon-delete-sweep></mdui-icon-delete-sweep>
      </mdui-button-icon>
      <mdui-button-icon @click="scrollToBottom">
        <mdui-icon-vertical-align-bottom></mdui-icon-vertical-align-bottom>
      </mdui-button-icon>
    </div>
    <div class="output-content" ref="outputContent">
      <div
        v-for="(message, index) in plcStore.outputMessages"
        :key="index"
        class="output-line"
      >
        {{ message }}
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, watch, nextTick } from 'vue'
import { usePlcStore } from '@/stores/plc'

import '@mdui/icons/delete-sweep.js'
import '@mdui/icons/vertical-align-bottom.js'

const plcStore = usePlcStore()
const outputContent = ref<HTMLElement | null>(null)

function clearOutput() {
  plcStore.clearOutput()
}

function scrollToBottom() {
  if (outputContent.value) {
    outputContent.value.scrollTop = outputContent.value.scrollHeight
  }
}

watch(() => plcStore.outputMessages.length, () => {
  nextTick(scrollToBottom)
})
</script>

<style scoped>
.output-panel {
  height: 100%;
  display: flex;
  flex-direction: column;
}

.panel-toolbar {
  display: flex;
  padding: 4px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
}

.output-content {
  flex: 1;
  overflow: auto;
  font-family: 'Consolas', 'Monaco', monospace;
  font-size: 12px;
  padding: 8px;
  background-color: rgb(var(--mdui-color-surface-container-lowest));
}

.output-line {
  padding: 2px 0;
  white-space: pre-wrap;
  word-break: break-all;
}
</style>

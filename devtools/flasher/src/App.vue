<template>
  <div class="flasher-app">
    <router-view />
  </div>
</template>

<script setup>
import { onMounted } from 'vue'
import { setTheme } from 'mdui/functions/setTheme.js'

onMounted(() => {
  // 设置暗色主题
  setTheme('dark')
})
</script>

<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}

html, body, #app {
  width: 100%;
  height: 100%;
  overflow: hidden;
  font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
}

.flasher-app {
  width: 100%;
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-background));
  color: rgb(var(--mdui-color-on-background));
}
</style>

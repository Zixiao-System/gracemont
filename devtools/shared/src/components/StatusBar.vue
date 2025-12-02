<template>
  <div class="status-bar">
    <div class="status-left">
      <slot name="left">
        <span v-for="(item, idx) in leftItems" :key="idx" class="status-item">
          <component :is="item.icon" v-if="item.icon" class="status-icon"></component>
          <span>{{ item.text }}</span>
        </span>
      </slot>
    </div>
    <div class="status-center">
      <slot name="center"></slot>
    </div>
    <div class="status-right">
      <slot name="right">
        <span v-for="(item, idx) in rightItems" :key="idx" class="status-item">
          <component :is="item.icon" v-if="item.icon" class="status-icon"></component>
          <span>{{ item.text }}</span>
        </span>
      </slot>
    </div>
  </div>
</template>

<script setup>
defineProps({
  leftItems: {
    type: Array,
    default: () => []
  },
  rightItems: {
    type: Array,
    default: () => []
  }
})
</script>

<style scoped>
.status-bar {
  display: flex;
  align-items: center;
  justify-content: space-between;
  height: 24px;
  padding: 0 12px;
  background: #007acc;
  color: white;
  font-size: 12px;
  user-select: none;
}

.status-left,
.status-center,
.status-right {
  display: flex;
  align-items: center;
  gap: 16px;
}

.status-item {
  display: flex;
  align-items: center;
  gap: 4px;
  cursor: default;
}

.status-item:hover {
  background: rgba(255, 255, 255, 0.1);
}

.status-icon {
  font-size: 14px;
}
</style>

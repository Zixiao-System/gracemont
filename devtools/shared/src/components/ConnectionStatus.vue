<template>
  <div class="connection-status" :class="statusClass">
    <div class="status-indicator"></div>
    <div class="status-text">
      <span class="status-label">{{ label }}</span>
      <span class="status-detail" v-if="detail">{{ detail }}</span>
    </div>
    <slot></slot>
  </div>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  connected: {
    type: Boolean,
    default: false
  },
  connecting: {
    type: Boolean,
    default: false
  },
  error: {
    type: Boolean,
    default: false
  },
  label: {
    type: String,
    default: ''
  },
  detail: {
    type: String,
    default: ''
  }
})

const statusClass = computed(() => {
  if (props.error) return 'error'
  if (props.connecting) return 'connecting'
  if (props.connected) return 'connected'
  return 'disconnected'
})
</script>

<style scoped>
.connection-status {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 8px 12px;
  border-radius: 8px;
  background: #f5f5f5;
}

.status-indicator {
  width: 10px;
  height: 10px;
  border-radius: 50%;
  flex-shrink: 0;
}

.connection-status.connected .status-indicator {
  background: #4caf50;
  box-shadow: 0 0 8px rgba(76, 175, 80, 0.5);
}

.connection-status.disconnected .status-indicator {
  background: #9e9e9e;
}

.connection-status.connecting .status-indicator {
  background: #ff9800;
  animation: pulse 1s infinite;
}

.connection-status.error .status-indicator {
  background: #f44336;
  box-shadow: 0 0 8px rgba(244, 67, 54, 0.5);
}

@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.4; }
}

.status-text {
  display: flex;
  flex-direction: column;
  flex: 1;
}

.status-label {
  font-size: 13px;
  font-weight: 500;
}

.status-detail {
  font-size: 11px;
  color: #666;
}
</style>

<template>
  <mdui-card class="progress-card" :class="{ error: hasError }">
    <div class="card-header">
      <div class="card-icon">
        <slot name="icon">
          <mdui-icon-info></mdui-icon-info>
        </slot>
      </div>
      <div class="card-title">
        <h3>{{ title }}</h3>
        <p v-if="subtitle">{{ subtitle }}</p>
      </div>
      <div class="card-status">
        <mdui-chip v-if="status" :variant="statusVariant">
          {{ status }}
        </mdui-chip>
      </div>
    </div>

    <div class="card-progress" v-if="showProgress">
      <mdui-linear-progress :value="progress / 100"></mdui-linear-progress>
      <div class="progress-info">
        <span>{{ progressText || `${progress}%` }}</span>
        <span v-if="speed">{{ speed }}</span>
      </div>
    </div>

    <div class="card-details" v-if="details.length > 0">
      <div v-for="(item, idx) in details" :key="idx" class="detail-item">
        <span class="detail-label">{{ item.label }}</span>
        <span class="detail-value">{{ item.value }}</span>
      </div>
    </div>

    <div class="card-actions" v-if="$slots.actions">
      <slot name="actions"></slot>
    </div>
  </mdui-card>
</template>

<script setup>
import { computed } from 'vue'
import '@mdui/icons/info.js'

const props = defineProps({
  title: {
    type: String,
    required: true
  },
  subtitle: {
    type: String,
    default: ''
  },
  status: {
    type: String,
    default: ''
  },
  statusType: {
    type: String,
    default: 'default' // 'default', 'success', 'warning', 'error'
  },
  showProgress: {
    type: Boolean,
    default: false
  },
  progress: {
    type: Number,
    default: 0
  },
  progressText: {
    type: String,
    default: ''
  },
  speed: {
    type: String,
    default: ''
  },
  details: {
    type: Array,
    default: () => []
  },
  hasError: {
    type: Boolean,
    default: false
  }
})

const statusVariant = computed(() => {
  switch (props.statusType) {
    case 'success': return 'elevated'
    case 'warning': return 'elevated'
    case 'error': return 'elevated'
    default: return 'outlined'
  }
})
</script>

<style scoped>
.progress-card {
  padding: 16px;
}

.progress-card.error {
  border-left: 4px solid #f44336;
}

.card-header {
  display: flex;
  align-items: flex-start;
  gap: 12px;
  margin-bottom: 16px;
}

.card-icon {
  font-size: 32px;
  color: #666;
}

.card-title {
  flex: 1;
}

.card-title h3 {
  margin: 0;
  font-size: 16px;
  font-weight: 500;
}

.card-title p {
  margin: 4px 0 0;
  font-size: 13px;
  color: #666;
}

.card-progress {
  margin-bottom: 16px;
}

.progress-info {
  display: flex;
  justify-content: space-between;
  margin-top: 8px;
  font-size: 12px;
  color: #666;
}

.card-details {
  display: grid;
  grid-template-columns: repeat(2, 1fr);
  gap: 8px 16px;
  padding: 12px;
  background: #f5f5f5;
  border-radius: 8px;
  margin-bottom: 16px;
}

.detail-item {
  display: flex;
  justify-content: space-between;
  font-size: 13px;
}

.detail-label {
  color: #666;
}

.detail-value {
  font-weight: 500;
  font-family: monospace;
}

.card-actions {
  display: flex;
  gap: 8px;
  justify-content: flex-end;
}
</style>

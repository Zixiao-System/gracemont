<template>
  <div class="split-pane" :class="{ vertical: direction === 'vertical' }">
    <div
      class="pane pane-first"
      :style="firstPaneStyle"
    >
      <slot name="first"></slot>
    </div>
    <div
      class="splitter"
      @mousedown="startDrag"
    >
      <div class="splitter-handle"></div>
    </div>
    <div class="pane pane-second">
      <slot name="second"></slot>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, onMounted, onUnmounted } from 'vue'

const props = defineProps({
  direction: {
    type: String,
    default: 'horizontal' // 'horizontal' | 'vertical'
  },
  initialSize: {
    type: Number,
    default: 50 // 百分比
  },
  minSize: {
    type: Number,
    default: 10 // 百分比
  },
  maxSize: {
    type: Number,
    default: 90 // 百分比
  }
})

const emit = defineEmits(['resize'])

const size = ref(props.initialSize)
const isDragging = ref(false)

const firstPaneStyle = computed(() => {
  if (props.direction === 'horizontal') {
    return { width: `${size.value}%` }
  } else {
    return { height: `${size.value}%` }
  }
})

function startDrag(e) {
  isDragging.value = true
  document.addEventListener('mousemove', onDrag)
  document.addEventListener('mouseup', stopDrag)
  e.preventDefault()
}

function onDrag(e) {
  if (!isDragging.value) return

  const container = e.target.closest('.split-pane')
  if (!container) return

  const rect = container.getBoundingClientRect()
  let newSize

  if (props.direction === 'horizontal') {
    newSize = ((e.clientX - rect.left) / rect.width) * 100
  } else {
    newSize = ((e.clientY - rect.top) / rect.height) * 100
  }

  // 限制范围
  newSize = Math.max(props.minSize, Math.min(props.maxSize, newSize))
  size.value = newSize
  emit('resize', newSize)
}

function stopDrag() {
  isDragging.value = false
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
}

onUnmounted(() => {
  document.removeEventListener('mousemove', onDrag)
  document.removeEventListener('mouseup', stopDrag)
})
</script>

<style scoped>
.split-pane {
  display: flex;
  width: 100%;
  height: 100%;
  overflow: hidden;
}

.split-pane.vertical {
  flex-direction: column;
}

.pane {
  overflow: auto;
}

.pane-second {
  flex: 1;
}

.splitter {
  display: flex;
  align-items: center;
  justify-content: center;
  background: #e0e0e0;
  cursor: col-resize;
  user-select: none;
  flex-shrink: 0;
}

.split-pane:not(.vertical) .splitter {
  width: 6px;
  cursor: col-resize;
}

.split-pane.vertical .splitter {
  height: 6px;
  cursor: row-resize;
}

.splitter-handle {
  background: #bdbdbd;
  border-radius: 2px;
}

.split-pane:not(.vertical) .splitter-handle {
  width: 2px;
  height: 32px;
}

.split-pane.vertical .splitter-handle {
  width: 32px;
  height: 2px;
}

.splitter:hover {
  background: #d0d0d0;
}

.splitter:hover .splitter-handle {
  background: #9e9e9e;
}
</style>

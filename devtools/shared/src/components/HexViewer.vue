<template>
  <div class="hex-viewer">
    <div class="hex-header">
      <div class="hex-offset-header">Offset</div>
      <div class="hex-bytes-header">
        <span v-for="i in bytesPerRow" :key="i">{{ formatHex(i - 1, 2) }}</span>
      </div>
      <div class="hex-ascii-header">ASCII</div>
    </div>
    <div class="hex-content" ref="contentRef">
      <div
        v-for="(row, rowIdx) in displayRows"
        :key="rowIdx"
        class="hex-row"
        :class="{ selected: isRowSelected(rowIdx) }"
      >
        <div class="hex-offset">{{ formatHex(row.offset, 8) }}</div>
        <div class="hex-bytes">
          <span
            v-for="(byte, byteIdx) in row.bytes"
            :key="byteIdx"
            class="hex-byte"
            :class="{
              selected: isSelected(row.offset + byteIdx),
              modified: isModified(row.offset + byteIdx),
              null: byte === null
            }"
            @click="selectByte(row.offset + byteIdx)"
          >
            {{ byte !== null ? formatHex(byte, 2) : '  ' }}
          </span>
        </div>
        <div class="hex-ascii">
          <span
            v-for="(byte, byteIdx) in row.bytes"
            :key="byteIdx"
            class="ascii-char"
            :class="{ selected: isSelected(row.offset + byteIdx) }"
          >
            {{ byte !== null ? toAscii(byte) : ' ' }}
          </span>
        </div>
      </div>
    </div>
    <div class="hex-footer">
      <span>大小: {{ formatSize(data.length) }}</span>
      <span v-if="selectedOffset !== null">
        偏移: 0x{{ formatHex(selectedOffset, 8) }} ({{ selectedOffset }})
      </span>
      <span v-if="selectedOffset !== null && data[selectedOffset] !== undefined">
        值: 0x{{ formatHex(data[selectedOffset], 2) }} ({{ data[selectedOffset] }})
      </span>
    </div>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  data: {
    type: Uint8Array,
    default: () => new Uint8Array(0)
  },
  bytesPerRow: {
    type: Number,
    default: 16
  },
  modifiedOffsets: {
    type: Set,
    default: () => new Set()
  }
})

const emit = defineEmits(['select'])

const selectedOffset = ref(null)
const selectionStart = ref(null)
const selectionEnd = ref(null)

const displayRows = computed(() => {
  const rows = []
  const length = props.data.length
  const bytesPerRow = props.bytesPerRow

  for (let offset = 0; offset < length; offset += bytesPerRow) {
    const bytes = []
    for (let i = 0; i < bytesPerRow; i++) {
      const idx = offset + i
      bytes.push(idx < length ? props.data[idx] : null)
    }
    rows.push({ offset, bytes })
  }

  // 至少显示一行
  if (rows.length === 0) {
    rows.push({ offset: 0, bytes: Array(bytesPerRow).fill(null) })
  }

  return rows
})

function formatHex(value, digits) {
  return value.toString(16).toUpperCase().padStart(digits, '0')
}

function toAscii(byte) {
  if (byte >= 32 && byte <= 126) {
    return String.fromCharCode(byte)
  }
  return '.'
}

function formatSize(bytes) {
  if (bytes === 0) return '0 B'
  const units = ['B', 'KB', 'MB', 'GB']
  const i = Math.floor(Math.log(bytes) / Math.log(1024))
  return (bytes / Math.pow(1024, i)).toFixed(i > 0 ? 2 : 0) + ' ' + units[i]
}

function selectByte(offset) {
  if (offset >= props.data.length) return
  selectedOffset.value = offset
  emit('select', offset)
}

function isSelected(offset) {
  if (selectionStart.value !== null && selectionEnd.value !== null) {
    const start = Math.min(selectionStart.value, selectionEnd.value)
    const end = Math.max(selectionStart.value, selectionEnd.value)
    return offset >= start && offset <= end
  }
  return offset === selectedOffset.value
}

function isRowSelected(rowIdx) {
  const row = displayRows.value[rowIdx]
  if (!row) return false
  for (let i = 0; i < props.bytesPerRow; i++) {
    if (isSelected(row.offset + i)) return true
  }
  return false
}

function isModified(offset) {
  return props.modifiedOffsets.has(offset)
}
</script>

<style scoped>
.hex-viewer {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #1e1e1e;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', 'Consolas', monospace;
  font-size: 13px;
  color: #d4d4d4;
}

.hex-header {
  display: flex;
  padding: 8px 12px;
  background: #252526;
  border-bottom: 1px solid #333;
  color: #888;
  font-weight: 500;
}

.hex-offset-header {
  width: 80px;
  flex-shrink: 0;
}

.hex-bytes-header {
  display: flex;
  gap: 8px;
  flex: 1;
}

.hex-bytes-header span {
  width: 20px;
  text-align: center;
}

.hex-ascii-header {
  width: 160px;
  flex-shrink: 0;
  text-align: center;
}

.hex-content {
  flex: 1;
  overflow: auto;
  padding: 4px 12px;
}

.hex-row {
  display: flex;
  line-height: 1.8;
}

.hex-row:hover {
  background: #2a2d2e;
}

.hex-row.selected {
  background: #264f78;
}

.hex-offset {
  width: 80px;
  flex-shrink: 0;
  color: #569cd6;
}

.hex-bytes {
  display: flex;
  gap: 8px;
  flex: 1;
}

.hex-byte {
  width: 20px;
  text-align: center;
  cursor: pointer;
  border-radius: 2px;
}

.hex-byte:hover {
  background: #3d3d3d;
}

.hex-byte.selected {
  background: #094771;
  color: #fff;
}

.hex-byte.modified {
  color: #f48771;
}

.hex-byte.null {
  color: #444;
}

.hex-ascii {
  width: 160px;
  flex-shrink: 0;
  letter-spacing: 2px;
}

.ascii-char {
  cursor: pointer;
}

.ascii-char:hover {
  background: #3d3d3d;
}

.ascii-char.selected {
  background: #094771;
  color: #fff;
}

.hex-footer {
  display: flex;
  gap: 24px;
  padding: 8px 12px;
  background: #252526;
  border-top: 1px solid #333;
  color: #888;
  font-size: 12px;
}
</style>

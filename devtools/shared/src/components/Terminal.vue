<template>
  <div class="terminal" ref="terminalRef">
    <div class="terminal-output">
      <div
        v-for="(line, idx) in lines"
        :key="idx"
        class="terminal-line"
        :class="line.type"
      >
        <span v-if="showTimestamp" class="line-time">[{{ line.time }}]</span>
        <span v-if="line.prefix" class="line-prefix">{{ line.prefix }}</span>
        <span class="line-text" v-html="formatText(line.text)"></span>
      </div>
    </div>
    <div v-if="showInput" class="terminal-input">
      <span class="input-prompt">{{ prompt }}</span>
      <input
        type="text"
        v-model="inputValue"
        @keyup.enter="sendInput"
        @keyup.up="historyUp"
        @keyup.down="historyDown"
        ref="inputRef"
        :placeholder="placeholder"
      />
    </div>
  </div>
</template>

<script setup>
import { ref, watch, nextTick, onMounted } from 'vue'

const props = defineProps({
  lines: {
    type: Array,
    default: () => []
  },
  showTimestamp: {
    type: Boolean,
    default: true
  },
  showInput: {
    type: Boolean,
    default: true
  },
  prompt: {
    type: String,
    default: '>'
  },
  placeholder: {
    type: String,
    default: ''
  },
  ansiColors: {
    type: Boolean,
    default: true
  }
})

const emit = defineEmits(['input'])

const terminalRef = ref(null)
const inputRef = ref(null)
const inputValue = ref('')
const history = ref([])
const historyIndex = ref(-1)

// ANSI 颜色映射
const ansiColors = {
  '30': 'color: #000',
  '31': 'color: #cd0000',
  '32': 'color: #00cd00',
  '33': 'color: #cdcd00',
  '34': 'color: #0000ee',
  '35': 'color: #cd00cd',
  '36': 'color: #00cdcd',
  '37': 'color: #e5e5e5',
  '90': 'color: #7f7f7f',
  '91': 'color: #ff0000',
  '92': 'color: #00ff00',
  '93': 'color: #ffff00',
  '94': 'color: #5c5cff',
  '95': 'color: #ff00ff',
  '96': 'color: #00ffff',
  '97': 'color: #ffffff'
}

function formatText(text) {
  if (!props.ansiColors || !text) return escapeHtml(text)

  // 转换 ANSI 颜色码
  let result = escapeHtml(text)
  result = result.replace(/\x1b\[(\d+)m/g, (match, code) => {
    if (code === '0') return '</span>'
    const style = ansiColors[code]
    if (style) return `<span style="${style}">`
    return ''
  })

  return result
}

function escapeHtml(text) {
  if (!text) return ''
  return text
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
}

function sendInput() {
  if (!inputValue.value.trim()) return

  emit('input', inputValue.value)
  history.value.push(inputValue.value)
  historyIndex.value = history.value.length
  inputValue.value = ''
}

function historyUp() {
  if (historyIndex.value > 0) {
    historyIndex.value--
    inputValue.value = history.value[historyIndex.value]
  }
}

function historyDown() {
  if (historyIndex.value < history.value.length - 1) {
    historyIndex.value++
    inputValue.value = history.value[historyIndex.value]
  } else {
    historyIndex.value = history.value.length
    inputValue.value = ''
  }
}

// 自动滚动到底部
watch(() => props.lines.length, () => {
  nextTick(() => {
    if (terminalRef.value) {
      const output = terminalRef.value.querySelector('.terminal-output')
      if (output) {
        output.scrollTop = output.scrollHeight
      }
    }
  })
})

// 聚焦输入框
function focus() {
  if (inputRef.value) {
    inputRef.value.focus()
  }
}

defineExpose({ focus })

onMounted(() => {
  focus()
})
</script>

<style scoped>
.terminal {
  display: flex;
  flex-direction: column;
  height: 100%;
  background: #1e1e1e;
  font-family: 'Monaco', 'Menlo', 'Ubuntu Mono', 'Consolas', monospace;
  font-size: 13px;
}

.terminal-output {
  flex: 1;
  overflow: auto;
  padding: 8px 12px;
}

.terminal-line {
  line-height: 1.6;
  white-space: pre-wrap;
  word-break: break-all;
}

.terminal-line.stdout {
  color: #d4d4d4;
}

.terminal-line.stderr {
  color: #f48771;
}

.terminal-line.info {
  color: #4fc3f7;
}

.terminal-line.success {
  color: #81c784;
}

.terminal-line.warning {
  color: #ffb74d;
}

.terminal-line.error {
  color: #e57373;
}

.line-time {
  color: #6a9955;
  margin-right: 8px;
}

.line-prefix {
  color: #569cd6;
  margin-right: 8px;
}

.line-text {
  color: inherit;
}

.terminal-input {
  display: flex;
  align-items: center;
  padding: 8px 12px;
  border-top: 1px solid #333;
  background: #252526;
}

.input-prompt {
  color: #4fc3f7;
  margin-right: 8px;
  font-weight: bold;
}

.terminal-input input {
  flex: 1;
  background: transparent;
  border: none;
  outline: none;
  color: #d4d4d4;
  font-family: inherit;
  font-size: inherit;
}

.terminal-input input::placeholder {
  color: #666;
}
</style>

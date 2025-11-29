<template>
  <div class="il-editor-view">
    <!-- 指令参考面板 -->
    <div class="instruction-panel">
      <h4>指令参考</h4>
      <mdui-collapse>
        <mdui-collapse-item>
          <div slot="header">位逻辑指令</div>
          <mdui-list dense>
            <mdui-list-item @click="insertInstruction('LD')">LD - 加载</mdui-list-item>
            <mdui-list-item @click="insertInstruction('LDN')">LDN - 加载取反</mdui-list-item>
            <mdui-list-item @click="insertInstruction('ST')">ST - 存储</mdui-list-item>
            <mdui-list-item @click="insertInstruction('STN')">STN - 存储取反</mdui-list-item>
            <mdui-list-item @click="insertInstruction('S')">S - 置位</mdui-list-item>
            <mdui-list-item @click="insertInstruction('R')">R - 复位</mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <mdui-collapse-item>
          <div slot="header">逻辑运算</div>
          <mdui-list dense>
            <mdui-list-item @click="insertInstruction('AND')">AND - 与</mdui-list-item>
            <mdui-list-item @click="insertInstruction('ANDN')">ANDN - 与取反</mdui-list-item>
            <mdui-list-item @click="insertInstruction('OR')">OR - 或</mdui-list-item>
            <mdui-list-item @click="insertInstruction('ORN')">ORN - 或取反</mdui-list-item>
            <mdui-list-item @click="insertInstruction('XOR')">XOR - 异或</mdui-list-item>
            <mdui-list-item @click="insertInstruction('NOT')">NOT - 取反</mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <mdui-collapse-item>
          <div slot="header">算术运算</div>
          <mdui-list dense>
            <mdui-list-item @click="insertInstruction('ADD')">ADD - 加法</mdui-list-item>
            <mdui-list-item @click="insertInstruction('SUB')">SUB - 减法</mdui-list-item>
            <mdui-list-item @click="insertInstruction('MUL')">MUL - 乘法</mdui-list-item>
            <mdui-list-item @click="insertInstruction('DIV')">DIV - 除法</mdui-list-item>
            <mdui-list-item @click="insertInstruction('MOD')">MOD - 取模</mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <mdui-collapse-item>
          <div slot="header">比较运算</div>
          <mdui-list dense>
            <mdui-list-item @click="insertInstruction('GT')">GT - 大于</mdui-list-item>
            <mdui-list-item @click="insertInstruction('GE')">GE - 大于等于</mdui-list-item>
            <mdui-list-item @click="insertInstruction('EQ')">EQ - 等于</mdui-list-item>
            <mdui-list-item @click="insertInstruction('NE')">NE - 不等于</mdui-list-item>
            <mdui-list-item @click="insertInstruction('LE')">LE - 小于等于</mdui-list-item>
            <mdui-list-item @click="insertInstruction('LT')">LT - 小于</mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>

        <mdui-collapse-item>
          <div slot="header">跳转指令</div>
          <mdui-list dense>
            <mdui-list-item @click="insertInstruction('JMP')">JMP - 无条件跳转</mdui-list-item>
            <mdui-list-item @click="insertInstruction('JMPC')">JMPC - 条件跳转</mdui-list-item>
            <mdui-list-item @click="insertInstruction('JMPCN')">JMPCN - 条件取反跳转</mdui-list-item>
            <mdui-list-item @click="insertInstruction('CAL')">CAL - 调用功能块</mdui-list-item>
            <mdui-list-item @click="insertInstruction('RET')">RET - 返回</mdui-list-item>
          </mdui-list>
        </mdui-collapse-item>
      </mdui-collapse>
    </div>

    <!-- 编辑器区域 -->
    <div class="editor-section">
      <!-- 变量声明 -->
      <div class="variable-section" v-if="showVariables">
        <div class="section-header">
          <span>变量声明</span>
          <mdui-button-icon @click="showVariables = false">
            <mdui-icon-expand-less></mdui-icon-expand-less>
          </mdui-button-icon>
        </div>
        <div class="variable-editor" ref="variableEditorRef"></div>
      </div>

      <div class="variable-collapse" v-else @click="showVariables = true">
        <mdui-icon-expand-more></mdui-icon-expand-more>
        <span>变量声明</span>
      </div>

      <!-- 指令编辑器 -->
      <div class="code-section">
        <div class="section-header">
          <span>指令列表</span>
        </div>
        <div class="code-editor" ref="codeEditorRef"></div>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed, onMounted, onUnmounted, watch } from 'vue'
import { useRoute } from 'vue-router'
import * as monaco from 'monaco-editor'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'

import '@mdui/icons/expand-less.js'
import '@mdui/icons/expand-more.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const pouId = computed(() => route.params.id as string)
const pou = computed(() => projectStore.getPOU(pouId.value))

const showVariables = ref(true)
const variableEditorRef = ref<HTMLElement | null>(null)
const codeEditorRef = ref<HTMLElement | null>(null)

let variableEditor: monaco.editor.IStandaloneCodeEditor | null = null
let codeEditor: monaco.editor.IStandaloneCodeEditor | null = null

function registerILLanguage() {
  monaco.languages.register({ id: 'il' })

  monaco.languages.setMonarchTokensProvider('il', {
    ignoreCase: true,
    keywords: [
      'LD', 'LDN', 'ST', 'STN', 'S', 'R',
      'AND', 'ANDN', 'OR', 'ORN', 'XOR', 'NOT',
      'ADD', 'SUB', 'MUL', 'DIV', 'MOD',
      'GT', 'GE', 'EQ', 'NE', 'LE', 'LT',
      'JMP', 'JMPC', 'JMPCN', 'CAL', 'CALC', 'CALCN', 'RET', 'RETC', 'RETCN'
    ],
    tokenizer: {
      root: [
        [/[a-zA-Z_]\w*:/, 'tag'],
        [/[a-zA-Z_]\w*/, {
          cases: {
            '@keywords': 'keyword',
            '@default': 'identifier'
          }
        }],
        [/\(\*/, 'comment', '@comment'],
        [/\/\/.*$/, 'comment'],
        [/\d+/, 'number'],
        [/%[IQM][XBWD]?\d+(\.\d+)?/, 'variable.predefined']
      ],
      comment: [
        [/[^(*]+/, 'comment'],
        [/\*\)/, 'comment', '@pop'],
        [/[(*]/, 'comment']
      ]
    }
  })
}

function createEditors() {
  registerILLanguage()

  if (variableEditorRef.value) {
    variableEditor = monaco.editor.create(variableEditorRef.value, {
      value: 'VAR\n    (* 声明变量 *)\nEND_VAR',
      language: 'il',
      theme: 'vs',
      minimap: { enabled: false },
      lineNumbers: 'on',
      scrollBeyondLastLine: false,
      automaticLayout: true,
      fontSize: 13
    })
  }

  if (codeEditorRef.value) {
    codeEditor = monaco.editor.create(codeEditorRef.value, {
      value: pou.value?.body as string || '(* 指令列表 *)\n\n',
      language: 'il',
      theme: 'vs',
      minimap: { enabled: false },
      lineNumbers: 'on',
      scrollBeyondLastLine: false,
      automaticLayout: true,
      fontSize: 13
    })

    codeEditor.onDidChangeModelContent(() => {
      editorStore.markModified(`${pouId.value}-pou`, true)
      if (pou.value) {
        projectStore.updatePOU(pouId.value, { body: codeEditor!.getValue() })
      }
    })

    codeEditor.onDidChangeCursorPosition((e) => {
      editorStore.updateCursorPosition(e.position.lineNumber, e.position.column)
    })
  }
}

function insertInstruction(instruction: string) {
  if (codeEditor) {
    const position = codeEditor.getPosition()
    if (position) {
      codeEditor.executeEdits('', [{
        range: new monaco.Range(position.lineNumber, position.column, position.lineNumber, position.column),
        text: instruction + ' '
      }])
      codeEditor.focus()
    }
  }
}

onMounted(() => {
  createEditors()
})

onUnmounted(() => {
  variableEditor?.dispose()
  codeEditor?.dispose()
})

watch(pouId, () => {
  if (codeEditor && pou.value) {
    codeEditor.setValue(pou.value.body as string)
  }
})
</script>

<style scoped>
.il-editor-view {
  height: 100%;
  display: flex;
  background-color: rgb(var(--mdui-color-surface));
}

.instruction-panel {
  width: 200px;
  border-right: 1px solid rgb(var(--mdui-color-outline-variant));
  overflow: auto;
  padding: 8px;
}

.instruction-panel h4 {
  margin: 0 0 8px 0;
  font-size: 14px;
  font-weight: 500;
}

.editor-section {
  flex: 1;
  display: flex;
  flex-direction: column;
}

.variable-section {
  height: 150px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  display: flex;
  flex-direction: column;
}

.variable-collapse {
  display: flex;
  align-items: center;
  gap: 8px;
  padding: 4px 16px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  cursor: pointer;
  font-size: 13px;
}

.section-header {
  display: flex;
  align-items: center;
  justify-content: space-between;
  padding: 4px 16px;
  background-color: rgb(var(--mdui-color-surface-container));
  font-size: 13px;
  font-weight: 500;
}

.variable-editor {
  flex: 1;
}

.code-section {
  flex: 1;
  display: flex;
  flex-direction: column;
  min-height: 0;
}

.code-editor {
  flex: 1;
}

mdui-list-item {
  font-size: 12px;
  cursor: pointer;
}

mdui-list-item:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
}
</style>

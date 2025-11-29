<template>
  <div class="st-editor-view">
    <!-- 变量声明区 -->
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

    <!-- 代码编辑区 -->
    <div class="code-section">
      <div class="section-header">
        <span>程序代码</span>
        <div class="header-actions">
          <mdui-button-icon @click="formatCode">
            <mdui-icon-format-align-left></mdui-icon-format-align-left>
          </mdui-button-icon>
          <mdui-button-icon @click="toggleFullscreen">
            <mdui-icon-fullscreen></mdui-icon-fullscreen>
          </mdui-button-icon>
        </div>
      </div>
      <div class="code-editor" ref="codeEditorRef"></div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, onMounted, onUnmounted, watch, computed } from 'vue'
import { useRoute } from 'vue-router'
import * as monaco from 'monaco-editor'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { POU } from '@/types/iec61131'

import '@mdui/icons/expand-less.js'
import '@mdui/icons/expand-more.js'
import '@mdui/icons/format-align-left.js'
import '@mdui/icons/fullscreen.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const showVariables = ref(true)
const variableEditorRef = ref<HTMLElement | null>(null)
const codeEditorRef = ref<HTMLElement | null>(null)

let variableEditor: monaco.editor.IStandaloneCodeEditor | null = null
let codeEditor: monaco.editor.IStandaloneCodeEditor | null = null

const pouId = computed(() => route.params.id as string)
const pou = computed(() => projectStore.getPOU(pouId.value))

// 注册 ST 语言
function registerSTLanguage() {
  monaco.languages.register({ id: 'st' })

  monaco.languages.setMonarchTokensProvider('st', {
    ignoreCase: true,
    keywords: [
      'PROGRAM', 'END_PROGRAM', 'FUNCTION', 'END_FUNCTION', 'FUNCTION_BLOCK', 'END_FUNCTION_BLOCK',
      'VAR', 'VAR_INPUT', 'VAR_OUTPUT', 'VAR_IN_OUT', 'VAR_GLOBAL', 'VAR_EXTERNAL', 'VAR_TEMP',
      'END_VAR', 'CONSTANT', 'RETAIN', 'PERSISTENT',
      'IF', 'THEN', 'ELSIF', 'ELSE', 'END_IF',
      'CASE', 'OF', 'END_CASE',
      'FOR', 'TO', 'BY', 'DO', 'END_FOR',
      'WHILE', 'END_WHILE',
      'REPEAT', 'UNTIL', 'END_REPEAT',
      'EXIT', 'RETURN', 'CONTINUE',
      'AND', 'OR', 'XOR', 'NOT', 'MOD',
      'TRUE', 'FALSE',
      'ARRAY', 'STRUCT', 'END_STRUCT', 'TYPE', 'END_TYPE'
    ],
    typeKeywords: [
      'BOOL', 'BYTE', 'WORD', 'DWORD', 'LWORD',
      'SINT', 'INT', 'DINT', 'LINT',
      'USINT', 'UINT', 'UDINT', 'ULINT',
      'REAL', 'LREAL',
      'TIME', 'DATE', 'TIME_OF_DAY', 'TOD', 'DATE_AND_TIME', 'DT',
      'STRING', 'WSTRING'
    ],
    operators: [
      ':=', '=', '<>', '<', '<=', '>', '>=', '+', '-', '*', '/', '**', '&'
    ],
    symbols: /[=><!~?:&|+\-*\/\^%]+/,
    tokenizer: {
      root: [
        [/[a-zA-Z_]\w*/, {
          cases: {
            '@keywords': 'keyword',
            '@typeKeywords': 'type',
            '@default': 'identifier'
          }
        }],
        { include: '@whitespace' },
        [/[{}()\[\]]/, '@brackets'],
        [/@symbols/, {
          cases: {
            '@operators': 'operator',
            '@default': ''
          }
        }],
        [/\d*\.\d+([eE][\-+]?\d+)?/, 'number.float'],
        [/\d+/, 'number'],
        [/[;,.]/, 'delimiter'],
        [/'([^'\\]|\\.)*$/, 'string.invalid'],
        [/'/, 'string', '@string'],
        [/\(\*/, 'comment', '@comment'],
        [/\/\/.*$/, 'comment']
      ],
      string: [
        [/[^\\']+/, 'string'],
        [/\\./, 'string.escape'],
        [/'/, 'string', '@pop']
      ],
      comment: [
        [/[^(*]+/, 'comment'],
        [/\*\)/, 'comment', '@pop'],
        [/[(*]/, 'comment']
      ],
      whitespace: [
        [/[ \t\r\n]+/, 'white']
      ]
    }
  })

  // 自动完成
  monaco.languages.registerCompletionItemProvider('st', {
    provideCompletionItems: (model, position) => {
      const suggestions: monaco.languages.CompletionItem[] = []
      const range = {
        startLineNumber: position.lineNumber,
        endLineNumber: position.lineNumber,
        startColumn: position.column,
        endColumn: position.column
      }

      // 关键字
      const keywords = [
        'PROGRAM', 'END_PROGRAM', 'FUNCTION', 'END_FUNCTION', 'FUNCTION_BLOCK', 'END_FUNCTION_BLOCK',
        'VAR', 'VAR_INPUT', 'VAR_OUTPUT', 'VAR_IN_OUT', 'END_VAR',
        'IF', 'THEN', 'ELSIF', 'ELSE', 'END_IF',
        'CASE', 'OF', 'END_CASE',
        'FOR', 'TO', 'BY', 'DO', 'END_FOR',
        'WHILE', 'END_WHILE',
        'REPEAT', 'UNTIL', 'END_REPEAT'
      ]

      keywords.forEach(kw => {
        suggestions.push({
          label: kw,
          kind: monaco.languages.CompletionItemKind.Keyword,
          insertText: kw,
          range
        })
      })

      // 数据类型
      const types = ['BOOL', 'INT', 'DINT', 'REAL', 'STRING', 'TIME']
      types.forEach(t => {
        suggestions.push({
          label: t,
          kind: monaco.languages.CompletionItemKind.TypeParameter,
          insertText: t,
          range
        })
      })

      // 代码片段
      suggestions.push({
        label: 'if-then',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: 'IF ${1:condition} THEN\n\t${2:// statements}\nEND_IF;',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'IF-THEN 语句',
        range
      })

      suggestions.push({
        label: 'for-loop',
        kind: monaco.languages.CompletionItemKind.Snippet,
        insertText: 'FOR ${1:i} := ${2:0} TO ${3:10} DO\n\t${4:// statements}\nEND_FOR;',
        insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
        documentation: 'FOR 循环',
        range
      })

      return { suggestions }
    }
  })
}

function createEditors() {
  registerSTLanguage()

  if (variableEditorRef.value) {
    variableEditor = monaco.editor.create(variableEditorRef.value, {
      value: getVariableDeclaration(),
      language: 'st',
      theme: 'vs',
      minimap: { enabled: false },
      lineNumbers: 'on',
      scrollBeyondLastLine: false,
      automaticLayout: true,
      fontSize: 13,
      tabSize: 4
    })

    variableEditor.onDidChangeModelContent(() => {
      editorStore.markModified(`${pouId.value}-pou`, true)
    })
  }

  if (codeEditorRef.value) {
    codeEditor = monaco.editor.create(codeEditorRef.value, {
      value: pou.value?.body as string || '',
      language: 'st',
      theme: 'vs',
      minimap: { enabled: true },
      lineNumbers: 'on',
      scrollBeyondLastLine: false,
      automaticLayout: true,
      fontSize: 13,
      tabSize: 4
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

function getVariableDeclaration(): string {
  if (!pou.value) return ''

  let decl = ''
  const vars = pou.value.variables

  const varInput = vars.filter(v => v.variableClass === 'VAR_INPUT')
  const varOutput = vars.filter(v => v.variableClass === 'VAR_OUTPUT')
  const varLocal = vars.filter(v => v.variableClass === 'VAR')

  if (varInput.length > 0) {
    decl += 'VAR_INPUT\n'
    varInput.forEach(v => {
      decl += `    ${v.name} : ${v.dataType};${v.comment ? ' (* ' + v.comment + ' *)' : ''}\n`
    })
    decl += 'END_VAR\n\n'
  }

  if (varOutput.length > 0) {
    decl += 'VAR_OUTPUT\n'
    varOutput.forEach(v => {
      decl += `    ${v.name} : ${v.dataType};${v.comment ? ' (* ' + v.comment + ' *)' : ''}\n`
    })
    decl += 'END_VAR\n\n'
  }

  if (varLocal.length > 0) {
    decl += 'VAR\n'
    varLocal.forEach(v => {
      decl += `    ${v.name} : ${v.dataType};${v.comment ? ' (* ' + v.comment + ' *)' : ''}\n`
    })
    decl += 'END_VAR\n'
  }

  return decl || 'VAR\n    (* 声明变量 *)\nEND_VAR'
}

function formatCode() {
  codeEditor?.getAction('editor.action.formatDocument')?.run()
}

function toggleFullscreen() {
  // TODO: 实现全屏切换
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
  if (variableEditor) {
    variableEditor.setValue(getVariableDeclaration())
  }
})
</script>

<style scoped>
.st-editor-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.variable-section {
  height: 200px;
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

.variable-collapse:hover {
  background-color: rgba(var(--mdui-color-primary), 0.08);
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

.header-actions {
  display: flex;
  gap: 4px;
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
</style>

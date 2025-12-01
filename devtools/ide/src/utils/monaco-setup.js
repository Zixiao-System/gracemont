import * as monaco from 'monaco-editor'

/**
 * 配置 IEC 61131-3 Structured Text 语言支持
 */
export function setupMonacoST() {
  // 注册 ST 语言
  monaco.languages.register({ id: 'st' })

  // 定义 ST 语言的词法规则
  monaco.languages.setMonarchTokensProvider('st', {
    keywords: [
      // 程序组织单元
      'PROGRAM', 'END_PROGRAM',
      'FUNCTION', 'END_FUNCTION',
      'FUNCTION_BLOCK', 'END_FUNCTION_BLOCK',
      'VAR', 'END_VAR', 'VAR_INPUT', 'VAR_OUTPUT', 'VAR_IN_OUT',
      'VAR_GLOBAL', 'VAR_EXTERNAL', 'VAR_TEMP',
      'CONSTANT',

      // 控制流
      'IF', 'THEN', 'ELSE', 'ELSIF', 'END_IF',
      'CASE', 'OF', 'END_CASE',
      'FOR', 'TO', 'BY', 'DO', 'END_FOR',
      'WHILE', 'END_WHILE',
      'REPEAT', 'UNTIL', 'END_REPEAT',
      'EXIT', 'RETURN',

      // 运算符
      'AND', 'OR', 'XOR', 'NOT',
      'MOD', 'DIV',

      // 其他关键字
      'AT', 'RETAIN', 'NON_RETAIN',
      'TASK', 'WITH', 'PRIORITY',
      'INTERVAL', 'SINGLE'
    ],

    typeKeywords: [
      // 基本数据类型
      'BOOL', 'SINT', 'INT', 'DINT', 'LINT',
      'USINT', 'UINT', 'UDINT', 'ULINT',
      'REAL', 'LREAL',
      'TIME', 'DATE', 'TIME_OF_DAY', 'TOD', 'DATE_AND_TIME', 'DT',
      'STRING', 'WSTRING',
      'BYTE', 'WORD', 'DWORD', 'LWORD',

      // 复合数据类型
      'ARRAY', 'STRUCT', 'END_STRUCT',
      'TYPE', 'END_TYPE',
      'POINTER', 'REFERENCE'
    ],

    constants: [
      'TRUE', 'FALSE',
      'NULL'
    ],

    operators: [
      ':=', '=', '<>', '>', '<', '>=', '<=',
      '+', '-', '*', '/', '**',
      '&'
    ],

    symbols: /[=><!~?:&|+\-*\/\^%]+/,
    escapes: /\\(?:[abfnrtv\\"']|x[0-9A-Fa-f]{1,4}|u[0-9A-Fa-f]{4}|U[0-9A-Fa-f]{8})/,

    tokenizer: {
      root: [
        // 标识符和关键字
        [/[a-zA-Z_]\w*/, {
          cases: {
            '@keywords': 'keyword',
            '@typeKeywords': 'type',
            '@constants': 'constant',
            '@default': 'identifier'
          }
        }],

        // 空白字符
        { include: '@whitespace' },

        // 数字
        [/\d+\.\d+([eE][\-+]?\d+)?/, 'number.float'],
        [/\d+/, 'number'],
        [/2#[01]+/, 'number.binary'],
        [/8#[0-7]+/, 'number.octal'],
        [/16#[0-9A-Fa-f]+/, 'number.hex'],

        // 字符串
        [/"([^"\\]|\\.)*$/, 'string.invalid'],
        [/'([^'\\]|\\.)*$/, 'string.invalid'],
        [/"/, 'string', '@string_double'],
        [/'/, 'string', '@string_single'],

        // 分隔符和运算符
        [/[;,.]/, 'delimiter'],
        [/[()]/, '@brackets'],
        [/[[\]]/, '@brackets'],
        [/@symbols/, {
          cases: {
            '@operators': 'operator',
            '@default': ''
          }
        }],
      ],

      whitespace: [
        [/[ \t\r\n]+/, ''],
        [/\(\*/, 'comment', '@comment'],
        [/\/\/.*$/, 'comment'],
      ],

      comment: [
        [/[^\*\)]+/, 'comment'],
        [/\*\)/, 'comment', '@pop'],
        [/[\*\)]/, 'comment']
      ],

      string_double: [
        [/[^\\"]+/, 'string'],
        [/@escapes/, 'string.escape'],
        [/\\./, 'string.escape.invalid'],
        [/"/, 'string', '@pop']
      ],

      string_single: [
        [/[^\\']+/, 'string'],
        [/@escapes/, 'string.escape'],
        [/\\./, 'string.escape.invalid'],
        [/'/, 'string', '@pop']
      ],
    }
  })

  // 定义语言配置
  monaco.languages.setLanguageConfiguration('st', {
    comments: {
      lineComment: '//',
      blockComment: ['(*', '*)']
    },
    brackets: [
      ['{', '}'],
      ['[', ']'],
      ['(', ')']
    ],
    autoClosingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      { open: '"', close: '"' },
      { open: "'", close: "'" },
    ],
    surroundingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      { open: '"', close: '"' },
      { open: "'", close: "'" },
    ],
    folding: {
      markers: {
        start: new RegExp('^\\s*(?:PROGRAM|FUNCTION|FUNCTION_BLOCK|IF|FOR|WHILE|REPEAT|CASE)'),
        end: new RegExp('^\\s*(?:END_PROGRAM|END_FUNCTION|END_FUNCTION_BLOCK|END_IF|END_FOR|END_WHILE|END_REPEAT|END_CASE)')
      }
    }
  })

  // 定义代码补全
  monaco.languages.registerCompletionItemProvider('st', {
    provideCompletionItems: (model, position) => {
      const suggestions = [
        // 程序结构
        {
          label: 'PROGRAM',
          kind: monaco.languages.CompletionItemKind.Snippet,
          insertText: 'PROGRAM ${1:ProgramName}\n  VAR\n    ${2}\n  END_VAR\n\n  ${3}\nEND_PROGRAM',
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: 'Create a new program'
        },
        {
          label: 'IF',
          kind: monaco.languages.CompletionItemKind.Snippet,
          insertText: 'IF ${1:condition} THEN\n  ${2}\nEND_IF',
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: 'IF statement'
        },
        {
          label: 'FOR',
          kind: monaco.languages.CompletionItemKind.Snippet,
          insertText: 'FOR ${1:i} := ${2:0} TO ${3:10} DO\n  ${4}\nEND_FOR',
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: 'FOR loop'
        },
        {
          label: 'WHILE',
          kind: monaco.languages.CompletionItemKind.Snippet,
          insertText: 'WHILE ${1:condition} DO\n  ${2}\nEND_WHILE',
          insertTextRules: monaco.languages.CompletionItemInsertTextRule.InsertAsSnippet,
          documentation: 'WHILE loop'
        }
      ]

      return { suggestions: suggestions }
    }
  })
}

/**
 * 创建 Monaco Editor 实例
 */
export function createEditor(container, options = {}) {
  const editor = monaco.editor.create(container, {
    theme: 'vs-dark',
    fontSize: 14,
    fontFamily: "'JetBrains Mono', 'Fira Code', 'Cascadia Code', 'Consolas', monospace",
    lineNumbers: 'on',
    minimap: { enabled: true },
    scrollBeyondLastLine: false,
    automaticLayout: true,
    tabSize: 2,
    insertSpaces: true,
    wordWrap: 'off',
    ...options
  })

  return editor
}

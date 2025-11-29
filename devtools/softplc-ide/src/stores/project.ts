import { defineStore } from 'pinia'
import { ref, computed } from 'vue'
import type { Project, POU, Variable, CustomDataType, ResourceConfig, CompileResult, CompileError, CompileWarning } from '@/types/iec61131'
import { POUType, LanguageType, DataType, VariableClass } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

export const useProjectStore = defineStore('project', () => {
  // 状态
  const currentProject = ref<Project | null>(null)
  const isModified = ref(false)
  const compileResult = ref<CompileResult | null>(null)
  const isCompiling = ref(false)

  // 计算属性
  const pous = computed(() => currentProject.value?.pous || [])
  const programs = computed(() => pous.value.filter(p => p.type === POUType.PROGRAM))
  const functions = computed(() => pous.value.filter(p => p.type === POUType.FUNCTION))
  const functionBlocks = computed(() => pous.value.filter(p => p.type === POUType.FUNCTION_BLOCK))
  const globalVariables = computed(() => {
    if (!currentProject.value) return []
    return currentProject.value.resources.flatMap(r => r.globalVariables)
  })
  const dataTypes = computed(() => currentProject.value?.dataTypes || [])
  const errors = computed(() => compileResult.value?.errors || [])
  const warnings = computed(() => compileResult.value?.warnings || [])

  // 创建新项目
  function createProject(name: string, description?: string): Project {
    const project: Project = {
      config: {
        id: uuidv4(),
        name,
        description,
        version: '1.0.0',
        created: new Date(),
        modified: new Date(),
        targetPlatform: 'ARM64-RTOS',
        scanCycle: 10,
        watchdogTime: 100
      },
      pous: [
        // 创建默认主程序
        {
          id: uuidv4(),
          name: 'Main',
          type: POUType.PROGRAM,
          language: LanguageType.ST,
          variables: [],
          body: '(* 主程序 *)\n\n',
          comment: '主程序'
        }
      ],
      resources: [
        {
          id: uuidv4(),
          name: 'Resource1',
          tasks: [
            {
              id: uuidv4(),
              name: 'MainTask',
              priority: 1,
              interval: 10,
              programs: []
            }
          ],
          globalVariables: []
        }
      ],
      dataTypes: [],
      libraries: []
    }

    // 将主程序关联到任务
    project.resources[0].tasks[0].programs.push(project.pous[0].id)

    currentProject.value = project
    isModified.value = false
    return project
  }

  // 打开项目
  async function openProject(): Promise<boolean> {
    // TODO: 实现文件对话框和项目加载
    console.log('Opening project...')
    return false
  }

  // 保存项目
  async function saveProject(): Promise<boolean> {
    if (!currentProject.value) return false

    currentProject.value.config.modified = new Date()
    // TODO: 实现文件保存
    isModified.value = false
    return true
  }

  // 添加 POU
  function addPOU(name: string, type: POUType, language: LanguageType): POU {
    const pou: POU = {
      id: uuidv4(),
      name,
      type,
      language,
      variables: [],
      body: getDefaultBody(language, type),
      comment: ''
    }

    if (type === POUType.FUNCTION) {
      pou.returnType = DataType.BOOL
    }

    currentProject.value?.pous.push(pou)
    isModified.value = true
    return pou
  }

  // 删除 POU
  function deletePOU(id: string): boolean {
    if (!currentProject.value) return false
    const index = currentProject.value.pous.findIndex(p => p.id === id)
    if (index >= 0) {
      currentProject.value.pous.splice(index, 1)
      isModified.value = true
      return true
    }
    return false
  }

  // 获取 POU
  function getPOU(id: string): POU | undefined {
    return currentProject.value?.pous.find(p => p.id === id)
  }

  // 更新 POU
  function updatePOU(id: string, updates: Partial<POU>): boolean {
    const pou = getPOU(id)
    if (pou) {
      Object.assign(pou, updates)
      isModified.value = true
      return true
    }
    return false
  }

  // 添加全局变量
  function addGlobalVariable(variable: Omit<Variable, 'id'>): Variable {
    const newVar: Variable = {
      id: uuidv4(),
      ...variable
    }

    if (currentProject.value && currentProject.value.resources.length > 0) {
      currentProject.value.resources[0].globalVariables.push(newVar)
      isModified.value = true
    }
    return newVar
  }

  // 添加自定义数据类型
  function addDataType(dataType: Omit<CustomDataType, 'id'>): CustomDataType {
    const newType: CustomDataType = {
      id: uuidv4(),
      ...dataType
    }

    currentProject.value?.dataTypes.push(newType)
    isModified.value = true
    return newType
  }

  // 编译项目
  async function compile(): Promise<CompileResult> {
    if (!currentProject.value) {
      return {
        success: false,
        errors: [{ file: '', line: 0, column: 0, message: '无打开的项目', code: 'E001' }],
        warnings: []
      }
    }

    isCompiling.value = true
    compileResult.value = null

    try {
      // TODO: 实现实际编译逻辑
      // 这里模拟编译过程
      await new Promise(resolve => setTimeout(resolve, 1000))

      const result: CompileResult = {
        success: true,
        errors: [],
        warnings: []
      }

      // 简单语法检查
      for (const pou of currentProject.value.pous) {
        if (pou.language === LanguageType.ST || pou.language === LanguageType.IL) {
          const body = pou.body as string
          // 检查基本语法错误
          if (body.includes(';;')) {
            result.warnings.push({
              file: pou.name,
              line: 1,
              column: 1,
              message: '发现连续的分号',
              code: 'W001'
            })
          }
        }
      }

      compileResult.value = result
      return result
    } finally {
      isCompiling.value = false
    }
  }

  // 获取默认代码模板
  function getDefaultBody(language: LanguageType, type: POUType): string | object {
    switch (language) {
      case LanguageType.ST:
        return getSTTemplate(type)
      case LanguageType.IL:
        return getILTemplate(type)
      case LanguageType.LD:
        return { rungs: [] }
      case LanguageType.FBD:
        return { elements: [], connections: [] }
      case LanguageType.SFC:
        return { steps: [], transitions: [] }
      default:
        return ''
    }
  }

  function getSTTemplate(type: POUType): string {
    switch (type) {
      case POUType.PROGRAM:
        return `(* 程序名称: *)
(* 描述: *)

`
      case POUType.FUNCTION:
        return `(* 函数名称: *)
(* 描述: *)
(* 返回值: *)

`
      case POUType.FUNCTION_BLOCK:
        return `(* 功能块名称: *)
(* 描述: *)

`
      default:
        return ''
    }
  }

  function getILTemplate(type: POUType): string {
    switch (type) {
      case POUType.PROGRAM:
        return `(* 程序名称: *)
(* 指令列表 *)

`
      default:
        return `(* 指令列表 *)

`
    }
  }

  return {
    // 状态
    currentProject,
    isModified,
    compileResult,
    isCompiling,
    // 计算属性
    pous,
    programs,
    functions,
    functionBlocks,
    globalVariables,
    dataTypes,
    errors,
    warnings,
    // 方法
    createProject,
    openProject,
    saveProject,
    addPOU,
    deletePOU,
    getPOU,
    updatePOU,
    addGlobalVariable,
    addDataType,
    compile
  }
})

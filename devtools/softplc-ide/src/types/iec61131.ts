// IEC 61131-3 数据类型定义

// 基本数据类型
export type IEC_BOOL = boolean
export type IEC_BYTE = number    // 0-255
export type IEC_WORD = number    // 0-65535
export type IEC_DWORD = number   // 0-4294967295
export type IEC_LWORD = bigint   // 64-bit
export type IEC_SINT = number    // -128 to 127
export type IEC_INT = number     // -32768 to 32767
export type IEC_DINT = number    // -2147483648 to 2147483647
export type IEC_LINT = bigint    // 64-bit signed
export type IEC_USINT = number   // 0-255
export type IEC_UINT = number    // 0-65535
export type IEC_UDINT = number   // 0-4294967295
export type IEC_ULINT = bigint   // 64-bit unsigned
export type IEC_REAL = number    // 32-bit float
export type IEC_LREAL = number   // 64-bit float
export type IEC_TIME = number    // Time duration in ms
export type IEC_DATE = Date
export type IEC_TOD = Date       // Time of day
export type IEC_DT = Date        // Date and time
export type IEC_STRING = string
export type IEC_WSTRING = string

// 数据类型枚举
export enum DataType {
  BOOL = 'BOOL',
  BYTE = 'BYTE',
  WORD = 'WORD',
  DWORD = 'DWORD',
  LWORD = 'LWORD',
  SINT = 'SINT',
  INT = 'INT',
  DINT = 'DINT',
  LINT = 'LINT',
  USINT = 'USINT',
  UINT = 'UINT',
  UDINT = 'UDINT',
  ULINT = 'ULINT',
  REAL = 'REAL',
  LREAL = 'LREAL',
  TIME = 'TIME',
  DATE = 'DATE',
  TOD = 'TIME_OF_DAY',
  DT = 'DATE_AND_TIME',
  STRING = 'STRING',
  WSTRING = 'WSTRING'
}

// 变量类型
export enum VariableClass {
  VAR = 'VAR',
  VAR_INPUT = 'VAR_INPUT',
  VAR_OUTPUT = 'VAR_OUTPUT',
  VAR_IN_OUT = 'VAR_IN_OUT',
  VAR_GLOBAL = 'VAR_GLOBAL',
  VAR_EXTERNAL = 'VAR_EXTERNAL',
  VAR_TEMP = 'VAR_TEMP',
  VAR_RETAIN = 'VAR_RETAIN',
  VAR_PERSISTENT = 'VAR_PERSISTENT'
}

// 变量定义
export interface Variable {
  id: string
  name: string
  dataType: DataType
  variableClass: VariableClass
  initialValue?: string
  address?: string          // 物理地址，如 %IX0.0, %QW10
  comment?: string
  arrayDimensions?: number[]
  structType?: string       // 结构体类型名
  retain?: boolean
  persistent?: boolean
}

// POU (Program Organization Unit) 类型
export enum POUType {
  PROGRAM = 'PROGRAM',
  FUNCTION = 'FUNCTION',
  FUNCTION_BLOCK = 'FUNCTION_BLOCK'
}

// 编程语言类型
export enum LanguageType {
  ST = 'ST',      // Structured Text 结构化文本
  LD = 'LD',      // Ladder Diagram 梯形图
  FBD = 'FBD',    // Function Block Diagram 功能块图
  IL = 'IL',      // Instruction List 指令表
  SFC = 'SFC'     // Sequential Function Chart 顺序功能图
}

// POU 定义
export interface POU {
  id: string
  name: string
  type: POUType
  language: LanguageType
  returnType?: DataType     // 仅 FUNCTION 类型需要
  variables: Variable[]
  body: string | object     // ST/IL 为字符串，LD/FBD/SFC 为结构化对象
  comment?: string
}

// 梯形图元素类型
export enum LDElementType {
  CONTACT_NO = 'CONTACT_NO',     // 常开触点
  CONTACT_NC = 'CONTACT_NC',     // 常闭触点
  CONTACT_P = 'CONTACT_P',       // 上升沿触点
  CONTACT_N = 'CONTACT_N',       // 下降沿触点
  COIL = 'COIL',                 // 线圈
  COIL_SET = 'COIL_SET',         // 置位线圈
  COIL_RESET = 'COIL_RESET',     // 复位线圈
  COIL_P = 'COIL_P',             // 上升沿线圈
  COIL_N = 'COIL_N',             // 下降沿线圈
  FUNCTION_BLOCK = 'FUNCTION_BLOCK',
  FUNCTION = 'FUNCTION'
}

// 梯形图元素
export interface LDElement {
  id: string
  type: LDElementType
  variable?: string
  functionBlock?: string
  x: number
  y: number
  inputs?: string[]
  outputs?: string[]
}

// 梯形图行
export interface LDRung {
  id: string
  elements: LDElement[]
  comment?: string
}

// 梯形图
export interface LDDiagram {
  rungs: LDRung[]
}

// FBD 元素类型
export enum FBDElementType {
  INPUT = 'INPUT',
  OUTPUT = 'OUTPUT',
  FUNCTION = 'FUNCTION',
  FUNCTION_BLOCK = 'FUNCTION_BLOCK',
  JUMP = 'JUMP',
  RETURN = 'RETURN',
  LABEL = 'LABEL'
}

// FBD 连接点
export interface FBDPin {
  id: string
  name: string
  dataType: DataType
  connected?: string      // 连接到的 pin id
}

// FBD 元素
export interface FBDElement {
  id: string
  type: FBDElementType
  name: string
  x: number
  y: number
  width: number
  height: number
  inputs: FBDPin[]
  outputs: FBDPin[]
  properties?: Record<string, any>
}

// FBD 连接
export interface FBDConnection {
  id: string
  sourceElement: string
  sourcePin: string
  targetElement: string
  targetPin: string
  points?: { x: number; y: number }[]
}

// FBD 图
export interface FBDDiagram {
  elements: FBDElement[]
  connections: FBDConnection[]
}

// SFC 步骤类型
export enum SFCStepType {
  INITIAL = 'INITIAL',
  NORMAL = 'NORMAL',
  FINAL = 'FINAL'
}

// SFC 动作类型
export enum SFCActionQualifier {
  N = 'N',    // Non-stored
  R = 'R',    // Reset
  S = 'S',    // Set
  L = 'L',    // Time limited
  D = 'D',    // Time delayed
  P = 'P',    // Pulse
  SD = 'SD',  // Stored and time delayed
  DS = 'DS',  // Delayed and stored
  SL = 'SL',  // Stored and time limited
  P0 = 'P0',  // Pulse (falling edge)
  P1 = 'P1'   // Pulse (rising edge)
}

// SFC 动作
export interface SFCAction {
  id: string
  name: string
  qualifier: SFCActionQualifier
  duration?: number       // 用于时间相关限定符
  body: string           // 动作代码 (ST)
}

// SFC 步骤
export interface SFCStep {
  id: string
  name: string
  type: SFCStepType
  x: number
  y: number
  actions: SFCAction[]
  entryAction?: string
  exitAction?: string
}

// SFC 转换
export interface SFCTransition {
  id: string
  name: string
  condition: string       // 转换条件 (ST 表达式)
  sourceSteps: string[]
  targetSteps: string[]
  x: number
  y: number
}

// SFC 图
export interface SFCDiagram {
  steps: SFCStep[]
  transitions: SFCTransition[]
}

// 项目配置
export interface ProjectConfig {
  id: string
  name: string
  description?: string
  version: string
  author?: string
  created: Date
  modified: Date
  targetPlatform: string
  scanCycle: number        // 扫描周期 (ms)
  watchdogTime: number     // 看门狗时间 (ms)
}

// 任务配置
export interface TaskConfig {
  id: string
  name: string
  priority: number
  interval?: number        // 周期任务间隔 (ms)
  event?: string           // 事件任务触发条件
  programs: string[]       // 关联的程序 POU id
}

// 资源配置
export interface ResourceConfig {
  id: string
  name: string
  tasks: TaskConfig[]
  globalVariables: Variable[]
}

// 项目结构
export interface Project {
  config: ProjectConfig
  pous: POU[]
  resources: ResourceConfig[]
  dataTypes: CustomDataType[]
  libraries: LibraryReference[]
}

// 自定义数据类型
export interface CustomDataType {
  id: string
  name: string
  baseType: 'STRUCT' | 'ENUM' | 'ARRAY' | 'ALIAS'
  members?: Variable[]           // STRUCT
  enumValues?: string[]          // ENUM
  arrayType?: DataType           // ARRAY
  arrayDimensions?: number[]     // ARRAY
  aliasType?: DataType           // ALIAS
}

// 库引用
export interface LibraryReference {
  id: string
  name: string
  version: string
  path?: string
}

// 编译结果
export interface CompileResult {
  success: boolean
  errors: CompileError[]
  warnings: CompileWarning[]
  output?: Uint8Array           // 编译后的二进制
}

export interface CompileError {
  file: string
  line: number
  column: number
  message: string
  code: string
}

export interface CompileWarning {
  file: string
  line: number
  column: number
  message: string
  code: string
}

// PLC 连接配置
export interface PLCConnection {
  type: 'tcp' | 'serial' | 'usb'
  host?: string
  port?: number
  serialPort?: string
  baudRate?: number
}

// 监控数据
export interface MonitorData {
  variableId: string
  value: any
  timestamp: number
  quality: 'good' | 'bad' | 'uncertain'
}

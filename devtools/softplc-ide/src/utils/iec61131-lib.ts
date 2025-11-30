// IEC 61131-3 标准函数库

import { DataType } from '@/types/iec61131'

// 标准功能块定义
export interface StandardFunctionBlock {
  name: string
  description: string
  inputs: { name: string; type: DataType; description: string }[]
  outputs: { name: string; type: DataType; description: string }[]
  category: 'timer' | 'counter' | 'bistable' | 'edge' | 'comparison' | 'arithmetic' | 'bitwise' | 'selection' | 'conversion'
}

export const standardFunctionBlocks: StandardFunctionBlock[] = [
  // 定时器
  {
    name: 'TON',
    description: '接通延时定时器',
    inputs: [
      { name: 'IN', type: DataType.BOOL, description: '定时器输入' },
      { name: 'PT', type: DataType.TIME, description: '预设时间' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '定时器输出' },
      { name: 'ET', type: DataType.TIME, description: '已过时间' }
    ],
    category: 'timer'
  },
  {
    name: 'TOF',
    description: '断开延时定时器',
    inputs: [
      { name: 'IN', type: DataType.BOOL, description: '定时器输入' },
      { name: 'PT', type: DataType.TIME, description: '预设时间' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '定时器输出' },
      { name: 'ET', type: DataType.TIME, description: '已过时间' }
    ],
    category: 'timer'
  },
  {
    name: 'TP',
    description: '脉冲定时器',
    inputs: [
      { name: 'IN', type: DataType.BOOL, description: '定时器输入' },
      { name: 'PT', type: DataType.TIME, description: '脉冲宽度' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '定时器输出' },
      { name: 'ET', type: DataType.TIME, description: '已过时间' }
    ],
    category: 'timer'
  },

  // 计数器
  {
    name: 'CTU',
    description: '递增计数器',
    inputs: [
      { name: 'CU', type: DataType.BOOL, description: '计数输入（上升沿）' },
      { name: 'R', type: DataType.BOOL, description: '复位' },
      { name: 'PV', type: DataType.INT, description: '预设值' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '计数完成' },
      { name: 'CV', type: DataType.INT, description: '当前值' }
    ],
    category: 'counter'
  },
  {
    name: 'CTD',
    description: '递减计数器',
    inputs: [
      { name: 'CD', type: DataType.BOOL, description: '计数输入（上升沿）' },
      { name: 'LD', type: DataType.BOOL, description: '加载预设值' },
      { name: 'PV', type: DataType.INT, description: '预设值' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '计数完成' },
      { name: 'CV', type: DataType.INT, description: '当前值' }
    ],
    category: 'counter'
  },
  {
    name: 'CTUD',
    description: '双向计数器',
    inputs: [
      { name: 'CU', type: DataType.BOOL, description: '递增计数' },
      { name: 'CD', type: DataType.BOOL, description: '递减计数' },
      { name: 'R', type: DataType.BOOL, description: '复位' },
      { name: 'LD', type: DataType.BOOL, description: '加载预设值' },
      { name: 'PV', type: DataType.INT, description: '预设值' }
    ],
    outputs: [
      { name: 'QU', type: DataType.BOOL, description: '向上计数完成' },
      { name: 'QD', type: DataType.BOOL, description: '向下计数完成' },
      { name: 'CV', type: DataType.INT, description: '当前值' }
    ],
    category: 'counter'
  },

  // 双稳态
  {
    name: 'SR',
    description: '置位优先双稳态',
    inputs: [
      { name: 'S1', type: DataType.BOOL, description: '置位（优先）' },
      { name: 'R', type: DataType.BOOL, description: '复位' }
    ],
    outputs: [
      { name: 'Q1', type: DataType.BOOL, description: '输出' }
    ],
    category: 'bistable'
  },
  {
    name: 'RS',
    description: '复位优先双稳态',
    inputs: [
      { name: 'S', type: DataType.BOOL, description: '置位' },
      { name: 'R1', type: DataType.BOOL, description: '复位（优先）' }
    ],
    outputs: [
      { name: 'Q1', type: DataType.BOOL, description: '输出' }
    ],
    category: 'bistable'
  },

  // 边沿检测
  {
    name: 'R_TRIG',
    description: '上升沿检测',
    inputs: [
      { name: 'CLK', type: DataType.BOOL, description: '时钟输入' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '上升沿脉冲' }
    ],
    category: 'edge'
  },
  {
    name: 'F_TRIG',
    description: '下降沿检测',
    inputs: [
      { name: 'CLK', type: DataType.BOOL, description: '时钟输入' }
    ],
    outputs: [
      { name: 'Q', type: DataType.BOOL, description: '下降沿脉冲' }
    ],
    category: 'edge'
  }
]

// 标准函数定义
export interface StandardFunction {
  name: string
  description: string
  inputs: { name: string; type: DataType | 'ANY'; description: string }[]
  returnType: DataType | 'ANY'
  category: 'arithmetic' | 'comparison' | 'bitwise' | 'selection' | 'conversion' | 'string' | 'time' | 'numeric'
}

export const standardFunctions: StandardFunction[] = [
  // 算术运算
  { name: 'ADD', description: '加法', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'arithmetic' },
  { name: 'SUB', description: '减法', inputs: [{ name: 'IN1', type: 'ANY', description: '被减数' }, { name: 'IN2', type: 'ANY', description: '减数' }], returnType: 'ANY', category: 'arithmetic' },
  { name: 'MUL', description: '乘法', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'arithmetic' },
  { name: 'DIV', description: '除法', inputs: [{ name: 'IN1', type: 'ANY', description: '被除数' }, { name: 'IN2', type: 'ANY', description: '除数' }], returnType: 'ANY', category: 'arithmetic' },
  { name: 'MOD', description: '取模', inputs: [{ name: 'IN1', type: 'ANY', description: '被除数' }, { name: 'IN2', type: 'ANY', description: '除数' }], returnType: 'ANY', category: 'arithmetic' },
  { name: 'ABS', description: '绝对值', inputs: [{ name: 'IN', type: 'ANY', description: '输入值' }], returnType: 'ANY', category: 'numeric' },
  { name: 'SQRT', description: '平方根', inputs: [{ name: 'IN', type: DataType.REAL, description: '输入值' }], returnType: DataType.REAL, category: 'numeric' },

  // 比较运算
  { name: 'GT', description: '大于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },
  { name: 'GE', description: '大于等于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },
  { name: 'EQ', description: '等于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },
  { name: 'NE', description: '不等于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },
  { name: 'LE', description: '小于等于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },
  { name: 'LT', description: '小于', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: DataType.BOOL, category: 'comparison' },

  // 位运算
  { name: 'AND', description: '按位与', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'bitwise' },
  { name: 'OR', description: '按位或', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'bitwise' },
  { name: 'XOR', description: '按位异或', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'bitwise' },
  { name: 'NOT', description: '按位取反', inputs: [{ name: 'IN', type: 'ANY', description: '输入值' }], returnType: 'ANY', category: 'bitwise' },
  { name: 'SHL', description: '左移', inputs: [{ name: 'IN', type: 'ANY', description: '输入值' }, { name: 'N', type: DataType.INT, description: '移位位数' }], returnType: 'ANY', category: 'bitwise' },
  { name: 'SHR', description: '右移', inputs: [{ name: 'IN', type: 'ANY', description: '输入值' }, { name: 'N', type: DataType.INT, description: '移位位数' }], returnType: 'ANY', category: 'bitwise' },

  // 选择函数
  { name: 'SEL', description: '选择', inputs: [{ name: 'G', type: DataType.BOOL, description: '选择条件' }, { name: 'IN0', type: 'ANY', description: 'G=0时选择' }, { name: 'IN1', type: 'ANY', description: 'G=1时选择' }], returnType: 'ANY', category: 'selection' },
  { name: 'MIN', description: '最小值', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'selection' },
  { name: 'MAX', description: '最大值', inputs: [{ name: 'IN1', type: 'ANY', description: '操作数1' }, { name: 'IN2', type: 'ANY', description: '操作数2' }], returnType: 'ANY', category: 'selection' },
  { name: 'LIMIT', description: '限幅', inputs: [{ name: 'MN', type: 'ANY', description: '最小值' }, { name: 'IN', type: 'ANY', description: '输入值' }, { name: 'MX', type: 'ANY', description: '最大值' }], returnType: 'ANY', category: 'selection' },

  // 类型转换
  { name: 'INT_TO_REAL', description: 'INT转REAL', inputs: [{ name: 'IN', type: DataType.INT, description: '输入值' }], returnType: DataType.REAL, category: 'conversion' },
  { name: 'REAL_TO_INT', description: 'REAL转INT', inputs: [{ name: 'IN', type: DataType.REAL, description: '输入值' }], returnType: DataType.INT, category: 'conversion' },
  { name: 'BOOL_TO_INT', description: 'BOOL转INT', inputs: [{ name: 'IN', type: DataType.BOOL, description: '输入值' }], returnType: DataType.INT, category: 'conversion' }
]

// 获取功能块分类
export function getFunctionBlocksByCategory(category: string): StandardFunctionBlock[] {
  return standardFunctionBlocks.filter(fb => fb.category === category)
}

// 获取函数分类
export function getFunctionsByCategory(category: string): StandardFunction[] {
  return standardFunctions.filter(f => f.category === category)
}

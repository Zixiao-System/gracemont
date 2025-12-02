/**
 * 获取文件扩展名
 * @param {string} filename - 文件名
 * @returns {string}
 */
export function getFileExtension(filename) {
  const idx = filename.lastIndexOf('.')
  return idx > 0 ? filename.slice(idx + 1).toLowerCase() : ''
}

/**
 * 获取文件名（不含扩展名）
 * @param {string} filepath - 文件路径
 * @returns {string}
 */
export function getFileName(filepath) {
  const name = filepath.split(/[/\\]/).pop() || ''
  const idx = name.lastIndexOf('.')
  return idx > 0 ? name.slice(0, idx) : name
}

/**
 * 获取目录路径
 * @param {string} filepath - 文件路径
 * @returns {string}
 */
export function getDirectory(filepath) {
  const parts = filepath.split(/[/\\]/)
  parts.pop()
  return parts.join('/')
}

/**
 * 规范化路径
 * @param {string} path - 路径
 * @returns {string}
 */
export function normalizePath(path) {
  return path.replace(/\\/g, '/').replace(/\/+/g, '/')
}

/**
 * 连接路径
 * @param  {...string} parts - 路径片段
 * @returns {string}
 */
export function joinPath(...parts) {
  return normalizePath(parts.filter(Boolean).join('/'))
}

/**
 * 文件类型图标映射
 */
export const FILE_ICONS = {
  // 代码文件
  'st': 'code',
  'c': 'code',
  'h': 'code',
  'cpp': 'code',
  'hpp': 'code',
  'js': 'javascript',
  'ts': 'javascript',
  'json': 'settings',
  'xml': 'code',

  // 二进制文件
  'bin': 'memory',
  'elf': 'memory',
  'hex': 'memory',
  'o': 'memory',
  'a': 'memory',

  // 配置文件
  'yaml': 'settings',
  'yml': 'settings',
  'toml': 'settings',
  'ini': 'settings',
  'cfg': 'settings',

  // 文档
  'md': 'description',
  'txt': 'description',
  'pdf': 'picture_as_pdf',
  'doc': 'description',
  'docx': 'description',

  // 图片
  'png': 'image',
  'jpg': 'image',
  'jpeg': 'image',
  'gif': 'image',
  'svg': 'image',
  'ico': 'image',

  // 默认
  'default': 'insert_drive_file'
}

/**
 * 获取文件图标
 * @param {string} filename - 文件名
 * @returns {string}
 */
export function getFileIcon(filename) {
  const ext = getFileExtension(filename)
  return FILE_ICONS[ext] || FILE_ICONS['default']
}

/**
 * 语言类型映射
 */
export const LANGUAGE_MAP = {
  'st': 'st',           // IEC 61131-3 Structured Text
  'c': 'c',
  'h': 'c',
  'cpp': 'cpp',
  'hpp': 'cpp',
  'js': 'javascript',
  'ts': 'typescript',
  'json': 'json',
  'xml': 'xml',
  'yaml': 'yaml',
  'yml': 'yaml',
  'md': 'markdown',
  'txt': 'plaintext',
  'default': 'plaintext'
}

/**
 * 获取文件语言类型
 * @param {string} filename - 文件名
 * @returns {string}
 */
export function getFileLanguage(filename) {
  const ext = getFileExtension(filename)
  return LANGUAGE_MAP[ext] || LANGUAGE_MAP['default']
}

/**
 * 读取文件为 ArrayBuffer
 * @param {File} file - 文件对象
 * @returns {Promise<ArrayBuffer>}
 */
export function readFileAsArrayBuffer(file) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(reader.result)
    reader.onerror = () => reject(reader.error)
    reader.readAsArrayBuffer(file)
  })
}

/**
 * 读取文件为文本
 * @param {File} file - 文件对象
 * @param {string} encoding - 编码
 * @returns {Promise<string>}
 */
export function readFileAsText(file, encoding = 'utf-8') {
  return new Promise((resolve, reject) => {
    const reader = new FileReader()
    reader.onload = () => resolve(reader.result)
    reader.onerror = () => reject(reader.error)
    reader.readAsText(file, encoding)
  })
}

/**
 * 下载数据为文件
 * @param {Blob|string} data - 数据
 * @param {string} filename - 文件名
 * @param {string} mimeType - MIME 类型
 */
export function downloadFile(data, filename, mimeType = 'application/octet-stream') {
  const blob = data instanceof Blob ? data : new Blob([data], { type: mimeType })
  const url = URL.createObjectURL(blob)

  const link = document.createElement('a')
  link.href = url
  link.download = filename
  link.click()

  URL.revokeObjectURL(url)
}

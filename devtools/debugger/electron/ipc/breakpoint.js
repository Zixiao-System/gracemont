const { ipcMain } = require('electron')

// 断点存储
const breakpoints = new Map()
let breakpointIdCounter = 1

// 添加断点
ipcMain.handle('breakpoint:add', async (event, bp) => {
  try {
    const id = breakpointIdCounter++
    const breakpoint = {
      id,
      file: bp.file,
      line: bp.line,
      condition: bp.condition || null,
      enabled: true,
      hitCount: 0
    }

    breakpoints.set(id, breakpoint)

    return { success: true, breakpoint }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 移除断点
ipcMain.handle('breakpoint:remove', async (event, id) => {
  try {
    if (!breakpoints.has(id)) {
      return { success: false, error: 'Breakpoint not found' }
    }

    breakpoints.delete(id)

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 启用/禁用断点
ipcMain.handle('breakpoint:enable', async (event, { id, enabled }) => {
  try {
    const bp = breakpoints.get(id)
    if (!bp) {
      return { success: false, error: 'Breakpoint not found' }
    }

    bp.enabled = enabled

    return { success: true, breakpoint: bp }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 列出所有断点
ipcMain.handle('breakpoint:list', async () => {
  try {
    return {
      success: true,
      breakpoints: Array.from(breakpoints.values())
    }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

module.exports = { breakpoints }

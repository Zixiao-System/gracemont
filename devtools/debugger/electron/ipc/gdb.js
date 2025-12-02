const { ipcMain } = require('electron')
const { spawn } = require('child_process')

let gdbProcess = null
let gdbState = {
  running: false,
  connected: false,
  paused: false,
  currentFile: null,
  currentLine: null
}

// GDB MI 命令队列
let commandQueue = []
let currentCommand = null

// 启动 GDB
ipcMain.handle('gdb:start', async (event, config) => {
  try {
    if (gdbProcess) {
      return { success: false, error: 'GDB already running' }
    }

    const args = ['--interpreter=mi2']

    // 添加可执行文件
    if (config.executable) {
      args.push(config.executable)
    }

    // 启动 GDB 进程
    gdbProcess = spawn(config.gdbPath || 'aarch64-none-elf-gdb', args)

    gdbState = {
      running: true,
      connected: false,
      paused: true,
      currentFile: null,
      currentLine: null
    }

    // 处理 GDB 输出
    gdbProcess.stdout.on('data', (data) => {
      const output = data.toString()
      parseGDBOutput(event, output)
    })

    gdbProcess.stderr.on('data', (data) => {
      event.sender.send('gdb:output', {
        type: 'stderr',
        data: data.toString()
      })
    })

    // 进程退出
    gdbProcess.on('exit', (code) => {
      gdbState.running = false
      gdbProcess = null
      event.sender.send('gdb:state', { ...gdbState })
    })

    event.sender.send('gdb:state', gdbState)

    // 如果配置了远程目标，自动连接
    if (config.target) {
      await sendGDBCommand(`target remote ${config.target}`)
      gdbState.connected = true
    }

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 停止 GDB
ipcMain.handle('gdb:stop', async () => {
  try {
    if (!gdbProcess) {
      return { success: false, error: 'GDB not running' }
    }

    gdbProcess.kill('SIGTERM')
    gdbProcess = null
    gdbState.running = false

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 发送 GDB 命令
ipcMain.handle('gdb:command', async (event, cmd) => {
  return sendGDBCommand(cmd)
})

// 继续执行
ipcMain.handle('gdb:continue', async (event) => {
  gdbState.paused = false
  event.sender.send('gdb:state', gdbState)
  return sendGDBCommand('-exec-continue')
})

// 单步执行 (进入函数)
ipcMain.handle('gdb:step', async (event) => {
  return sendGDBCommand('-exec-step')
})

// 单步执行 (跳过函数)
ipcMain.handle('gdb:step-over', async (event) => {
  return sendGDBCommand('-exec-next')
})

// 跳出函数
ipcMain.handle('gdb:step-out', async (event) => {
  return sendGDBCommand('-exec-finish')
})

// 暂停执行
ipcMain.handle('gdb:pause', async (event) => {
  if (gdbProcess) {
    gdbProcess.kill('SIGINT')
    gdbState.paused = true
    event.sender.send('gdb:state', gdbState)
    return { success: true }
  }
  return { success: false, error: 'GDB not running' }
})

// 获取局部变量
ipcMain.handle('gdb:variables', async () => {
  return sendGDBCommand('-stack-list-locals --all-values')
})

// 获取调用栈
ipcMain.handle('gdb:callstack', async () => {
  return sendGDBCommand('-stack-list-frames')
})

// 获取寄存器
ipcMain.handle('gdb:registers', async () => {
  return sendGDBCommand('-data-list-register-values x')
})

// 计算表达式
ipcMain.handle('gdb:evaluate', async (event, expr) => {
  return sendGDBCommand(`-data-evaluate-expression "${expr}"`)
})

// 发送 GDB 命令的辅助函数
function sendGDBCommand(cmd) {
  return new Promise((resolve, reject) => {
    if (!gdbProcess) {
      reject(new Error('GDB not running'))
      return
    }

    gdbProcess.stdin.write(cmd + '\n')
    resolve({ success: true })
  })
}

// 解析 GDB MI 输出
function parseGDBOutput(event, output) {
  const lines = output.split('\n').filter(line => line.trim())

  for (const line of lines) {
    // 发送原始输出
    event.sender.send('gdb:output', {
      type: 'stdout',
      data: line
    })

    // 解析 MI 输出
    if (line.startsWith('*stopped')) {
      gdbState.paused = true

      // 解析停止原因和位置
      const reasonMatch = line.match(/reason="([^"]+)"/)
      const fileMatch = line.match(/file="([^"]+)"/)
      const lineMatch = line.match(/line="(\d+)"/)

      if (fileMatch) gdbState.currentFile = fileMatch[1]
      if (lineMatch) gdbState.currentLine = parseInt(lineMatch[1])

      event.sender.send('gdb:state', gdbState)

      if (reasonMatch && reasonMatch[1] === 'breakpoint-hit') {
        event.sender.send('gdb:breakpoint-hit', {
          file: gdbState.currentFile,
          line: gdbState.currentLine
        })
      }
    } else if (line.startsWith('*running')) {
      gdbState.paused = false
      event.sender.send('gdb:state', gdbState)
    } else if (line.startsWith('^connected')) {
      gdbState.connected = true
      event.sender.send('gdb:state', gdbState)
    }
  }
}

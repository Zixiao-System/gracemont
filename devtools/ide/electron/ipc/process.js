const { spawn } = require('child_process')

const processes = new Map()

function setupProcessHandlers(ipcMain) {
  // 启动进程
  ipcMain.handle('process:spawn', async (event, cmd, args = [], options = {}) => {
    try {
      const proc = spawn(cmd, args, {
        cwd: options.cwd || process.cwd(),
        env: { ...process.env, ...options.env },
        shell: true
      })

      const pid = proc.pid
      processes.set(pid, proc)

      // 监听标准输出
      proc.stdout.on('data', (data) => {
        event.sender.send('process:output', {
          pid: pid,
          type: 'stdout',
          data: data.toString()
        })
      })

      // 监听标准错误
      proc.stderr.on('data', (data) => {
        event.sender.send('process:error', {
          pid: pid,
          type: 'stderr',
          data: data.toString()
        })
      })

      // 监听进程退出
      proc.on('exit', (code, signal) => {
        event.sender.send('process:exit', {
          pid: pid,
          code: code,
          signal: signal
        })
        processes.delete(pid)
      })

      // 监听错误
      proc.on('error', (error) => {
        event.sender.send('process:error', {
          pid: pid,
          error: error.message
        })
        processes.delete(pid)
      })

      return {
        success: true,
        pid: pid
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 终止进程
  ipcMain.handle('process:kill', async (_event, pid) => {
    try {
      const proc = processes.get(pid)
      if (proc) {
        proc.kill()
        processes.delete(pid)
        return { success: true }
      } else {
        return {
          success: false,
          error: 'Process not found'
        }
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })
}

module.exports = { setupProcessHandlers }

const { ipcMain } = require('electron')
const { spawn } = require('child_process')
const path = require('path')

let qemuProcess = null
let qemuStatus = {
  running: false,
  config: null,
  pid: null
}

// 启动 QEMU
ipcMain.handle('qemu:start', async (event, config) => {
  try {
    if (qemuProcess) {
      return { success: false, error: 'QEMU already running' }
    }

    // QEMU 参数
    const args = [
      '-machine', config.machine || 'virt',
      '-cpu', config.cpu || 'cortex-a57',
      '-m', config.memory || '512M',
      '-kernel', config.kernelPath,
      '-nographic',
      '-serial', 'stdio'
    ]

    // 添加额外设备
    if (config.devices) {
      config.devices.forEach(device => {
        args.push('-device', device)
      })
    }

    // 启动 QEMU 进程
    qemuProcess = spawn('qemu-system-aarch64', args)

    qemuStatus = {
      running: true,
      config: config,
      pid: qemuProcess.pid
    }

    // 监听输出
    qemuProcess.stdout.on('data', (data) => {
      event.sender.send('qemu:output', {
        type: 'stdout',
        data: data.toString()
      })
    })

    qemuProcess.stderr.on('data', (data) => {
      event.sender.send('qemu:output', {
        type: 'stderr',
        data: data.toString()
      })
    })

    // 进程退出
    qemuProcess.on('exit', (code) => {
      qemuStatus.running = false
      qemuProcess = null
      event.sender.send('qemu:state', {
        running: false,
        exitCode: code
      })
    })

    event.sender.send('qemu:state', { running: true })

    return { success: true, pid: qemuProcess.pid }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 停止 QEMU
ipcMain.handle('qemu:stop', async () => {
  try {
    if (!qemuProcess) {
      return { success: false, error: 'QEMU not running' }
    }

    qemuProcess.kill('SIGTERM')
    qemuProcess = null
    qemuStatus.running = false

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 获取 QEMU 状态
ipcMain.handle('qemu:status', async () => {
  return { success: true, status: qemuStatus }
})

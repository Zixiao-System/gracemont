const { ipcMain } = require('electron')

// 模拟 I/O 状态
const ioState = {
  digitalInputs: Array(32).fill(false),   // 32个数字输入
  digitalOutputs: Array(32).fill(false),  // 32个数字输出
  analogInputs: Array(16).fill(0),        // 16个模拟输入 (0-4095)
  analogOutputs: Array(8).fill(0)         // 8个模拟输出 (0-4095)
}

// 设置数字输出
ipcMain.handle('io:digital-set', async (event, { pin, value }) => {
  try {
    if (pin < 0 || pin >= ioState.digitalOutputs.length) {
      return { success: false, error: 'Invalid pin number' }
    }

    ioState.digitalOutputs[pin] = Boolean(value)

    // 通知变化
    event.sender.send('io:change', {
      type: 'digital-output',
      pin,
      value: ioState.digitalOutputs[pin]
    })

    return { success: true, value: ioState.digitalOutputs[pin] }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 设置模拟输出
ipcMain.handle('io:analog-set', async (event, { channel, value }) => {
  try {
    if (channel < 0 || channel >= ioState.analogOutputs.length) {
      return { success: false, error: 'Invalid channel number' }
    }

    // 限制在 0-4095 范围
    const clampedValue = Math.max(0, Math.min(4095, Math.round(value)))
    ioState.analogOutputs[channel] = clampedValue

    // 通知变化
    event.sender.send('io:change', {
      type: 'analog-output',
      channel,
      value: clampedValue
    })

    return { success: true, value: clampedValue }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 获取数字输入
ipcMain.handle('io:digital-get', async (event, pin) => {
  try {
    if (pin < 0 || pin >= ioState.digitalInputs.length) {
      return { success: false, error: 'Invalid pin number' }
    }

    return { success: true, value: ioState.digitalInputs[pin] }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 获取模拟输入
ipcMain.handle('io:analog-get', async (event, channel) => {
  try {
    if (channel < 0 || channel >= ioState.analogInputs.length) {
      return { success: false, error: 'Invalid channel number' }
    }

    return { success: true, value: ioState.analogInputs[channel] }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// 模拟输入变化（用于测试）
function simulateInputChange(event) {
  // 随机改变一些输入
  const randomPin = Math.floor(Math.random() * ioState.digitalInputs.length)
  ioState.digitalInputs[randomPin] = Math.random() > 0.5

  const randomChannel = Math.floor(Math.random() * ioState.analogInputs.length)
  ioState.analogInputs[randomChannel] = Math.floor(Math.random() * 4096)
}

module.exports = { ioState }

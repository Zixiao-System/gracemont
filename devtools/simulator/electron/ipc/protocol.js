const { ipcMain } = require('electron')
const ModbusRTU = require('modbus-serial')
const { OPCUAClient, AttributeIds, DataType } = require('node-opcua')

// Modbus 客户端
let modbusClient = null
let modbusConnected = false

// OPC UA 客户端
let opcuaClient = null
let opcuaSession = null
let opcuaConnected = false

// ========== Modbus ==========

ipcMain.handle('modbus:connect', async (event, config) => {
  try {
    if (modbusClient) {
      modbusClient.close(() => {})
    }

    modbusClient = new ModbusRTU()

    // 根据类型连接
    if (config.type === 'tcp') {
      await modbusClient.connectTCP(config.host, { port: config.port || 502 })
    } else if (config.type === 'rtu') {
      await modbusClient.connectRTUBuffered(config.port, {
        baudRate: config.baudRate || 9600,
        dataBits: config.dataBits || 8,
        parity: config.parity || 'none',
        stopBits: config.stopBits || 1
      })
    }

    modbusClient.setID(config.unitId || 1)
    modbusClient.setTimeout(config.timeout || 1000)
    modbusConnected = true

    return { success: true }
  } catch (error) {
    modbusConnected = false
    return { success: false, error: error.message }
  }
})

ipcMain.handle('modbus:disconnect', async () => {
  try {
    if (modbusClient) {
      modbusClient.close(() => {})
      modbusClient = null
      modbusConnected = false
    }
    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('modbus:read', async (event, request) => {
  try {
    if (!modbusConnected) {
      return { success: false, error: 'Not connected' }
    }

    let result
    const { type, address, length } = request

    switch (type) {
      case 'coils':
        result = await modbusClient.readCoils(address, length)
        break
      case 'discrete-inputs':
        result = await modbusClient.readDiscreteInputs(address, length)
        break
      case 'holding-registers':
        result = await modbusClient.readHoldingRegisters(address, length)
        break
      case 'input-registers':
        result = await modbusClient.readInputRegisters(address, length)
        break
      default:
        return { success: false, error: 'Invalid read type' }
    }

    return { success: true, data: result.data }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('modbus:write', async (event, request) => {
  try {
    if (!modbusConnected) {
      return { success: false, error: 'Not connected' }
    }

    const { type, address, value } = request

    switch (type) {
      case 'coil':
        await modbusClient.writeCoil(address, value)
        break
      case 'register':
        await modbusClient.writeRegister(address, value)
        break
      case 'coils':
        await modbusClient.writeCoils(address, value)
        break
      case 'registers':
        await modbusClient.writeRegisters(address, value)
        break
      default:
        return { success: false, error: 'Invalid write type' }
    }

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

// ========== OPC UA ==========

ipcMain.handle('opcua:connect', async (event, config) => {
  try {
    if (opcuaClient) {
      await opcuaClient.disconnect()
    }

    opcuaClient = OPCUAClient.create({
      endpoint_must_exist: false,
      connectionStrategy: {
        maxRetry: config.maxRetry || 3,
        initialDelay: config.initialDelay || 2000,
        maxDelay: config.maxDelay || 10000
      }
    })

    await opcuaClient.connect(config.endpointUrl)
    opcuaSession = await opcuaClient.createSession()
    opcuaConnected = true

    return { success: true }
  } catch (error) {
    opcuaConnected = false
    return { success: false, error: error.message }
  }
})

ipcMain.handle('opcua:disconnect', async () => {
  try {
    if (opcuaSession) {
      await opcuaSession.close()
      opcuaSession = null
    }
    if (opcuaClient) {
      await opcuaClient.disconnect()
      opcuaClient = null
    }
    opcuaConnected = false

    return { success: true }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('opcua:read', async (event, nodeId) => {
  try {
    if (!opcuaConnected || !opcuaSession) {
      return { success: false, error: 'Not connected' }
    }

    const dataValue = await opcuaSession.read({
      nodeId: nodeId,
      attributeId: AttributeIds.Value
    })

    return {
      success: true,
      value: dataValue.value.value,
      statusCode: dataValue.statusCode.value,
      sourceTimestamp: dataValue.sourceTimestamp
    }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

ipcMain.handle('opcua:write', async (event, { nodeId, value }) => {
  try {
    if (!opcuaConnected || !opcuaSession) {
      return { success: false, error: 'Not connected' }
    }

    const statusCode = await opcuaSession.write({
      nodeId: nodeId,
      attributeId: AttributeIds.Value,
      value: {
        value: {
          dataType: DataType.Double, // 可根据需要调整
          value: value
        }
      }
    })

    return { success: true, statusCode: statusCode.value }
  } catch (error) {
    return { success: false, error: error.message }
  }
})

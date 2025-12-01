const { dialog } = require('electron')
const fs = require('fs').promises
const path = require('path')

function setupProjectHandlers(ipcMain) {
  // 创建新项目
  ipcMain.handle('project:create', async (_event, projectPath, config) => {
    try {
      // 创建项目目录
      await fs.mkdir(projectPath, { recursive: true })

      // 创建项目结构
      const dirs = ['src', 'build', 'lib']
      for (const dir of dirs) {
        await fs.mkdir(path.join(projectPath, dir), { recursive: true })
      }

      // 创建项目配置文件
      const projectConfig = {
        name: config.name || 'Untitled Project',
        version: '1.0.0',
        target: config.target || 'arm64',
        rtos: config.rtos || 'gracemont',
        created: new Date().toISOString(),
        files: []
      }

      await fs.writeFile(
        path.join(projectPath, 'project.json'),
        JSON.stringify(projectConfig, null, 2),
        'utf-8'
      )

      // 创建示例主程序
      const mainProgram = `PROGRAM Main
  VAR
    counter : INT := 0;
    enabled : BOOL := TRUE;
  END_VAR

  IF enabled THEN
    counter := counter + 1;
  END_IF
END_PROGRAM
`
      await fs.writeFile(
        path.join(projectPath, 'src', 'main.st'),
        mainProgram,
        'utf-8'
      )

      return {
        success: true,
        path: projectPath,
        config: projectConfig
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 打开项目
  ipcMain.handle('project:open', async () => {
    const result = await dialog.showOpenDialog({
      properties: ['openDirectory']
    })

    if (result.canceled) {
      return null
    }

    const projectPath = result.filePaths[0]

    try {
      // 读取项目配置
      const configPath = path.join(projectPath, 'project.json')
      const configContent = await fs.readFile(configPath, 'utf-8')
      const config = JSON.parse(configContent)

      return {
        success: true,
        path: projectPath,
        config: config
      }
    } catch (error) {
      return {
        success: false,
        error: `Invalid project: ${error.message}`
      }
    }
  })

  // 保存项目配置
  ipcMain.handle('project:save', async (_event, projectPath, config) => {
    try {
      const configPath = path.join(projectPath, 'project.json')
      await fs.writeFile(
        configPath,
        JSON.stringify(config, null, 2),
        'utf-8'
      )
      return { success: true }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })

  // 获取项目文件树
  ipcMain.handle('project:getFiles', async (_event, projectPath) => {
    try {
      async function buildTree(dirPath, relativePath = '') {
        const entries = await fs.readdir(dirPath, { withFileTypes: true })
        const items = []

        for (const entry of entries) {
          // 跳过隐藏文件和 build 目录
          if (entry.name.startsWith('.') || entry.name === 'build' || entry.name === 'node_modules') {
            continue
          }

          const fullPath = path.join(dirPath, entry.name)
          const relPath = path.join(relativePath, entry.name)

          if (entry.isDirectory()) {
            items.push({
              name: entry.name,
              path: relPath,
              type: 'directory',
              children: await buildTree(fullPath, relPath)
            })
          } else {
            const stats = await fs.stat(fullPath)
            items.push({
              name: entry.name,
              path: relPath,
              type: 'file',
              size: stats.size,
              ext: path.extname(entry.name)
            })
          }
        }

        return items
      }

      const tree = await buildTree(projectPath)

      return {
        success: true,
        tree: tree
      }
    } catch (error) {
      return {
        success: false,
        error: error.message
      }
    }
  })
}

module.exports = { setupProjectHandlers }

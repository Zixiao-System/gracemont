import { contextBridge, ipcRenderer } from 'electron'

contextBridge.exposeInMainWorld('electronAPI', {
  getAppPath: () => ipcRenderer.invoke('get-app-path'),
  onMenuAction: (callback: (action: string) => void) => {
    const actions = [
      'menu:new-project', 'menu:open-project', 'menu:save', 'menu:save-all',
      'menu:export', 'menu:compile', 'menu:compile-all', 'menu:download',
      'menu:upload', 'menu:run', 'menu:stop', 'menu:monitor',
      'menu:find', 'menu:replace', 'menu:view-project', 'menu:view-variables',
      'menu:view-crossref', 'menu:view-output', 'menu:help-iec',
      'menu:help-instructions', 'menu:about'
    ]
    actions.forEach(action => {
      ipcRenderer.on(action, () => callback(action))
    })
  }
})

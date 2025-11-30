export {}

declare global {
  interface Window {
    electronAPI?: {
      getAppPath: () => Promise<string>
      onMenuAction: (callback: (action: string) => void) => void
    }
  }
}

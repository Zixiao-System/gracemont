/// <reference types="vite/client" />

declare module '*.vue' {
  import type { DefineComponent } from 'vue'
  const component: DefineComponent<{}, {}, any>
  export default component
}

// MDUI 组件类型声明
declare namespace JSX {
  interface IntrinsicElements {
    'mdui-button': any
    'mdui-button-icon': any
    'mdui-card': any
    'mdui-checkbox': any
    'mdui-chip': any
    'mdui-collapse': any
    'mdui-collapse-item': any
    'mdui-dialog': any
    'mdui-divider': any
    'mdui-dropdown': any
    'mdui-icon': any
    'mdui-layout': any
    'mdui-layout-main': any
    'mdui-list': any
    'mdui-list-item': any
    'mdui-list-subheader': any
    'mdui-menu': any
    'mdui-menu-item': any
    'mdui-navigation-drawer': any
    'mdui-segmented-button': any
    'mdui-segmented-button-group': any
    'mdui-select': any
    'mdui-switch': any
    'mdui-tab': any
    'mdui-tab-panel': any
    'mdui-tabs': any
    'mdui-text-field': any
    'mdui-tooltip': any
    'mdui-top-app-bar': any
    'mdui-top-app-bar-title': any
    'mdui-badge': any
    [key: string]: any
  }
}

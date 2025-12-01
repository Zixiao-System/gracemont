import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'

// 导入 MDUI CSS
import 'mdui/mdui.css'
// 导入 MDUI 组件
import 'mdui'

// 导入 Monaco Editor 样式
import 'monaco-editor/min/vs/editor/editor.main.css'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)
app.use(router)

app.mount('#app')

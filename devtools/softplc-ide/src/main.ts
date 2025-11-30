import { createApp } from 'vue'
import { createPinia } from 'pinia'
import router from './router'
import App from './App.vue'

// MDUI 样式和组件
import 'mdui/mdui.css'
import 'mdui'

// 设置中文语言
import { loadLocale } from 'mdui/functions/loadLocale.js'
import { setLocale } from 'mdui/functions/setLocale.js'

loadLocale((locale: string) => import(`../node_modules/mdui/locales/${locale}.js`))
setLocale('zh-cn')

// 全局样式
import './styles/global.css'

const app = createApp(App)
const pinia = createPinia()

app.use(pinia)
app.use(router)
app.mount('#app')

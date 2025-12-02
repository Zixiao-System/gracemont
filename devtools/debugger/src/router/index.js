import { createRouter, createWebHistory } from 'vue-router'
import DebuggerView from '../views/DebuggerView.vue'

const routes = [
  {
    path: '/',
    name: 'debugger',
    component: DebuggerView
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router

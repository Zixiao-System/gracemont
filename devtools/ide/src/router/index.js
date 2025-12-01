import { createRouter, createMemoryHistory } from 'vue-router'
import IDEView from '../views/IDEView.vue'

const routes = [
  {
    path: '/',
    name: 'ide',
    component: IDEView
  }
]

const router = createRouter({
  history: createMemoryHistory(),
  routes
})

export default router

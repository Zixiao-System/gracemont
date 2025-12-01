import { createRouter, createMemoryHistory } from 'vue-router'
import FlasherView from '../views/FlasherView.vue'

const routes = [
  {
    path: '/',
    name: 'flasher',
    component: FlasherView
  }
]

const router = createRouter({
  history: createMemoryHistory(),
  routes
})

export default router

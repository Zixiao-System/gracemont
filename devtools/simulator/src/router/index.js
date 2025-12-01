import { createRouter, createWebHistory } from 'vue-router'
import SimulatorView from '../views/SimulatorView.vue'

const routes = [
  {
    path: '/',
    name: 'simulator',
    component: SimulatorView
  }
]

const router = createRouter({
  history: createWebHistory(),
  routes
})

export default router

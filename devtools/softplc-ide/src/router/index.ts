import { createRouter, createWebHashHistory } from 'vue-router'
import type { RouteRecordRaw } from 'vue-router'

const routes: RouteRecordRaw[] = [
  {
    path: '/',
    name: 'welcome',
    component: () => import('@/views/WelcomeView.vue')
  },
  {
    path: '/editor/st/:id',
    name: 'st-editor',
    component: () => import('@/views/editors/STEditorView.vue')
  },
  {
    path: '/editor/ld/:id',
    name: 'ld-editor',
    component: () => import('@/views/editors/LDEditorView.vue')
  },
  {
    path: '/editor/fbd/:id',
    name: 'fbd-editor',
    component: () => import('@/views/editors/FBDEditorView.vue')
  },
  {
    path: '/editor/il/:id',
    name: 'il-editor',
    component: () => import('@/views/editors/ILEditorView.vue')
  },
  {
    path: '/editor/sfc/:id',
    name: 'sfc-editor',
    component: () => import('@/views/editors/SFCEditorView.vue')
  },
  {
    path: '/variables/:id',
    name: 'variables',
    component: () => import('@/views/VariablesView.vue')
  },
  {
    path: '/config/:id',
    name: 'config',
    component: () => import('@/views/ConfigView.vue')
  }
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

export default router

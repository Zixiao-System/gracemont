<template>
  <div class="variables-view">
    <div class="variables-toolbar">
      <mdui-button variant="outlined" @click="addVariable">
        <mdui-icon-add slot="icon"></mdui-icon-add>
        添加变量
      </mdui-button>

      <mdui-button variant="outlined" @click="deleteSelected" :disabled="selectedVariables.length === 0">
        <mdui-icon-delete slot="icon"></mdui-icon-delete>
        删除
      </mdui-button>

      <mdui-divider vertical style="height: 24px; margin: 0 16px;"></mdui-divider>

      <mdui-text-field
        v-model="searchText"
        placeholder="搜索变量..."
        variant="outlined"
        style="width: 200px; --mdui-comp-text-field-height: 36px;"
      >
        <mdui-icon-search slot="icon"></mdui-icon-search>
      </mdui-text-field>

      <div class="spacer"></div>

      <mdui-segmented-button-group>
        <mdui-segmented-button value="all" :selected="filterClass === 'all'" @click="filterClass = 'all'">
          全部
        </mdui-segmented-button>
        <mdui-segmented-button value="VAR_INPUT" :selected="filterClass === 'VAR_INPUT'" @click="filterClass = 'VAR_INPUT'">
          输入
        </mdui-segmented-button>
        <mdui-segmented-button value="VAR_OUTPUT" :selected="filterClass === 'VAR_OUTPUT'" @click="filterClass = 'VAR_OUTPUT'">
          输出
        </mdui-segmented-button>
        <mdui-segmented-button value="VAR" :selected="filterClass === 'VAR'" @click="filterClass = 'VAR'">
          本地
        </mdui-segmented-button>
      </mdui-segmented-button-group>
    </div>

    <div class="variables-table-container">
      <table class="variables-table">
        <thead>
          <tr>
            <th style="width: 40px;">
              <mdui-checkbox
                :checked="isAllSelected"
                :indeterminate="isSomeSelected && !isAllSelected"
                @change="toggleSelectAll"
              ></mdui-checkbox>
            </th>
            <th style="width: 180px;">名称</th>
            <th style="width: 120px;">数据类型</th>
            <th style="width: 100px;">变量类型</th>
            <th style="width: 120px;">初始值</th>
            <th style="width: 120px;">地址</th>
            <th>注释</th>
            <th style="width: 60px;"></th>
          </tr>
        </thead>
        <tbody>
          <tr
            v-for="variable in filteredVariables"
            :key="variable.id"
            :class="{ selected: selectedVariables.includes(variable.id) }"
          >
            <td>
              <mdui-checkbox
                :checked="selectedVariables.includes(variable.id)"
                @change="toggleSelect(variable.id)"
              ></mdui-checkbox>
            </td>
            <td>
              <mdui-text-field
                :value="variable.name"
                @input="updateVariable(variable.id, 'name', ($event.target as HTMLInputElement).value)"
                variant="outlined"
                style="--mdui-comp-text-field-height: 32px;"
              ></mdui-text-field>
            </td>
            <td>
              <mdui-select
                :value="variable.dataType"
                @change="updateVariable(variable.id, 'dataType', ($event.target as HTMLSelectElement).value)"
                style="--mdui-comp-select-height: 32px;"
              >
                <mdui-menu-item value="BOOL">BOOL</mdui-menu-item>
                <mdui-menu-item value="BYTE">BYTE</mdui-menu-item>
                <mdui-menu-item value="WORD">WORD</mdui-menu-item>
                <mdui-menu-item value="DWORD">DWORD</mdui-menu-item>
                <mdui-menu-item value="SINT">SINT</mdui-menu-item>
                <mdui-menu-item value="INT">INT</mdui-menu-item>
                <mdui-menu-item value="DINT">DINT</mdui-menu-item>
                <mdui-menu-item value="REAL">REAL</mdui-menu-item>
                <mdui-menu-item value="LREAL">LREAL</mdui-menu-item>
                <mdui-menu-item value="TIME">TIME</mdui-menu-item>
                <mdui-menu-item value="STRING">STRING</mdui-menu-item>
              </mdui-select>
            </td>
            <td>
              <mdui-select
                :value="variable.variableClass"
                @change="updateVariable(variable.id, 'variableClass', ($event.target as HTMLSelectElement).value)"
                style="--mdui-comp-select-height: 32px;"
              >
                <mdui-menu-item value="VAR">VAR</mdui-menu-item>
                <mdui-menu-item value="VAR_INPUT">VAR_INPUT</mdui-menu-item>
                <mdui-menu-item value="VAR_OUTPUT">VAR_OUTPUT</mdui-menu-item>
                <mdui-menu-item value="VAR_IN_OUT">VAR_IN_OUT</mdui-menu-item>
                <mdui-menu-item value="VAR_TEMP">VAR_TEMP</mdui-menu-item>
              </mdui-select>
            </td>
            <td>
              <mdui-text-field
                :value="variable.initialValue"
                @input="updateVariable(variable.id, 'initialValue', ($event.target as HTMLInputElement).value)"
                variant="outlined"
                :placeholder="getDefaultValue(variable.dataType)"
                style="--mdui-comp-text-field-height: 32px;"
              ></mdui-text-field>
            </td>
            <td>
              <mdui-text-field
                :value="variable.address"
                @input="updateVariable(variable.id, 'address', ($event.target as HTMLInputElement).value)"
                variant="outlined"
                placeholder="%IX0.0"
                style="--mdui-comp-text-field-height: 32px;"
              ></mdui-text-field>
            </td>
            <td>
              <mdui-text-field
                :value="variable.comment"
                @input="updateVariable(variable.id, 'comment', ($event.target as HTMLInputElement).value)"
                variant="outlined"
                placeholder="注释"
                style="--mdui-comp-text-field-height: 32px;"
              ></mdui-text-field>
            </td>
            <td>
              <mdui-button-icon @click="deleteVariable(variable.id)">
                <mdui-icon-delete></mdui-icon-delete>
              </mdui-button-icon>
            </td>
          </tr>
        </tbody>
      </table>

      <div class="empty-state" v-if="filteredVariables.length === 0">
        <mdui-icon-inventory-2 style="font-size: 48px; opacity: 0.3;"></mdui-icon-inventory-2>
        <p>暂无变量</p>
        <mdui-button @click="addVariable">添加变量</mdui-button>
      </div>
    </div>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue'
import { useRoute } from 'vue-router'
import { useProjectStore } from '@/stores/project'
import { useEditorStore } from '@/stores/editor'
import type { Variable } from '@/types/iec61131'
import { DataType, VariableClass } from '@/types/iec61131'
import { v4 as uuidv4 } from 'uuid'

import '@mdui/icons/add.js'
import '@mdui/icons/delete.js'
import '@mdui/icons/search.js'
import '@mdui/icons/inventory-2.js'

const route = useRoute()
const projectStore = useProjectStore()
const editorStore = useEditorStore()

const pouId = computed(() => route.params.id as string)
const isGlobal = computed(() => pouId.value === 'global')
const pou = computed(() => isGlobal.value ? null : projectStore.getPOU(pouId.value))

const searchText = ref('')
const filterClass = ref('all')
const selectedVariables = ref<string[]>([])

// 获取变量列表
const variables = computed(() => {
  if (isGlobal.value) {
    return projectStore.globalVariables
  }
  return pou.value?.variables || []
})

// 过滤后的变量
const filteredVariables = computed(() => {
  let result = variables.value

  if (filterClass.value !== 'all') {
    result = result.filter(v => v.variableClass === filterClass.value)
  }

  if (searchText.value) {
    const search = searchText.value.toLowerCase()
    result = result.filter(v =>
      v.name.toLowerCase().includes(search) ||
      v.comment?.toLowerCase().includes(search)
    )
  }

  return result
})

const isAllSelected = computed(() =>
  filteredVariables.value.length > 0 &&
  filteredVariables.value.every(v => selectedVariables.value.includes(v.id))
)

const isSomeSelected = computed(() =>
  filteredVariables.value.some(v => selectedVariables.value.includes(v.id))
)

function toggleSelectAll() {
  if (isAllSelected.value) {
    selectedVariables.value = []
  } else {
    selectedVariables.value = filteredVariables.value.map(v => v.id)
  }
}

function toggleSelect(id: string) {
  const index = selectedVariables.value.indexOf(id)
  if (index >= 0) {
    selectedVariables.value.splice(index, 1)
  } else {
    selectedVariables.value.push(id)
  }
}

function addVariable() {
  const newVar: Variable = {
    id: uuidv4(),
    name: `Var${variables.value.length + 1}`,
    dataType: DataType.BOOL,
    variableClass: VariableClass.VAR,
    initialValue: '',
    address: '',
    comment: ''
  }

  if (isGlobal.value) {
    projectStore.addGlobalVariable(newVar)
  } else if (pou.value) {
    pou.value.variables.push(newVar)
    markModified()
  }
}

function updateVariable(id: string, field: keyof Variable, value: string) {
  const variable = variables.value.find(v => v.id === id)
  if (variable) {
    (variable as any)[field] = value
    markModified()
  }
}

function deleteVariable(id: string) {
  if (isGlobal.value) {
    // 从全局变量中删除
    const resource = projectStore.currentProject?.resources[0]
    if (resource) {
      resource.globalVariables = resource.globalVariables.filter(v => v.id !== id)
    }
  } else if (pou.value) {
    pou.value.variables = pou.value.variables.filter(v => v.id !== id)
  }
  selectedVariables.value = selectedVariables.value.filter(vid => vid !== id)
  markModified()
}

function deleteSelected() {
  selectedVariables.value.forEach(id => {
    deleteVariable(id)
  })
  selectedVariables.value = []
}

function getDefaultValue(dataType: DataType): string {
  const defaults: Record<string, string> = {
    [DataType.BOOL]: 'FALSE',
    [DataType.INT]: '0',
    [DataType.DINT]: '0',
    [DataType.REAL]: '0.0',
    [DataType.STRING]: "''",
    [DataType.TIME]: 'T#0s'
  }
  return defaults[dataType] || '0'
}

function markModified() {
  if (isGlobal.value) {
    editorStore.markModified('global-variables', true)
  } else {
    editorStore.markModified(`${pouId.value}-variables`, true)
  }
}
</script>

<style scoped>
.variables-view {
  height: 100%;
  display: flex;
  flex-direction: column;
  background-color: rgb(var(--mdui-color-surface));
}

.variables-toolbar {
  display: flex;
  align-items: center;
  padding: 12px 16px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  background-color: rgb(var(--mdui-color-surface-container));
  gap: 8px;
}

.spacer {
  flex: 1;
}

.variables-table-container {
  flex: 1;
  overflow: auto;
}

.variables-table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

.variables-table th {
  text-align: left;
  padding: 12px 8px;
  background-color: rgb(var(--mdui-color-surface-container));
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  font-weight: 500;
  position: sticky;
  top: 0;
  z-index: 1;
}

.variables-table td {
  padding: 4px 8px;
  border-bottom: 1px solid rgb(var(--mdui-color-outline-variant));
  vertical-align: middle;
}

.variables-table tr.selected {
  background-color: rgba(var(--mdui-color-primary), 0.08);
}

.empty-state {
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  padding: 48px;
  color: rgb(var(--mdui-color-on-surface-variant));
}

.empty-state p {
  margin: 16px 0;
}
</style>

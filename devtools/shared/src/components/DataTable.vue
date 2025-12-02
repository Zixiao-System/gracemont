<template>
  <div class="data-table">
    <table>
      <thead>
        <tr>
          <th
            v-for="col in columns"
            :key="col.key"
            :style="{ width: col.width }"
            :class="{ sortable: col.sortable }"
            @click="col.sortable && toggleSort(col.key)"
          >
            {{ col.label }}
            <span v-if="col.sortable && sortKey === col.key" class="sort-icon">
              {{ sortOrder === 'asc' ? '↑' : '↓' }}
            </span>
          </th>
        </tr>
      </thead>
      <tbody>
        <tr
          v-for="(row, idx) in sortedData"
          :key="row[rowKey] || idx"
          :class="{ selected: selectedRow === idx }"
          @click="selectRow(idx)"
          @dblclick="$emit('dblclick', row)"
        >
          <td v-for="col in columns" :key="col.key">
            <slot :name="`cell-${col.key}`" :row="row" :value="row[col.key]">
              {{ formatValue(row[col.key], col) }}
            </slot>
          </td>
        </tr>
        <tr v-if="sortedData.length === 0">
          <td :colspan="columns.length" class="empty-row">
            {{ emptyText }}
          </td>
        </tr>
      </tbody>
    </table>
  </div>
</template>

<script setup>
import { ref, computed } from 'vue'

const props = defineProps({
  columns: {
    type: Array,
    required: true
    // { key: string, label: string, width?: string, sortable?: boolean, format?: Function }
  },
  data: {
    type: Array,
    default: () => []
  },
  rowKey: {
    type: String,
    default: 'id'
  },
  emptyText: {
    type: String,
    default: '暂无数据'
  }
})

const emit = defineEmits(['select', 'dblclick'])

const selectedRow = ref(null)
const sortKey = ref(null)
const sortOrder = ref('asc')

const sortedData = computed(() => {
  if (!sortKey.value) return props.data

  return [...props.data].sort((a, b) => {
    const aVal = a[sortKey.value]
    const bVal = b[sortKey.value]

    if (aVal === bVal) return 0
    if (aVal === null || aVal === undefined) return 1
    if (bVal === null || bVal === undefined) return -1

    let result
    if (typeof aVal === 'number' && typeof bVal === 'number') {
      result = aVal - bVal
    } else {
      result = String(aVal).localeCompare(String(bVal))
    }

    return sortOrder.value === 'asc' ? result : -result
  })
})

function toggleSort(key) {
  if (sortKey.value === key) {
    sortOrder.value = sortOrder.value === 'asc' ? 'desc' : 'asc'
  } else {
    sortKey.value = key
    sortOrder.value = 'asc'
  }
}

function selectRow(idx) {
  selectedRow.value = idx
  emit('select', sortedData.value[idx])
}

function formatValue(value, col) {
  if (col.format) {
    return col.format(value)
  }
  if (value === null || value === undefined) {
    return '-'
  }
  return value
}
</script>

<style scoped>
.data-table {
  width: 100%;
  height: 100%;
  overflow: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  font-size: 13px;
}

thead {
  position: sticky;
  top: 0;
  background: #f5f5f5;
  z-index: 1;
}

th {
  padding: 10px 12px;
  text-align: left;
  font-weight: 500;
  border-bottom: 2px solid #e0e0e0;
  user-select: none;
}

th.sortable {
  cursor: pointer;
}

th.sortable:hover {
  background: #eeeeee;
}

.sort-icon {
  margin-left: 4px;
  color: #666;
}

td {
  padding: 8px 12px;
  border-bottom: 1px solid #eee;
}

tr:hover {
  background: #f9f9f9;
}

tr.selected {
  background: #e3f2fd;
}

tr.selected:hover {
  background: #bbdefb;
}

.empty-row {
  text-align: center;
  color: #999;
  padding: 40px;
}
</style>

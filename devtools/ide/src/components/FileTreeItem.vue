<template>
  <div class="file-tree-item">
    <div
      class="item-content"
      :class="{ 'is-directory': item.type === 'directory' }"
      @click="handleClick"
      :style="{ paddingLeft: (depth * 16 + 8) + 'px' }"
    >
      <!-- 展开/折叠图标 -->
      <mdui-button-icon
        v-if="item.type === 'directory'"
        @click.stop="toggleExpanded"
        style="--mdui-comp-button-icon-size: 24px"
      >
        <mdui-icon-chevron-right
          v-if="!expanded"
          style="font-size: 18px"
        ></mdui-icon-chevron-right>
        <mdui-icon-expand-more
          v-else
          style="font-size: 18px"
        ></mdui-icon-expand-more>
      </mdui-button-icon>
      <div v-else style="width: 24px"></div>

      <!-- 文件/文件夹图标 -->
      <mdui-icon-folder
        v-if="item.type === 'directory'"
        style="font-size: 18px; color: #ffa726"
      ></mdui-icon-folder>
      <mdui-icon-description
        v-else
        style="font-size: 18px; color: rgb(var(--mdui-color-primary))"
      ></mdui-icon-description>

      <!-- 文件/文件夹名称 -->
      <span class="item-name">{{ item.name }}</span>
    </div>

    <!-- 子项 -->
    <div v-if="item.type === 'directory' && expanded && item.children">
      <FileTreeItem
        v-for="child in item.children"
        :key="child.path"
        :item="child"
        :depth="depth + 1"
        @select="$emit('select', $event)"
      />
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'

// 导入图标
import '@mdui/icons/chevron-right.js'
import '@mdui/icons/expand-more.js'
import '@mdui/icons/folder.js'
import '@mdui/icons/description.js'

import 'mdui/components/button-icon.js'

const props = defineProps({
  item: {
    type: Object,
    required: true
  },
  depth: {
    type: Number,
    default: 0
  }
})

const emit = defineEmits(['select'])

const expanded = ref(false)

const toggleExpanded = () => {
  expanded.value = !expanded.value
}

const handleClick = () => {
  if (props.item.type === 'directory') {
    toggleExpanded()
  } else {
    emit('select', props.item)
  }
}
</script>

<style scoped>
.file-tree-item {
  user-select: none;
}

.item-content {
  display: flex;
  align-items: center;
  gap: 4px;
  padding: 4px 8px;
  cursor: pointer;
  border-radius: 4px;
  transition: background-color 0.2s;
}

.item-content:hover {
  background-color: rgb(var(--mdui-color-surface-container-high));
}

.item-content.is-directory {
  font-weight: 500;
}

.item-name {
  font-size: 14px;
  flex: 1;
  overflow: hidden;
  text-overflow: ellipsis;
  white-space: nowrap;
}
</style>

<template>
  <div class="toolbar">
    <div class="toolbar-group" v-for="(group, gIdx) in groups" :key="gIdx">
      <template v-for="(item, idx) in group" :key="idx">
        <mdui-tooltip v-if="item.tooltip" :content="item.tooltip">
          <mdui-button-icon
            :disabled="item.disabled"
            @click="$emit('action', item.action)"
          >
            <component :is="item.icon"></component>
          </mdui-button-icon>
        </mdui-tooltip>
        <mdui-button-icon
          v-else
          :disabled="item.disabled"
          @click="$emit('action', item.action)"
        >
          <component :is="item.icon"></component>
        </mdui-button-icon>
      </template>
      <mdui-divider v-if="gIdx < groups.length - 1" vertical class="toolbar-divider"></mdui-divider>
    </div>
    <slot></slot>
  </div>
</template>

<script setup>
defineProps({
  groups: {
    type: Array,
    default: () => []
  }
})

defineEmits(['action'])
</script>

<style scoped>
.toolbar {
  display: flex;
  align-items: center;
  padding: 4px 8px;
  background: #f5f5f5;
  border-bottom: 1px solid #e0e0e0;
  gap: 4px;
}

.toolbar-group {
  display: flex;
  align-items: center;
  gap: 2px;
}

.toolbar-divider {
  height: 24px;
  margin: 0 8px;
}
</style>

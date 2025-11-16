<template>
  <div class="text-center">
    <v-snackbar
    v-model="snackbar.isOpen"
    @update:model-value="onSnackbarUpdate"
    class="centeredSnackbar"
    transition="md-slide"
    :color="snackbar.status"
    :timeout="-1"
    >   
    <p>{{ snackbar.message }}</p>
    <template #actions>
      <v-btn
      v-for="act of snackbar.customActions"
      :key="act.actionName"
      :to="act.link"
      >
      {{ act.actionName }}
      </v-btn>
      <v-btn v-if="isClosable" variant="tonal" @click="closeSnackbar">Close</v-btn>
    </template>
    </v-snackbar>
  </div>
</template>
   
<script lang="ts" setup>
  import { toRef, computed } from 'vue'
  import { useSnackbarStore } from '@/stores/SnackbarStore'
  
  const snackbar = useSnackbarStore()
  
  const closeSnackbar = () => {
    snackbar.closeSnackbar()
  }

  const isClosable = computed(() => snackbar.isClosable())

  function onSnackbarUpdate(value: boolean) {
  if (!value) {
    snackbar.closeSnackbar()
  }
}
</script>
   
<style>
  .centeredSnackbar  {
    text-align: center;
    text-transform: uppercase !important;
  }

  .md-slide-enter-active,
  .md-slide-leave-active {
    transition: all 0.25s ease;
  }

  .md-slide-enter-from {
    opacity: 0;
    transform: translateY(50px);
  }

  .md-slide-leave-to {
    opacity: 0;
    transform: translateY(50px);
  }

</style>

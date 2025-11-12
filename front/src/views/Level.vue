<template>
  <v-container fluid>
    <v-card class="pa-5">
      <VolumeSlider name="center" v-bind:value="center_level" :loading="loading" />
      <VolumeSlider name="rear" v-bind:value="rear_level" :loading="loading" />
      <VolumeSlider name="sub" v-bind:value="sub_level" :loading="loading" />
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import VolumeSlider from '@/components/VolumeSlider.vue'

defineProps({
  center_level: { type: Number, required: true },
  rear_level: { type: Number, required: true },
  sub_level: { type: Number, required: true },
  loading: { type: Boolean, required: true },
})
</script>

<template>
  <v-container fluid>
    <v-card class="pa-5">
      <v-skeleton-loader :loading="loading" type="avatar@2">
        <v-btn :color="standby ? 'primary' : ''" @click="CyclePower" icon class="mx-2">
          <v-icon icon="$power" :color="standby ? '' : 'green'"></v-icon>
        </v-btn>
        <v-btn class="mx-2" :color="muted ? 'red' : 'grey-darken-2'" @click="CycleMute" icon>
          <v-icon :icon="muted ? '$muteOn' : '$muteOff'"></v-icon>
        </v-btn>
      </v-skeleton-loader>
      <VolumeSlider name="main" v-bind:value="main_level" :loading="loading" class="py-5" />
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import axios from 'axios'
import VolumeSlider from '@/components/VolumeSlider.vue'

const props = defineProps({
  muted: { type: Boolean, required: true },
  main_level: { type: Number, required: true },
  standby: { type: Number, required: true },
  loading: { type: Boolean, required: true },
})

const CyclePower = () => {
  const query: string = props.standby ? 'on' : 'off'
  axios.get('/power/' + query)
}

const CycleMute = () => {
  const query: string = props.muted ? 'off' : 'on'
  axios.get('/mute/' + query)
}
</script>

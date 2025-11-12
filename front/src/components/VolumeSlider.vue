<template>
  <div class="py-2">
    <div class="text-caption">{{ name }}</div>
    <v-skeleton-loader :loading="loading" type="list-item">
      <v-slider
        :model-value="volume"
        thumb-label
        hide-details
        prepend-icon="$volMinus"
        append-icon="$volPlus"
        color="primary"
        :track-color="trackColor"
        @click:prepend="VolumeDown"
        @click:append="VolumeUp"
        @end="ChangeVolume"
        :min="0"
        :max="43"
        :step="1"
      ></v-slider>
    </v-skeleton-loader>
  </div>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import axios from 'axios'

const props = defineProps({
  name: String,
  value: { type: Number, default: 0 },
  loading: Boolean,
})

const volume = computed({
  get(): number {
    return props.value
  },
  set() {},
})

const trackColor = computed(() => {
  switch (true) {
    case props.value < 5:
      return 'primary-lighten-2'
    case props.value > 25:
      return 'red'
    default:
      return 'primary'
  }
})

const ChangeVolume = (val: number) => {
  axios.get('volume/' + props.name + '/set?value=' + Math.round((val * 255) / 43)).then(() => {
    volume.value = val
  })
}

const VolumeUp = () => {
  axios.get('volume/' + props.name + '/up').then(() => {
    volume.value = Math.min(43, volume.value + 1)
  })
}

const VolumeDown = () => {
  axios.get('volume/' + props.name + '/down').then(() => {
    volume.value = Math.max(0, volume.value - 1)
  })
}
</script>

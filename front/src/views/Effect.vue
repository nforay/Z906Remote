<template>
  <v-container fluid>
    <v-card class="pa-5">
      <v-skeleton-loader :loading="loading" type="actions, button@4" class="d-flex justify-space-around">
        <v-container>
          <v-switch
            v-model="decodeMode"
            color="primary"
            @update:model="SetDecode"
            append-icon="$dolby"
            class="d-flex justify-end"
            hide-details
          >
            <v-tooltip :open-on-hover="true" :open-delay="1000" activator="parent" location="right">test </v-tooltip>
          </v-switch>
        </v-container>
        <v-container fluid>
          <v-btn-toggle
            v-model="selectedEffect"
            @update:model-value="SetEffect"
            :disabled="decodeMode"
            :color="decodeMode ? 'grey-lighten-1' : 'primary'"
            shaped
            mandatory
            class="d-flex"
          >
            <v-btn :value="0" class="flex-grow-1">
              <v-tooltip v-bind:open-on-hover="!decodeMode" :open-delay="1000" activator="parent" location="bottom">
                Stereo 3D: Provides a 3D surround sound effect through all speakers.</v-tooltip
              >3D
            </v-btn>

            <v-btn :value="1" class="flex-grow-1">
              <v-tooltip v-bind:open-on-hover="!decodeMode" :open-delay="1000" activator="parent" location="bottom">
                Stereo 2.1: Plays stereo through both the front satellites and the subwoofer.</v-tooltip
              >2.1
            </v-btn>

            <v-btn :value="2" class="flex-grow-1">
              <v-tooltip v-bind:open-on-hover="!decodeMode" :open-delay="1000" activator="parent" location="bottom">
                Stereo 4.1: Plays stereo through both front and both rear satellites, plus the subwoofer.</v-tooltip
              >4.1
            </v-btn>

            <v-btn v-if="current_input == 0" :value="3" class="flex-grow-1">
              <v-tooltip v-bind:open-on-hover="!decodeMode" :open-delay="1000" activator="parent" location="bottom">
                No effect: Plays audio format of your computer’s sound card without applying any effects. Available for
                6 channel input only.</v-tooltip
              >OFF
            </v-btn>
          </v-btn-toggle>
        </v-container>
      </v-skeleton-loader>
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import axios from 'axios'

const emit = defineEmits<{
  (e: 'update'): { type: void; required: true }
}>()

const props = defineProps({
  current_input: { type: Number, required: true },
  current_fx: { type: Number, required: true },
  decode_mode: { type: Boolean, required: true },
  loading: { type: Boolean, required: true },
})

const selectedEffect = computed<number>({
  get(): number {
    return props.current_fx
  },
  set() {},
})

const decodeMode = computed<boolean>({
  get(): boolean {
    return props.decode_mode
  },
  set(val: boolean) {
    SetDecode(!val)
  },
})

const SetEffect = (effect: number) => {
  if (effect != selectedEffect.value) {
    axios.get('/input/effect/' + effect).then(() => {
      emit('update')
    })
  }
}

const SetDecode = (input: boolean) => {
  axios.get('/input/decode/' + (input ? 'off' : 'on')).then(() => {
    emit('update')
  })
}
</script>

<template>
  <v-container fluid>
    <v-card class="pa-5">
      <v-skeleton-loader :loading="loading" type="list-item@6">
        <v-radio-group color="primary" hide-details @update:model-value="SetInput" v-model="selectedInput">
          <v-radio
            v-for="input in inputs"
            :key="input.value"
            true-icon="$radioTrue"
            false-icon="$radioFalse"
            :label="input.label"
            :value="input.value"
          ></v-radio>
        </v-radio-group>
      </v-skeleton-loader>
    </v-card>
  </v-container>
</template>

<script setup lang="ts">
import { computed } from 'vue'
import axios from 'axios'

const props = defineProps({
  current_input: { type: Number, required: true },
  loading: { type: Boolean, required: true },
})

const inputs = [
  { label: '6-channel direct', value: 0 },
  { label: 'RCA', value: 1 },
  { label: 'Digital optical 1', value: 2 },
  { label: 'Digital optical 2', value: 3 },
  { label: 'Digital coaxial', value: 4 },
  { label: 'Auxiliary input', value: 5 },
]

const selectedInput = computed<Number>({
  get(): Number {
    return props.current_input
  },
  set() {},
})

const SetInput = (input: Number | null) => {
  if (input != selectedInput.value) {
    axios.get('/input/' + input)
  }
}
</script>

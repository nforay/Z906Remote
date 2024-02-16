<template>
  <v-tabs v-model="currentTab" color="primary" fixed-tabs>
    <v-tab
      class="px-0"
      v-for="item in tabs"
      :key="item.value"
      :text="item.title"
      :prepend-icon="item.icon"
      :value="item.value"
      :href="item.value"
    ></v-tab>
  </v-tabs>
  <v-window v-model="currentTab" :touch="false">
    <v-window-item key="main" value="#main">
      <MainView
        :loading="loading"
        :muted="status.data.muted"
        :main_level="status.data.main_level"
        :standby="status.data.stby"
        @update="GetStatus"
      ></MainView>
    </v-window-item>
    <v-window-item key="volume" value="#level">
      <LevelView
        :loading="loading"
        :center_level="status.data.center_level"
        :rear_level="status.data.rear_level"
        :sub_level="status.data.sub_level"
        @update="GetStatus"
      ></LevelView>
    </v-window-item>
    <v-window-item key="input" value="#input">
      <InputView :loading="loading" :current_input="status.data.current_input" @update="GetStatus"></InputView>
    </v-window-item>
    <v-window-item key="effect" value="#effect">
      <EffectView
        :loading="loading"
        :current_fx="status.data.current_fx"
        :decode_mode="status.data.decode_mode"
        @update="GetStatus"
      ></EffectView>
    </v-window-item>
  </v-window>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import axios from 'axios'
import MainView from '@/views/Main.vue'
import LevelView from '@/views/Level.vue'
import InputView from '@/views/Input.vue'
import EffectView from '@/views/Effect.vue'
import Status from '@/models/statusDTO'

const status = ref<Status>(new Status())
const loading = ref(true)
const router = useRouter()

const currentTab = ref(router.currentRoute.value.hash)
const tabs = [
  { title: 'main', icon: '$speaker', value: '#main' },
  { title: 'level', icon: '$tune', value: '#level' },
  { title: 'input', icon: '$rca', value: '#input' },
  { title: 'effect', icon: '$surround', value: '#effect' },
]

const GetStatus = () => {
  axios.get<Status>('/status').then((response) => {
    status.value = response.data
    loading.value = false
  })
}

onMounted(() => {
  GetStatus()
})
</script>

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
        :muted="status.muted"
        :main_level="status.main_level"
        :standby="status.stby"
      ></MainView>
    </v-window-item>
    <v-window-item key="volume" value="#level">
      <LevelView
        :loading="loading"
        :center_level="status.center_level"
        :rear_level="status.rear_level"
        :sub_level="status.sub_level"
      ></LevelView>
    </v-window-item>
    <v-window-item key="input" value="#input">
      <InputView :loading="loading" :current_input="status.current_input"></InputView>
    </v-window-item>
    <v-window-item key="effect" value="#effect">
      <EffectView
        :loading="loading"
        :current_fx="status.current_fx"
        :decode_mode="status.decode_mode"
      ></EffectView>
    </v-window-item>
  </v-window>
</template>

<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import axios from 'axios'
import MainView from '@/views/Main.vue'
import LevelView from '@/views/Level.vue'
import InputView from '@/views/Input.vue'
import EffectView from '@/views/Effect.vue'
import Status, { StatusDTO, IStatus } from '@/models/statusDTO'
import { useSnackbarStore } from '@/stores/SnackbarStore'

const status = ref<Status>(new Status(new StatusDTO()))
const loading = ref(true)
const router = useRouter()
const snackbar = useSnackbarStore()
const endpoint = import.meta.env.VITE_API_ENDPOINT
const wsendpoint = endpoint.includes("://") ? endpoint.split("://")[1] : endpoint;
const ws = ref<WebSocket | null>(null)
const isConnected = ref(false)
let retryTimeout: number | undefined
const baseDelay = 5000
const maxDelay = 30000
let currentDelay = baseDelay

const currentTab = ref(router.currentRoute.value.hash)
const tabs = [
  { title: 'main', icon: '$speaker', value: '#main' },
  { title: 'level', icon: '$tune', value: '#level' },
  { title: 'input', icon: '$rca', value: '#input' },
  { title: 'effect', icon: '$surround', value: '#effect' },
]

const Connect = () => {
  snackbar.showSnackbar(`Attempting WebSocket connection...`, 'info')
  ws.value = new WebSocket(((window.location.protocol === "https:") ? "wss://" : "ws://") + wsendpoint + "/ws")

  ws.value.onopen = () => {
    snackbar.showSnackbar(`Websocket Connected !`, 'success')
    isConnected.value = true
    currentDelay = baseDelay
  }

  ws.value.onclose = () => {
    snackbar.showSnackbar(`Websocket Disconnected !`, 'warning')
    isConnected.value = false
    RetryConnect()
  }

  ws.value.onerror = (err) => {
    snackbar.showSnackbar(`Websocket Error: ${err.type}`, 'error')
    console.error("Websocket Error", err)
    ws.value?.close()
  }

  ws.value.onmessage = (event) => {
    try {
      const incoming = JSON.parse(event.data) as {
        data: Partial<IStatus>
      }
      Object.assign(status.value, incoming.data)
    } catch (err) {
      snackbar.showSnackbar(`Invalid Message : ${event.data}`, 'error')
    }
  }
}

const RetryConnect = () => {
    if (retryTimeout) return
    retryTimeout = window.setTimeout(() => {
      retryTimeout = undefined
      Connect()
      currentDelay = Math.min(currentDelay * 2, maxDelay)
      snackbar.showSnackbar(`Retrying WebSocket connection in ${currentDelay / 1000}s...`, 'info')
    }, currentDelay)
  }

const close = () => {
  ws.value?.close()
  if (retryTimeout) {
    clearTimeout(retryTimeout)
    retryTimeout = undefined
  }
}

interface IStatusApiResponse {
  status: string
  success: boolean
  data: IStatus
}

const GetStatus = async () => {
  try {
    const response = await axios.get<IStatusApiResponse>('/status')
    
    if (response.data.success && response.data.data) {
      status.value = new Status(response.data.data)
    }
  } catch (err) {
    let message = 'Unknown error'

    if (axios.isAxiosError(err)) {
      message = err.response?.data?.message ?? err.response?.statusText ?? err.message
    } else if (err instanceof Error) {
      message = err.message
    }
    snackbar.showSnackbar(`Error fetching status : ${message}`, 'error')
  } finally {
    setTimeout(() => (loading.value = false), 500)
  }
}

onMounted(() => {
  Connect()
  GetStatus()
})

onUnmounted(() => {
  close()
})
</script>

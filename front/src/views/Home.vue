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
  <v-window v-model="currentTab">
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
import { onMounted, ref } from 'vue'
import { useRouter } from 'vue-router'
import MainView from '@/views/Main.vue'
import LevelView from '@/views/Level.vue'
import InputView from '@/views/Input.vue'
import EffectView from '@/views/Effect.vue'
import Status, { StatusDTO } from '@/models/statusDTO'
import ToastNotification from '@/models/toastNotificationDTO'
import LogMessage from '@/models/logMessageDTO'
import { useSnackbarStore } from '@/stores/SnackbarStore'
import { useVersionStore } from "@/stores/VersionStore"

const status = ref<Status>(new Status(new StatusDTO()))
const loading = ref(true)
const router = useRouter()
const snackbar = useSnackbarStore()
const version = useVersionStore()
const evtSource = ref<EventSource | null>(null)
const isConnected = ref(false)

const currentTab = ref(router.currentRoute.value.hash)
const tabs = [
  { title: 'main', icon: '$speaker', value: '#main' },
  { title: 'level', icon: '$tune', value: '#level' },
  { title: 'input', icon: '$rca', value: '#input' },
  { title: 'effect', icon: '$surround', value: '#effect' },
]

const connect = () => {
  snackbar.showSnackbar('connecting...', 'info')

  evtSource.value = new EventSource("/events")

  evtSource.value.onopen = () => {
    snackbar.showSnackbar('Connected !', 'success')
    isConnected.value = true
  }

  evtSource.value.onerror = (err) => {
    if (isConnected.value) {
      snackbar.showSnackbar('Disconnected ! Reconnecting...', 'warning')
      isConnected.value = false
    }
  }

  evtSource.value?.addEventListener("status", (event: MessageEvent) => {
    try {
      const data = JSON.parse(event.data)
      const partialDto: Partial<StatusDTO> = data.status
      Object.assign(status.value, partialDto)
      if (loading.value) {
        setTimeout(() => (loading.value = false), 500)
      }
    } catch (err) {
      console.error(event.data)
    }
  });

  evtSource.value?.addEventListener("log", (event: MessageEvent) => {
    try {
      const data = JSON.parse(event.data)
      const logMsg = new LogMessage(data)
      logMsg.logToConsole()
    } catch (err) {
      console.error(event.data)
    }
  });

  evtSource.value?.addEventListener("notification", (event: MessageEvent) => {
    try {
      const data = JSON.parse(event.data);
      const notif = new ToastNotification(data)
      snackbar.showSnackbar(notif.text, notif.type)
    } catch (err) {
      console.error(event.data)
    }
  });

  evtSource.value?.addEventListener("update", (event: MessageEvent) => {
    version.checkVersion(true);
  });
}

onMounted(() => {
  connect()
})
</script>

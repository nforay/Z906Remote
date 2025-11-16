<template>
  <router-view />
  <Snackbar
      :error-msg="snackbar.message"
      :snackbar-actions="snackbar.customActions"
      :snackbar-show="snackbar.isOpen"
      :snackbar-status="snackbar.status"
    />
</template>

<script lang="ts" setup>
import axios from 'axios'
import Snackbar from '@/components/SnackBar.vue'
import { useSnackbarStore } from '@/stores/SnackbarStore'

const endpoint = import.meta.env.VITE_API_ENDPOINT
const snackbar = useSnackbarStore()

//set env VITE_API_ENDPOINT to host the front-end on another device
axios.defaults.baseURL = (typeof endpoint !== 'undefined') ? endpoint : window.location.origin
axios.defaults.headers['Access-Control-Allow-Origin'] = '*'
</script>
